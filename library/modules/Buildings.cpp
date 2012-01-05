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
#include "Error.h"
#include "modules/Buildings.h"
#include "ModuleFactory.h"
#include "Core.h"
using namespace DFHack;

#include "DataDefs.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/building.h"
#include "df/building_workshopst.h"

using namespace df::enums;
using df::global::world;
using df::building_def;

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
    Process * owner;
    bool Inited;
    bool Started;
    int32_t custom_workshop_id;
};

Module* DFHack::createBuildings()
{
    return new Buildings();
}

Buildings::Buildings()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->Started = false;
    d->owner = c.p;
    d->Inited = true;
    c.vinfo->resolveClassnameToClassID("building_custom_workshop", d->custom_workshop_id);
}

Buildings::~Buildings()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Buildings::Start(uint32_t & numbuildings)
{
    if(!d->Inited)
        return false;
    numbuildings = world->buildings.all.size();
    d->Started = true;
    return true;
}

bool Buildings::Read (const uint32_t index, t_building & building)
{
    if(!d->Started)
        return false;
    df::building *bld_40d = world->buildings.all[index];

    // transform
    int32_t type = -1;
    d->owner->getDescriptor()->resolveObjectToClassID ( (char *)bld_40d, type);
    building.x1 = bld_40d->x1;
    building.x2 = bld_40d->x2;
    building.y1 = bld_40d->y1;
    building.y2 = bld_40d->y2;
    building.z = bld_40d->z;
    building.material.index = bld_40d->materialIndex;
    building.material.type = bld_40d->materialType;
    building.type = type;
    building.origin = (void *) &bld_40d;
    return true;
}

bool Buildings::Finish()
{
    d->Started = false;
    return true;
}

bool Buildings::ReadCustomWorkshopTypes(map <uint32_t, string> & btypes)
{
    if(!d->Inited)
        return false;
    Core & c = Core::getInstance();

    Process * p = d->owner;
    vector <building_def *> & bld_def = world->raws.buildings.all;
    uint32_t size = bld_def.size();
    btypes.clear();

    c.con.print("Probing vector at 0x%x for custom workshops.\n", &bld_def);
    for (auto iter = bld_def.begin(); iter != bld_def.end();iter++)
    {
        building_def * temp = *iter;
        btypes[temp->id] = temp->code;
        c.con.print("%d : %s\n",temp->id, temp->code.c_str());
    }
    return true;
}

// FIXME: ugly hack
int32_t Buildings::GetCustomWorkshopType(t_building & building)
{
    if(!d->Inited)
        return false;
    int32_t type = (int32_t)building.type;
    int32_t ret = -1;
    if(type != -1 && type == d->custom_workshop_id)
    {
        // read the custom workshop subtype
        df::building_workshopst * workshop = (df::building_workshopst *) building.origin;
        ret = workshop->custom_type;
    }
    return ret;
}

