/*
https://github.com/peterix/dfhack
Copyright (c) 2011 Petr Mrázek <peterix@gmail.com>

A thread-safe logging console with a line editor for windows.

Based on linenoise win32 port,
copyright 2010, Jon Griffiths <jon_p_griffiths at yahoo dot com>.
All rights reserved.
Based on linenoise, copyright 2010, Salvatore Sanfilippo <antirez at gmail dot com>.
The original linenoise can be found at: http://github.com/antirez/linenoise

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of Redis nor the names of its contributors may be used
    to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>
#include <stdint.h>

#include "Console.h"
#include "RemoteClient.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

void enablevt100() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == NULL) {
        return;
    }
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        if (!(dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}

const char* exeName = "Dwarf Fortress.exe";
#else
    // Assume Posix
    void enablevt100() { return; }
    const char* exeName = "./dfhack";
#endif

using namespace DFHack;
using namespace dfproto;

int main (int argc, char *argv[])
{
    auto conPtr = Console::makeConsole();
    Console& out = *conPtr;

    if (argc <= 1)
    {
        fprintf(stderr, "Usage: dfhack-run <command> [args...]\n\n");
        fprintf(stderr, "Note: this command does not start DFHack; it is intended to connect\n"
                        "to a running DFHack instance. If you were trying to start DFHack, run\n"
                        "   %s\n"
                        "or see the documentation in hack/docs/index.html for more help.\n"
        , exeName);
#ifdef _WIN32
        fprintf(stderr, "\nPress Enter to quit.\n");
        fgetc(stdin);
#endif
        return 2;
    }

    enablevt100();

    // Connect to DFHack
    RemoteClient client(&out);
    if (!client.connect())
        return 2;

    out.init(true);

    command_result rv;

    if (strcmp(argv[1], "--lua") == 0)
    {
        if (argc <= 3)
        {
            out.shutdown();
            fprintf(stderr, "Usage: dfhack-run --lua <module> <function> [args...]\n");
            return 2;
        }

        RemoteFunction<dfproto::CoreRunLuaRequest,dfproto::StringListMessage> run_call;

        if (!run_call.bind(&client, "RunLua"))
        {
            out.shutdown();
            fprintf(stderr, "No RunLua protocol function found.");
            return 3;
        }

        run_call.in()->set_module(argv[2]);
        run_call.in()->set_function(argv[3]);
        for (int i = 4; i < argc; i++)
            run_call.in()->add_arguments(argv[i]);

        rv = run_call();

        out.flush();

        if (rv == CR_OK)
        {
            for (int i = 0; i < run_call.out()->value_size(); i++)
                printf("%s%s", (i>0?"\t":""), run_call.out()->value(i).c_str());
            printf("\n");
        }
    }
    else
    {
        // Call the command
        std::vector<std::string> args;
        for (int i = 2; i < argc; i++)
            args.push_back(argv[i]);

        rv = client.run_command(argv[1], args);
    }

    out.flush();
    out.shutdown();

    if (rv != CR_OK)
        return 1;

    return 0;
}
