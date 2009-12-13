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

// TODO: investigate use of PostMessage() for input sending to background windows
void API::TypeStr (const char *lpszString, int delay, bool useShift)
{
    //Resume();
    ForceResume();

    //sendmessage needs a window handle HWND, so have to get that from the process HANDLE
    HWND currentWindow = GetForegroundWindow();
    window myWindow;
    myWindow.pid = GetProcessId (DFHack::g_ProcessHandle);
    EnumWindows (EnumWindowsProc, (LPARAM) &myWindow);

    HWND nextWindow = GetWindow (myWindow.windowHandle, GW_HWNDNEXT);

    SetActiveWindow (myWindow.windowHandle);
    SetForegroundWindow (myWindow.windowHandle);

    char cChar;

    while ( (cChar = *lpszString++)) // loops through chars
    {
        short vk = VkKeyScan (cChar); // keycode of char
        if (useShift || (vk >> 8) &1)   // char is capital, so need to hold down shift
        {
            //shift down
            INPUT input[4] = {0};
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = VK_SHIFT;

            input[1].type = INPUT_KEYBOARD;
            input[1].ki.wVk = vk;

            input[2].type = INPUT_KEYBOARD;
            input[2].ki.wVk = vk;
            input[2].ki.dwFlags = KEYEVENTF_KEYUP;

            // shift up
            input[3].type = INPUT_KEYBOARD;
            input[3].ki.wVk = VK_SHIFT;
            input[3].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput (4, input, sizeof (input[0]));
        }
        else
        {
            INPUT input[2] = {0};
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = vk;

            input[1].type = INPUT_KEYBOARD;
            input[1].ki.wVk = vk;
            input[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput (2, input, sizeof (input[0]));
        }
    }
    SetForegroundWindow (currentWindow);
    SetActiveWindow (currentWindow);
    SetWindowPos (myWindow.windowHandle, nextWindow, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    Sleep (delay);

}
void API::TypeSpecial (t_special command, int count, int delay)
{
    ForceResume();
    if (command != WAIT)
    {
        HWND currentWindow = GetForegroundWindow();
        window myWindow;
        myWindow.pid = GetProcessId (DFHack::g_ProcessHandle);
        EnumWindows (EnumWindowsProc, (LPARAM) &myWindow);

        HWND nextWindow = GetWindow (myWindow.windowHandle, GW_HWNDNEXT);
        SetForegroundWindow (myWindow.windowHandle);
        SetActiveWindow (myWindow.windowHandle);
        INPUT shift;
        shift.type = INPUT_KEYBOARD;
        shift.ki.wVk = VK_SHIFT;
        shift.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput (1, &shift, sizeof (shift));
        INPUT input[2] = {0};
        input[0].type = INPUT_KEYBOARD;
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        input[0].ki.wVk = input[1].ki.wVk = ksTable[command];
        for (int i = 0; i < count;i++)
        {
            SendInput (2, input, sizeof (input[0]));
        }
        SetForegroundWindow (currentWindow);
        SetActiveWindow (currentWindow);
        SetWindowPos (myWindow.windowHandle, nextWindow, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Sleep (delay);
    }
    else
    {
        Sleep (delay*count);
    }
}
