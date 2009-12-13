/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/
#include "DFCommonInternal.h"

#include <X11/Xlib.h>   //need for X11 functions
#include <X11/extensions/XTest.h> //need for Xtest
#include <X11/Xatom.h> //for the atom stuff
#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

using namespace DFHack;

// should always reflect the enum in DFkeys.h
const static KeySym ksTable[NUM_SPECIALS]=
{
    XK_Return,
    XK_space,
    XK_BackSpace,
    XK_Tab,
    XK_Caps_Lock,
    XK_Shift_L,
    XK_Shift_R,
    XK_Control_L,
    XK_Control_R,
    XK_Alt_L,
    XK_VoidSymbol, // WAIT
    XK_Escape,
    XK_Up,
    XK_Down,
    XK_Left,
    XK_Right,
    XK_F1,
    XK_F2,
    XK_F3,
    XK_F4,
    XK_F5,
    XK_F6,
    XK_F7,
    XK_F8,
    XK_F9,
    XK_F10,
    XK_F11,
    XK_F12,
    XK_Page_Up,
    XK_Page_Down,
    XK_Insert,
    XK_Delete,
    XK_Home,
    XK_End,
    XK_KP_Divide,
    XK_KP_Multiply,
    XK_KP_Subtract,
    XK_KP_Add,
    XK_KP_Enter,
    XK_KP_0,
    XK_KP_1,
    XK_KP_2,
    XK_KP_3,
    XK_KP_4,
    XK_KP_5,
    XK_KP_6,
    XK_KP_7,
    XK_KP_8,
    XK_KP_9,
    XK_KP_Decimal
};

// ENUMARATE THROUGH WINDOWS AND DISPLAY THEIR TITLES
Window EnumerateWindows (Display *display, Window rootWindow, const char *searchString)
{
    static int level = 0;
    Window parent;
    Window *children;
    Window *child;
    Window retWindow = BadWindow;
    unsigned int noOfChildren;
    int status;
    int i;

    char * win_name;
    status = XFetchName (display, rootWindow, &win_name);

    if ( (status >= Success) && (win_name) && strcmp (win_name, searchString) == 0)
    {
        return rootWindow;
    }

    level++;

    status = XQueryTree (display, rootWindow, &rootWindow, &parent, &children, &noOfChildren);

    if (status == 0)
    {
        return BadWindow;
    }

    if (noOfChildren == 0)
    {
        return BadWindow;
    }
    for (i = 0; i < noOfChildren; i++)
    {
        Window tempWindow = EnumerateWindows (display, children[i], searchString);
        if (tempWindow != BadWindow)
        {
            retWindow = tempWindow;
            break;
        }
    }

    XFree ( (char*) children);
    return retWindow;
}
// END ENUMERATE WINDOWS
bool getDFWindow (Display *dpy, Window& dfWindow, Window & rootWindow)
{
    //  int numScreeens = ScreenCount(dpy);
    for (int i = 0;i < ScreenCount (dpy);i++)
    {
        rootWindow = RootWindow (dpy, i);
        Window retWindow = EnumerateWindows (dpy, rootWindow, "Dwarf Fortress");
        if (retWindow != BadWindow)
        {
            dfWindow = retWindow;
            return true;
        }
        //I would ideally like to find the dfwindow using the PID, but X11 Windows only know their processes pid if the _NET_WM_PID attribute is set, which it is not for SDL 1.2.  Supposedly SDL 1.3 will set this, but who knows when that will occur.
    }
    return false;
}
bool setWMClientLeaderProperty (Display *dpy, Window &dfWin, Window &currentFocus)
{
    static bool propertySet;
    if (propertySet)
    {
        return true;
    }
    Atom leaderAtom;
    Atom type;
    int format, status;
    unsigned long nitems = 0;
    unsigned long extra = 0;
    unsigned char *data = 0;
    Window result = 0;
    leaderAtom = XInternAtom (dpy, "WM_CLIENT_LEADER", false);
    status = XGetWindowProperty (dpy, currentFocus, leaderAtom, 0L, 1L, false, XA_WINDOW, &type, &format, &nitems, &extra, (unsigned char **) & data);
    if (status == Success)
    {
        if (data != 0)
        {
            result = * ( (Window*) data);
            XFree (data);
        }
        else
        {
            Window curr = currentFocus;
            while (data == 0)
            {
                Window parent;
                Window root;
                Window *children;
                uint numChildren;
                XQueryTree (dpy, curr, &root, &parent, &children, &numChildren);
                XGetWindowProperty (dpy, parent, leaderAtom, 0L, 1L, false, XA_WINDOW, &type, &format, &nitems, &extra, (unsigned char **) &data);
            }
            result = * ( (Window*) data);
            XFree (data);
        }
    }
    XChangeProperty (dpy, dfWin, leaderAtom, XA_WINDOW, 32, PropModeReplace, (const unsigned char *) &result, 1);
    propertySet = true;
    return true;
}
void API::TypeStr (const char *lpszString, int delay, bool useShift)
{
    ForceResume();
    Display *dpy = XOpenDisplay (NULL); // null opens the display in $DISPLAY
    Window dfWin;
    Window rootWin;
    if (getDFWindow (dpy, dfWin, rootWin))
    {

        XWindowAttributes currAttr;
        Window currentFocus;
        int currentRevert;
        XGetInputFocus (dpy, &currentFocus, &currentRevert); //get current focus
        setWMClientLeaderProperty (dpy, dfWin, currentFocus);
        XGetWindowAttributes (dpy, dfWin, &currAttr);
        if (currAttr.map_state == IsUnmapped)
        {
            XMapRaised (dpy, dfWin);
        }
        if (currAttr.map_state == IsUnviewable)
        {
            XRaiseWindow (dpy, dfWin);
        }
        XSync (dpy, false);
        XSetInputFocus (dpy, dfWin, RevertToNone, CurrentTime);
        XSync (dpy, false);

        char cChar;
        KeyCode xkeycode;
        char prevKey = 0;
        int sleepAmnt = 0;
        while ( (cChar = *lpszString++)) // loops through chars
        {
            xkeycode = XKeysymToKeycode (dpy, cChar);
            //HACK  add an extra shift up event, this fixes the problem of the same character twice in a row being displayed in df
            XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XStringToKeysym ("Shift_L")), false, CurrentTime);
            if (useShift || cChar >= 'A' && cChar <= 'Z')
            {
                XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XStringToKeysym ("Shift_L")), true, CurrentTime);
                XSync (dpy, false);
            }
            XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
            XSync (dpy, false);
            XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
            XSync (dpy, false);
            if (useShift || cChar >= 'A' && cChar <= 'Z')
            {
                XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XStringToKeysym ("Shift_L")), false, CurrentTime);
                XSync (dpy, false);
            }

        }
        if (currAttr.map_state == IsUnmapped)
        {
            //      XUnmapWindow(dpy,dfWin); if I unmap the window, it is no longer on the task bar, so just lower it instead
            XLowerWindow (dpy, dfWin);
        }
        XSetInputFocus (dpy, currentFocus, currentRevert, CurrentTime);
        XSync (dpy, true);
    }
    usleep (delay*1000);
}

