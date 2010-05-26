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
#include "ContextShared.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFMemInfo.h"
#include "dfhack/DFTypes.h"

using namespace DFHack;

struct Gui::Private
{
    bool Inited;
    bool Started;
    uint32_t pause_state_offset;
    uint32_t view_screen_offset;
    uint32_t current_cursor_creature_offset;
    uint32_t current_menu_state_offset;
    DFContextShared *d;
    Process * owner;
};

Gui::Gui(DFContextShared * _d)
{
    
    d = new Private;
    d->d = _d;
    d->owner = _d->p;
    d->Inited = d->Started = true;
    
    memory_info * mem = d->d->offset_descriptor;
    d->current_menu_state_offset = mem->getAddress("current_menu_state");
    d->pause_state_offset = mem->getAddress ("pause_state");
    d->view_screen_offset = mem->getAddress ("view_screen");
    d->Inited = d->Started = true;
}

Gui::~Gui()
{
    delete d;
}

bool Gui::Start()
{
    return true;
}

bool Gui::Finish()
{
    return true;
}

bool Gui::ReadPauseState()
{
    // replace with an exception
    if(!d->Inited) return false;

    uint32_t pauseState = d->owner->readDWord (d->pause_state_offset);
    return pauseState & 1;
}

uint32_t Gui::ReadMenuState()
{
    if(d->Inited)
        return(d->owner->readDWord(d->current_menu_state_offset));
    return false;
}

bool Gui::ReadViewScreen (t_viewscreen &screen)
{
    if (!d->Inited) return false;
    Process * p = d->owner;
    
    uint32_t last = p->readDWord (d->view_screen_offset);
    uint32_t screenAddr = p->readDWord (last);
    uint32_t nextScreenPtr = p->readDWord (last + 4);
    while (nextScreenPtr != 0)
    {
        last = nextScreenPtr;
        screenAddr = p->readDWord (nextScreenPtr);
        nextScreenPtr = p->readDWord (nextScreenPtr + 4);
    }
    return d->d->offset_descriptor->resolveObjectToClassID (last, screen.type);
}
