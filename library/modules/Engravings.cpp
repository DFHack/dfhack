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


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "modules/Engravings.h"
#include "ModuleFactory.h"
#include "Core.h"

using namespace DFHack;

struct Engravings::Private
{
    uint32_t engraving_vector;
    vector <t_engraving *> * p_engr;

    Process * owner;
    bool Inited;
    bool Started;
};

Module* DFHack::createEngravings()
{
    return new Engravings();
}

Engravings::Engravings()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    d->Inited = d->Started = false;
    d->p_engr = (decltype(d->p_engr)) c.vinfo->getGroup("Engravings")->getAddress ("vector");
    d->Inited = true;
}

Engravings::~Engravings()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Engravings::Start(uint32_t & numengravings)
{
    if(!d->Inited)
        return false;
    numengravings = d->p_engr->size();
    d->Started = true;
    return true;
}


bool Engravings::Read (const uint32_t index, dfh_engraving & engraving)
{
    if(!d->Started) return false;

    // read pointer from vector at position
    engraving.s = *d->p_engr->at (index);

    // transform
    engraving.origin = d->p_engr->at (index);
    return true;
}

bool Engravings::Write (const dfh_engraving & engraving)
{
    if(!d->Started) return false;
    //write engraving to memory
    d->owner->write (engraving.origin, sizeof (t_engraving), (uint8_t *) &(engraving.s));
    return true;
}

bool Engravings::Finish()
{
    d->Started = false;
    return true;
}

