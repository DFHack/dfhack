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

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "ContextShared.h"

#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include "dfhack/Types.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Translation.h"
#include "ModuleFactory.h"
using namespace DFHack;

Module* DFHack::createVegetation(DFContextShared * d)
{
    return new Vegetation(d);
}

struct Vegetation::Private
{
    uint32_t vegetation_vector;
    uint32_t tree_desc_offset;
    // translation
    DfVector <uint32_t> * p_veg;

    DFContextShared *d;
    Process * owner;
    bool Inited;
    bool Started;
};

Vegetation::Vegetation(DFContextShared * d_)
{
    d = new Private;
    d->owner = d_->p;
    d->d = d_;
    d->Inited = d->Started = false;
    OffsetGroup * OG_Veg = d->d->offset_descriptor->getGroup("Vegetation");
    d->vegetation_vector = OG_Veg->getAddress ("vector");
    d->tree_desc_offset = OG_Veg->getOffset ("tree_desc_offset");
    d->Inited = true;
}

Vegetation::~Vegetation()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Vegetation::Start(uint32_t & numplants)
{
    if(!d->Inited)
        return false;
    d->p_veg = new DfVector <uint32_t> (d->vegetation_vector);
    numplants = d->p_veg->size();
    d->Started = true;
    return true;
}


bool Vegetation::Read (const uint32_t index, dfh_plant & shrubbery)
{
    if(!d->Started)
        return false;
    // read pointer from vector at position
    uint32_t temp = d->p_veg->at (index);
    // read from memory
    d->d->readName(shrubbery.name,temp);
    d->owner->read (temp + d->tree_desc_offset, sizeof (t_plant), (uint8_t *) &shrubbery.sdata);
    shrubbery.address = temp;
    return true;
}

bool Vegetation::Write (dfh_plant & shrubbery)
{
    if(!d->Started)
        return false;
    d->owner->write (shrubbery.address + d->tree_desc_offset, sizeof (t_plant), (uint8_t *) &shrubbery.sdata);
    return true;
}


bool Vegetation::Finish()
{
    if(d->p_veg)
    {
        delete d->p_veg;
        d->p_veg = 0;
    }
    d->Started = false;
    return true;
}

