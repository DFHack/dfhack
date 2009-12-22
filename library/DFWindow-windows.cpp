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
using namespace DFHack;

// should always reflect the enum in DFkeys.h
const static int ksTable[NUM_SPECIALS]=
{
    VK_RETURN,
    VK_SPACE,
    VK_BACK,
    VK_TAB,
    VK_CAPITAL,
    VK_LSHIFT,
    VK_RSHIFT,
    VK_LCONTROL,
    VK_RCONTROL,
    VK_MENU,
    0, //XK_VoidSymbol, // WAIT
    VK_ESCAPE,
    VK_UP,
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,
    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_F5,
    VK_F6,
    VK_F7,
    VK_F8,
    VK_F9,
    VK_F10,
    VK_F11,
    VK_F12,
    VK_PRIOR,
    VK_NEXT,
    VK_INSERT,
    VK_DELETE,
    VK_HOME,
    VK_END,
    VK_DIVIDE,
    VK_MULTIPLY,
    VK_SUBTRACT,
    VK_ADD,
    VK_RETURN,
    VK_NUMPAD0,
    VK_NUMPAD1,
    VK_NUMPAD2,
    VK_NUMPAD3,
    VK_NUMPAD4,
    VK_NUMPAD5,
    VK_NUMPAD6,
    VK_NUMPAD7,
    VK_NUMPAD8,
    VK_NUMPAD9,
    VK_SEPARATOR
};
//Windows key handlers
struct window
{
    HWND windowHandle;
    uint32_t pid;
};

BOOL CALLBACK EnumWindowsProc (HWND hwnd, LPARAM lParam)
{
    uint32_t pid;
    
    GetWindowThreadProcessId (hwnd, (LPDWORD) &pid);
    if (pid == ( (window *) lParam)->pid)
    {
        ( (window *) lParam)->windowHandle = hwnd;
        return FALSE;
    }
        return TRUE;
}

class DFWindow::Private
{
    public:
        Private(Process * _p)
        {
            p=_p;
        };
        ~Private(){};
        // our parent process
        Process * p;
};

// ctor
DFWindow::DFWindow (Process * p)
{
    d = new Private(p);
}

// dtor
DFWindow::~DFWindow ()
{}

// TODO: also investigate possible problems with UIPI on Vista and 7
void DFWindow::TypeStr (const char *input, int delay, bool useShift)
{
    //sendmessage needs a window handle HWND, so have to get that from the process HANDLE
    HWND currentWindow = GetForegroundWindow();
    window myWindow;
    myWindow.pid = d->p->getPID();
    EnumWindows (EnumWindowsProc, (LPARAM) &myWindow);

    char cChar;
    DWORD dfProccess = GetWindowThreadProcessId(myWindow.windowHandle,NULL);
    DWORD currentProccess = GetWindowThreadProcessId(currentWindow,NULL);
    AttachThreadInput(currentProccess,dfProccess,TRUE); //The two threads have to have attached input in order to change the keyboard state, which is needed to set the shift state
    while ( (cChar = *input++)) // loops through chars
    {
        short vk = VkKeyScan (cChar); // keycode of char
        if (useShift || (vk >> 8) &1)   // char is capital, so need to hold down shift
        {
            BYTE keybstate[256] = {0};
            BYTE keybstateOrig[256] = {0};
            GetKeyboardState((LPBYTE)&keybstateOrig);
            GetKeyboardState((LPBYTE)&keybstate);
            keybstate[VK_SHIFT] |= 0x80; //Set shift state to on in variable
            SetKeyboardState((LPBYTE)&keybstate); //set the current state to the variable
            SendMessage(myWindow.windowHandle,WM_KEYDOWN,vk,0);
            SendMessage(myWindow.windowHandle,WM_KEYUP,vk,0);
            SetKeyboardState((LPBYTE)&keybstateOrig); // reset the shift state to the original state
        }
        else
        {
            SendMessage(myWindow.windowHandle,WM_KEYDOWN,vk,0);
            SendMessage(myWindow.windowHandle,WM_KEYUP,vk,0);
        }
    }
    AttachThreadInput(currentProccess,dfProccess,FALSE); //detach the threads
    Sleep (delay);
}

void DFWindow::TypeSpecial (t_special command, int count, int delay)
{
    if (command != WAIT)
    {
        HWND currentWindow = GetForegroundWindow();
        window myWindow;
        myWindow.pid = d->p->getPID();
        EnumWindows (EnumWindowsProc, (LPARAM) &myWindow);

        for (int i = 0; i < count;i++)
        {
            SendMessage(myWindow.windowHandle,WM_KEYDOWN,ksTable[command],0);
            SendMessage(myWindow.windowHandle,WM_KEYUP,ksTable[command],0);
        }
        Sleep (delay);
    }
    else
    {
        Sleep (delay*count);
    }
}
