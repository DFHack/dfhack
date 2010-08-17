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

/*
FIXME: Japa said that he had to do this with the time stuff he got from here
       Investigate.

    currentYear = Wold->ReadCurrentYear();
    currentTick = Wold->ReadCurrentTick();
    currentMonth = (currentTick+9)/33600;
    currentDay = ((currentTick+9)%33600)/1200;
    currentHour = ((currentTick+9)-(((currentMonth*28)+currentDay)*1200))/50;
    currentTickRel = (currentTick+9)-(((((currentMonth*28)+currentDay)*24)+currentHour)*50);
    */

#include "Internal.h"
#include "ContextShared.h"
#include "dfhack/modules/World.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFMemInfo.h"
#include "dfhack/DFTypes.h"

using namespace DFHack;

struct World::Private
{
    bool Inited;
    bool Started;
    uint32_t year_offset;
    uint32_t tick_offset;
    DFContextShared *d;
    Process * owner;
};

World::World(DFContextShared * _d)
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

// FIX'D according to this: 
/*
World::ReadCurrentMonth and World::ReadCurrentDay
« Sent to: peterix on: June 04, 2010, 04:44:30 »
« You have forwarded or responded to this message. »
ReplyQuoteRemove
Shouldn't these be /28 and %28 instead of 24?  There're 28 days in a DF month.
Using 28 and doing the calculation on the value stored at the memory location
specified by memory.xml gets me the current month/date.
*/
uint32_t World::ReadCurrentMonth()
{
    return this->ReadCurrentTick() / 1200 / 28;
}

uint32_t World::ReadCurrentDay()
{
    return ((this->ReadCurrentTick() / 1200) % 28) + 1;
}
