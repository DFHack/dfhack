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

#include "dfhack/DFCommonInternal.h"
#include "../private/APIPrivate.h"
#include "modules/Translation.h"
#include "dfhack/DFMemInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include "dfhack/DFTypes.h"
#include "modules/Vegetation.h"

using namespace DFHack;

struct Vegetation::Private
{
    uint32_t vegetation_vector;
    uint32_t tree_desc_offset;
    // translation
    DfVector <uint32_t> * p_veg;
    
    DFContextPrivate *d;
    Process * owner;
    bool Inited;
    bool Started;
};

Vegetation::Vegetation(DFContextPrivate * d_)
{
    d = new Private;
    d->owner = d_->p;
    d->d = d_;
    d->Inited = d->Started = false;
    memory_info * mem = d->d->offset_descriptor;
    d->vegetation_vector = mem->getAddress ("vegetation_vector");
    d->tree_desc_offset = mem->getOffset ("tree_desc_offset");
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
    d->p_veg = new DfVector <uint32_t> (d->owner, d->vegetation_vector);
    numplants = d->p_veg->size();
    d->Started = true;
    return true;
}


bool Vegetation::Read (const uint32_t index, t_tree & shrubbery)
{
    if(!d->Started)
        return false;
    // read pointer from vector at position
    uint32_t temp = d->p_veg->at (index);
    // read from memory
    d->owner->read (temp + d->tree_desc_offset, sizeof (t_tree), (uint8_t *) &shrubbery);
    shrubbery.address = temp;
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

