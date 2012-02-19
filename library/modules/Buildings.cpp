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
using namespace DFHack::Simple;

#include "DataDefs.h"
#include "df/world.h"
#include "df/building_def.h"
#include "df/building_workshopst.h"

using namespace df::enums;
using df::global::world;
using df::building_def;

uint32_t Buildings::getNumBuildings()
{
    return world->buildings.all.size();
}

bool Buildings::Read (const uint32_t index, t_building & building)
{
    Core & c = Core::getInstance();
    df::building *bld = world->buildings.all[index];

    building.x1 = bld->x1;
    building.x2 = bld->x2;
    building.y1 = bld->y1;
    building.y2 = bld->y2;
    building.z = bld->z;
    building.material.index = bld->mat_index;
    building.material.type = bld->mat_type;
    building.type = bld->getType();
    building.subtype = bld->getSubtype();
    building.custom_type = bld->getCustomType();
    building.origin = bld;
    return true;
}

bool Buildings::ReadCustomWorkshopTypes(map <uint32_t, string> & btypes)
{
    Core & c = Core::getInstance();

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

