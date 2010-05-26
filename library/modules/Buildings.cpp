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
#include "modules/Buildings.h"

using namespace DFHack;

//raw
struct t_building_df40d
{
    uint32_t vtable;
    uint32_t x1;
    uint32_t y1;
    uint32_t centerx;
    uint32_t x2;
    uint32_t y2;
    uint32_t centery;
    uint32_t z;
    uint32_t height;
    t_matglossPair material;
    // not complete
};

struct Buildings::Private
{
    uint32_t buildings_vector;
    uint32_t custom_workshop_vector;
    uint32_t building_custom_workshop_type;
    uint32_t custom_workshop_type;
    uint32_t custom_workshop_name;
    int32_t custom_workshop_id;
    DfVector <uint32_t> * p_bld;
    DFContextPrivate *d;
    Process * owner;
    bool Inited;
    bool Started;
};

Buildings::Buildings(DFContextPrivate * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
    d->Inited = d->Started = false;
    memory_info * mem = d->d->offset_descriptor;
    d->custom_workshop_vector = mem->getAddress("custom_workshop_vector");
    d->building_custom_workshop_type = mem->getOffset("building_custom_workshop_type");
    d->custom_workshop_type = mem->getOffset("custom_workshop_type");
    d->custom_workshop_name = mem->getOffset("custom_workshop_name");
    d->buildings_vector = mem->getAddress ("buildings_vector");
    mem->resolveClassnameToClassID("building_custom_workshop", d->custom_workshop_id);
    d->Inited = true;
}

Buildings::~Buildings()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Buildings::Start(uint32_t & numbuildings)
{
    d->p_bld = new DfVector <uint32_t> (d->owner, d->buildings_vector);
    numbuildings = d->p_bld->size();
    d->Started = true;
    return true;
}

bool Buildings::Read (const uint32_t index, t_building & building)
{
    if(!d->Started)
        return false;
    t_building_df40d bld_40d;

    // read pointer from vector at position
    uint32_t temp = d->p_bld->at (index);
    //d->p_bld->read(index,(uint8_t *)&temp);

    //read building from memory
    d->owner->read (temp, sizeof (t_building_df40d), (uint8_t *) &bld_40d);

    // transform
    int32_t type = -1;
    d->owner->getDescriptor()->resolveObjectToClassID (temp, type);
    building.origin = temp;
    building.vtable = bld_40d.vtable;
    building.x1 = bld_40d.x1;
    building.x2 = bld_40d.x2;
    building.y1 = bld_40d.y1;
    building.y2 = bld_40d.y2;
    building.z = bld_40d.z;
    building.material = bld_40d.material;
    building.type = type;
    return true;
}

bool Buildings::Finish()
{
    if(d->p_bld)
    {
        delete d->p_bld;
        d->p_bld = NULL;
    }
    d->Started = false;
    return true;
}

bool Buildings::ReadCustomWorkshopTypes(map <uint32_t, string> & btypes)
{
    if(!d->Started)
        return false;
    
    Process * p = d->owner;
    DfVector <uint32_t> p_matgloss (p, d->custom_workshop_vector);
    uint32_t size = p_matgloss.size();
    btypes.clear();
    
    for (uint32_t i = 0; i < size;i++)
    {
        string out = p->readSTLString (p_matgloss[i] + d->custom_workshop_name);
        uint32_t type = p->readDWord (p_matgloss[i] + d->custom_workshop_type);
        #ifdef DEBUG
            cout << out << ": " << type << endl;
        #endif
        btypes[type] = out;
    }
    return true;
}

int32_t Buildings::GetCustomWorkshopType(t_building & building)
{
    if(!d->Inited)
        return false;
    int32_t type = (int32_t)building.type;
    int32_t ret = -1;
    if(type != -1 && type == d->custom_workshop_id)
    {
        // read the custom workshop subtype
        ret = (int32_t) d->owner->readDWord(building.origin + d->building_custom_workshop_type);
    }
    return ret;
}

