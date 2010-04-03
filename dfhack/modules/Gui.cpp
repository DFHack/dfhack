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
#include "../private/APIPrivate.h"
using namespace DFHack;

bool API::ReadPauseState()
{
    // replace with an exception
    if(!d->cursorWindowInited) return false;

    uint32_t pauseState = g_pProcess->readDWord (d->pause_state_offset);
    return pauseState & 1;
}

uint32_t API::ReadMenuState()
{
    if(d->cursorWindowInited)
        return(g_pProcess->readDWord(d->current_menu_state_offset));
    return false;
}

bool API::ReadViewScreen (t_viewscreen &screen)
{
    if (!d->cursorWindowInited) return false;
    
    uint32_t last = g_pProcess->readDWord (d->view_screen_offset);
    uint32_t screenAddr = g_pProcess->readDWord (last);
    uint32_t nextScreenPtr = g_pProcess->readDWord (last + 4);
    while (nextScreenPtr != 0)
    {
        last = nextScreenPtr;
        screenAddr = g_pProcess->readDWord (nextScreenPtr);
        nextScreenPtr = g_pProcess->readDWord (nextScreenPtr + 4);
    }
    return d->offset_descriptor->resolveObjectToClassID (last, screen.type);
}
