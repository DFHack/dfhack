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
#include "modules/World.h"
#include "DFProcess.h"
#include "DFMemInfo.h"
#include "DFTypes.h"

using namespace DFHack;

struct World::Private
{
    bool Inited;
    bool Started;
    uint32_t year_offset;
    uint32_t tick_offset;
    APIPrivate *d;
    Process * owner;
};

World::World(APIPrivate * _d)
{
    
    d = new Private;
    d->d = _d;
    d->owner = _d->p;
    
    memory_info * mem = d->d->offset_descriptor;
    d->year_offset = mem->getAddress( "current_year" );
    d->tick_offset = mem->getAddress( "current_tick" );
    d->Inited = d->Started = true;
}

World::~World()
{
    delete d;
}

bool World::Start()
{
    return true;
}

bool World::Finish()
{
    return true;
}

uint32_t World::ReadCurrentYear()
{
    if(d->Inited)
        return(d->owner->readDWord(d->year_offset));
    return 0;
}

uint32_t World::ReadCurrentTick()
{
    if(d->Inited)
        return(d->owner->readDWord(d->tick_offset));
    return 0;
}

uint32_t World::ReadCurrentMonth()
{
    return this->ReadCurrentTick() / 1200 / 24;
}

uint32_t World::ReadCurrentDay()
{
    return ((this->ReadCurrentTick() / 1200) % 24) + 1;
}
