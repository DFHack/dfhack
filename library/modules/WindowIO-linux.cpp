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
#include "Internal.h"
#include "dfhack/modules/WindowIO.h"

#include <X11/Xlib.h>   //need for X11 functions
#include <X11/keysym.h>
#include <ContextShared.h>

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

class WindowIO::Private
{
    public:
        Private(Process * _p)
        {
            p=_p;
        };
        ~Private(){};
        
        // Source: http://www.experts-exchange.com/OS/Unix/X_Windows/Q_21341279.html
        // find named window recursively
        Window EnumerateWindows (Display *display, Window rootWindow, const char *searchString);
        
        // iterate through X screens, find the window
        // FIXME: use _NET_WM_PID primarily and this current stuff only as a fallback
        // see http://www.mail-archive.com/devel@xfree86.org/msg05818.html
        bool getDFWindow (Display *dpy, Window& dfWindow, Window & rootWindow);
        
        // send synthetic key events to a window
        // Source: http://homepage3.nifty.com/tsato/xvkbd/events.html
        void send_xkeyevent(Display *display, Window dfW,Window rootW, int keycode, int modstate, int is_press, useconds_t delay);
        
        // our parent process
        Process * p;
};

// ctor
WindowIO::WindowIO (DFContextShared * csh)
{
    d = new Private(csh->p);
}

// dtor
WindowIO::~WindowIO ()
{
    delete d;
}

Window WindowIO::Private::EnumerateWindows (Display *display, Window rootWindow, const char *searchString)
{
    Window parent;
    Window *children;
    Window retWindow = BadWindow;
    unsigned int noOfChildren;
    int status;
    
    // get name of current window, compare to control, return if found
    char * win_name;
    status = XFetchName (display, rootWindow, &win_name);
    if ( (status >= Success) && (win_name) && strcmp (win_name, searchString) == 0)
    {
        return rootWindow;
    }

    // look at surrounding window tree nodes, bailout on error or no children
    status = XQueryTree (display, rootWindow, &rootWindow, &parent, &children, &noOfChildren);
    if (!status || !noOfChildren)
    {
        return BadWindow;
    }
    
    // recurse into children
    for (uint32_t i = 0; i < noOfChildren; i++)
    {
        Window tempWindow = EnumerateWindows (display, children[i], searchString);
        if (tempWindow != BadWindow)
        {
            retWindow = tempWindow;
            break;
        }
    }
    
    // free resources
    XFree ( (char*) children);
    return retWindow;
}

bool WindowIO::Private::getDFWindow (Display *dpy, Window& dfWindow, Window & rootWindow)
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
    }
    return false;
}

void WindowIO::Private::send_xkeyevent(Display *display, Window dfW,Window rootW, int keycode, int modstate, int is_press, useconds_t delay)
{
    XKeyEvent event;
    
    event.display = display;
    event.window = dfW;
    event.root = rootW;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = true;
    
    event.type = (is_press ? KeyPress : KeyRelease);
    event.keycode = keycode;
    event.state = modstate;
    XSendEvent(event.display, event.window, true, KeyPressMask, (XEvent *) &event);
    usleep(delay);
}

void WindowIO::TypeStr (const char *input, int delay, bool useShift)
{
    Display *dpy = XOpenDisplay (NULL); // null opens the display in $DISPLAY
    Window dfWin;
    Window rootWin;
    if (d->getDFWindow (dpy, dfWin, rootWin))
    {
        char cChar;
        int realDelay = delay * 1000;
        KeyCode xkeycode;
        while ( (cChar = *input++)) // loops through chars
        {
            // HACK: the timing here is a strange beast
            xkeycode = XKeysymToKeycode (dpy, cChar);
            d->send_xkeyevent(dpy,dfWin,rootWin,XKeysymToKeycode(dpy, ksTable[DFHack::LEFT_SHIFT]),0,false, realDelay);
            if (useShift || (cChar >= 'A' && cChar <= 'Z'))
            {
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,ShiftMask,true, realDelay);
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,ShiftMask,false, realDelay);
                XSync (dpy, false);
            }
            else
            {
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,0,true, realDelay);
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,0,false, realDelay);
                XSync (dpy, false);
            }
        }
    }
    else
    {
        cout << "FAIL!" << endl;
    }
}

void WindowIO::TypeSpecial (t_special command, int count, int delay)
{
    if (command != WAIT)
    {
        KeySym mykeysym;
        KeyCode xkeycode;
        Display *dpy = XOpenDisplay (NULL); // null opens the display in $DISPLAY
        int realDelay = delay * 1000;
        Window dfWin;
        Window rootWin;
        if (d->getDFWindow (dpy, dfWin, rootWin))
        {
            for (int i = 0;i < count; i++)
            {
                // HACK: the timing here is a strange beast
                mykeysym = ksTable[command];
                xkeycode = XKeysymToKeycode (dpy, mykeysym);
                d->send_xkeyevent(dpy,dfWin,rootWin,XKeysymToKeycode(dpy, ksTable[DFHack::LEFT_SHIFT]),0,false, realDelay);
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,0,true, realDelay);
                d->send_xkeyevent(dpy,dfWin,rootWin,xkeycode,0,false, realDelay);
                XSync (dpy, false);
            }
        }
        else
        {
            cout << "FAIL!" << endl;
        }
    }
    else
    {
        usleep (delay*1000 * count);
    }
}
