#ifndef TERMUTIL_H
#define TERMUTIL_H

#ifdef LINUX_BUILD
// FIXME: is this ever true?
bool TemporaryTerminal ()
{
    return false;
}
#else
#include <windows.h>
#include <stdio.h>
#include <conio.h>
bool TemporaryTerminal ()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdOutput;
    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(hStdOutput, &csbi))
    {
        printf("GetConsoleScreenBufferInfo failed: %d\n", GetLastError());
        return false;
    }
    return ((!csbi.dwCursorPosition.X) && (!csbi.dwCursorPosition.Y));
};

#endif // LINUX_BUILD

#endif