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
#define LINUX_BUILD
#include "DFCommonInternal.h"
using namespace DFHack;

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
                XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XStringToKeysym ("Shift_L")), false, CurrentTime);

                switch (command)
                {
                    case ENTER:
                        mykeysym = XStringToKeysym ("Return");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case SPACE:
                        mykeysym = XStringToKeysym ("space");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case BACK_SPACE:
                        mykeysym = XStringToKeysym ("BackSpace");
                        xkeycode = XK_BackSpace;
                        xkeycode = XKeysymToKeycode (dpy, XK_BackSpace);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case TAB:
                        mykeysym = XStringToKeysym ("Tab");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case CAPS_LOCK:
                        mykeysym = XStringToKeysym ("Caps_Lock");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case LEFT_SHIFT: // I am not positive that this will work to distinguish the left and right..
                        mykeysym = XStringToKeysym ("Shift_L");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case RIGHT_SHIFT:
                        mykeysym = XStringToKeysym ("Shift_R");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case LEFT_CONTROL:
                        mykeysym = XStringToKeysym ("Control_L");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                    case RIGHT_CONTROL:
                        mykeysym = XStringToKeysym ("Control_R");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case ALT:
                        mykeysym = XStringToKeysym ("Alt_L");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case ESCAPE:
                        mykeysym = XStringToKeysym ("Escape");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case UP:
                        mykeysym = XStringToKeysym ("Up");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case DOWN:
                        mykeysym = XStringToKeysym ("Down");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case LEFT:
                        mykeysym = XStringToKeysym ("Left");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case RIGHT:
                        mykeysym = XStringToKeysym ("Right");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F1:
                        mykeysym = XStringToKeysym ("F1");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F2:
                        mykeysym = XStringToKeysym ("F2");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F3:
                        mykeysym = XStringToKeysym ("F3");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F4:
                        mykeysym = XStringToKeysym ("F4");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F5:
                        mykeysym = XStringToKeysym ("F5");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F6:
                        mykeysym = XStringToKeysym ("F6");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F7:
                        mykeysym = XStringToKeysym ("F7");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F8:
                        mykeysym = XStringToKeysym ("F8");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F9:
                        mykeysym = XStringToKeysym ("F9");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F11:
                        mykeysym = XStringToKeysym ("F11");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case F12:
                        mykeysym = XStringToKeysym ("F12");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case PAGE_UP:
                        mykeysym = XStringToKeysym ("Page_Up");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case PAGE_DOWN:
                        mykeysym = XStringToKeysym ("Page_Down");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case INSERT:
                        mykeysym = XStringToKeysym ("Insert");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEY_DELETE:
                        mykeysym = XStringToKeysym ("Delete");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case HOME:
                        mykeysym = XStringToKeysym ("Home");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case END:
                        mykeysym = XStringToKeysym ("End");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_DIVIDE:
                        mykeysym = XStringToKeysym ("KP_Divide");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_MULTIPLY:
                        mykeysym = XStringToKeysym ("KP_Multiply");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_SUBTRACT:
                        mykeysym = XStringToKeysym ("KP_Subtract");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_ADD:
                        mykeysym = XStringToKeysym ("KP_Add");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_ENTER:
                        mykeysym = XStringToKeysym ("KP_Enter");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_0:
                        mykeysym = XStringToKeysym ("KP_0");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_1:
                        mykeysym = XStringToKeysym ("KP_1");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_2:
                        mykeysym = XStringToKeysym ("KP_2");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_3:
                        mykeysym = XStringToKeysym ("KP_3");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_4:
                        mykeysym = XStringToKeysym ("KP_4");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_5:
                        mykeysym = XStringToKeysym ("KP_5");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_6:
                        mykeysym = XStringToKeysym ("KP_6");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_7:
                        mykeysym = XStringToKeysym ("KP_7");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_8:
                        mykeysym = XStringToKeysym ("KP_8");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_9:
                        mykeysym = XStringToKeysym ("KP_9");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                    case KEYPAD_DECIMAL_POINT:
                        mykeysym = XStringToKeysym ("KP_Decimal");
                        xkeycode = XKeysymToKeycode (dpy, mykeysym);
                        XTestFakeKeyEvent (dpy, xkeycode, true, CurrentTime);
                        XSync (dpy, true);
                        XTestFakeKeyEvent (dpy, xkeycode, false, CurrentTime);
                        XSync (dpy, true);
                        break;
                }
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
