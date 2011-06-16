/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
#pragma once

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