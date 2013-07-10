/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

#include "Core.h"
#include "Hooks.h"
#include <iostream>

// hook - called before rendering
DFhackCExport int egg_init(void)
{
    // reroute stderr
    freopen("stderr.log", "w", stderr);
    // we don't reroute stdout until  we figure out if this should be done at all
    // See: Console-linux.cpp
    fprintf(stderr,"dfhack: hooking successful\n");
    return true;
}

// hook - called before rendering
DFhackCExport int egg_shutdown(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.Shutdown();
}

// hook - called for each game tick (or more often)
DFhackCExport int egg_tick(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.Update();
}
// hook - called before rendering
DFhackCExport int egg_prerender(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.TileUpdate();
}

// hook - called for each SDL event, returns 0 when the event has been consumed. 1 otherwise
DFhackCExport int egg_sdl_event(SDL::Event* event)
{
    // if the event is valid, intercept
    if( event != 0 )
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        return c.DFH_SDL_Event(event);
    }
    return true;
}

// return this if you want to kill the event.
const int curses_error = -1;
// hook - ncurses event, -1 signifies error.
DFhackCExport int egg_curses_event(int orig_return)
{
    /*
    if(orig_return != -1)
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        int out;
        return c.ncurses_wgetch(orig_return,);
    }*/
    return orig_return;
}
