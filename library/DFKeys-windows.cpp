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
        switch (command)
        {
            case ENTER:
                input[0].ki.wVk = VK_RETURN;
                input[1].ki.wVk = VK_RETURN;
                break;
            case SPACE:
                input[0].ki.wVk = VK_SPACE;
                input[1].ki.wVk = VK_SPACE;
                break;
            case BACK_SPACE:
                input[0].ki.wVk = VK_BACK;
                input[1].ki.wVk = VK_BACK;
                break;
            case TAB:
                input[0].ki.wVk = VK_TAB;
                input[1].ki.wVk = VK_TAB;
                break;
            case CAPS_LOCK:
                input[0].ki.wVk = VK_CAPITAL;
                input[1].ki.wVk = VK_CAPITAL;
                break;
                // These are only for pressing the modifier key itself, you can't do key combinations with them, like ctrl+C
            case LEFT_SHIFT: // I am not positive that this will work to distinguish the left and right..
                input[0].ki.wVk = VK_LSHIFT;
                input[1].ki.wVk = VK_LSHIFT;
                break;
            case RIGHT_SHIFT:
                input[0].ki.wVk = VK_RSHIFT;
                input[1].ki.wVk = VK_RSHIFT;
                break;
            case LEFT_CONTROL:
                input[0].ki.wVk = VK_LCONTROL;
                input[1].ki.wVk = VK_LCONTROL;
                break;
            case RIGHT_CONTROL:
                input[0].ki.wVk = VK_RCONTROL;
                input[1].ki.wVk = VK_RCONTROL;
                break;
            case ALT:
                input[0].ki.wVk = VK_MENU;
                input[1].ki.wVk = VK_MENU;
                break;
            case ESCAPE:
                input[0].ki.wVk = VK_ESCAPE;
                input[1].ki.wVk = VK_ESCAPE;
                break;
            case UP:
                input[0].ki.wVk = VK_UP;
                input[1].ki.wVk = VK_UP;
                break;
            case DOWN:
                input[0].ki.wVk = VK_DOWN;
                input[1].ki.wVk = VK_DOWN;
                break;
            case LEFT:
                input[0].ki.wVk = VK_LEFT;
                input[1].ki.wVk = VK_LEFT;
                break;
            case RIGHT:
                input[0].ki.wVk = VK_RIGHT;
                input[1].ki.wVk = VK_RIGHT;
                break;
            case F1:
                input[0].ki.wVk = VK_F1;
                input[1].ki.wVk = VK_F1;
                break;
            case F2:
                input[0].ki.wVk = VK_F2;
                input[1].ki.wVk = VK_F2;
                break;
            case F3:
                input[0].ki.wVk = VK_F3;
                input[1].ki.wVk = VK_F3;
                break;
            case F4:
                input[0].ki.wVk = VK_F4;
                input[1].ki.wVk = VK_F4;
                break;
            case F5:
                input[0].ki.wVk = VK_F5;
                input[1].ki.wVk = VK_F5;
                break;
            case F6:
                input[0].ki.wVk = VK_F6;
                input[1].ki.wVk = VK_F6;
                break;
            case F7:
                input[0].ki.wVk = VK_F7;
                input[1].ki.wVk = VK_F7;
                break;
            case F8:
                input[0].ki.wVk = VK_F8;
                input[1].ki.wVk = VK_F8;
                break;
            case F9:
                input[0].ki.wVk = VK_F9;
                input[1].ki.wVk = VK_F9;
                break;
            case F10:
                input[0].ki.wVk = VK_F10;
                input[1].ki.wVk = VK_F10;
                break;
            case F11:
                input[0].ki.wVk = VK_F11;
                input[1].ki.wVk = VK_F11;
                break;
            case F12:
                input[0].ki.wVk = VK_F12;
                input[1].ki.wVk = VK_F12;
                break;
            case PAGE_UP:
                input[0].ki.wVk = VK_PRIOR;
                input[1].ki.wVk = VK_PRIOR;
                break;
            case PAGE_DOWN:
                input[0].ki.wVk = VK_NEXT;
                input[1].ki.wVk = VK_NEXT;
                break;
            case INSERT:
                input[0].ki.wVk = VK_INSERT;
                input[1].ki.wVk = VK_INSERT;
                break;
            case KEY_DELETE:
                input[0].ki.wVk = VK_DELETE;
                input[1].ki.wVk = VK_DELETE;
                break;
            case HOME:
                input[0].ki.wVk = VK_HOME;
                input[1].ki.wVk = VK_HOME;
                break;
            case END:
                input[0].ki.wVk = VK_END;
                input[1].ki.wVk = VK_END;
                break;
            case KEYPAD_DIVIDE:
                input[0].ki.wVk = VK_DIVIDE;
                input[1].ki.wVk = VK_DIVIDE;
                break;
            case KEYPAD_MULTIPLY:
                input[0].ki.wVk = VK_MULTIPLY;
                input[1].ki.wVk = VK_MULTIPLY;
                break;
            case KEYPAD_SUBTRACT:
                input[0].ki.wVk = VK_SUBTRACT;
                input[1].ki.wVk = VK_SUBTRACT;
                break;
            case KEYPAD_ADD:
                input[0].ki.wVk = VK_ADD;
                input[1].ki.wVk = VK_ADD;
                break;
            case KEYPAD_ENTER:
                input[0].ki.wVk = VK_RETURN;
                input[1].ki.wVk = VK_RETURN;
                break;
            case KEYPAD_0:
                input[0].ki.wVk = VK_NUMPAD0;
                input[1].ki.wVk = VK_NUMPAD0;
                break;
            case KEYPAD_1:
                input[0].ki.wVk = VK_NUMPAD1;
                input[1].ki.wVk = VK_NUMPAD1;
                break;
            case KEYPAD_2:
                input[0].ki.wVk = VK_NUMPAD2;
                input[1].ki.wVk = VK_NUMPAD2;
                break;
            case KEYPAD_3:
                input[0].ki.wVk = VK_NUMPAD3;
                input[1].ki.wVk = VK_NUMPAD3;
                break;
            case KEYPAD_4:
                input[0].ki.wVk = VK_NUMPAD4;
                input[1].ki.wVk = VK_NUMPAD4;
                break;
            case KEYPAD_5:
                input[0].ki.wVk = VK_NUMPAD5;
                input[1].ki.wVk = VK_NUMPAD5;
                break;
            case KEYPAD_6:
                input[0].ki.wVk = VK_NUMPAD6;
                input[1].ki.wVk = VK_NUMPAD6;
                break;
            case KEYPAD_7:
                input[0].ki.wVk = VK_NUMPAD7;
                input[1].ki.wVk = VK_NUMPAD7;
                break;
            case KEYPAD_8:
                input[0].ki.wVk = VK_NUMPAD8;
                input[1].ki.wVk = VK_NUMPAD8;
                break;
            case KEYPAD_9:
                input[0].ki.wVk = VK_NUMPAD9;
                input[1].ki.wVk = VK_NUMPAD9;
                break;
            case KEYPAD_DECIMAL_POINT:
                input[0].ki.wVk = VK_SEPARATOR;
                input[1].ki.wVk = VK_SEPARATOR;
                break;
        }
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