void API::TypeSpecial (t_special command, int count, int delay)
{
    ForceResume();
    if (command != WAIT)
    {
        KeySym mykeysym;
        KeyCode xkeycode;
        Display *dpy = XOpenDisplay (NULL); // null opens the display in $DISPLAY
        Window dfWin;
        Window rootWin;
        if (getDFWindow (dpy, dfWin, rootWin))
        {
            XWindowAttributes currAttr;
            Window currentFocus;
            int currentRevert;
            XGetInputFocus (dpy, &currentFocus, &currentRevert); //get current focus
            setWMClientLeaderProperty (dpy, dfWin, currentFocus);
            XGetWindowAttributes (dpy, dfWin, &currAttr);
            if (currAttr.map_state == IsUnmapped)
            {
                XMapRaised (dpy, dfWin);
            }
            if (currAttr.map_state == IsUnviewable)
            {
                XRaiseWindow (dpy, dfWin);
            }
            XSync (dpy, false);
            XSetInputFocus (dpy, dfWin, RevertToParent, CurrentTime);

            for (int i = 0;i < count; i++)
            {
                //HACK  add an extra shift up event, this fixes the problem of the same character twice in a row being displayed in df
                XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, ksTable[LEFT_SHIFT]), false, CurrentTime);
                mykeysym = ksTable[command];
                xkeycode = XKeysymToKeycode (dpy, mykeysym);
                XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                XSync (dpy, true);
                XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                XSync (dpy, true);
                usleep(20000);
            }
            if (currAttr.map_state == IsUnmapped)
            {
                XLowerWindow (dpy, dfWin); // can't unmap, because you lose task bar
            }
            XSetInputFocus (dpy, currentFocus, currentRevert, CurrentTime);
            XSync (dpy, false);
        }
    }
    else
    {
        usleep (delay*1000 * count);
    }
}
