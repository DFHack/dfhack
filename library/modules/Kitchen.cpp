
#include "Internal.h"

#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <map>
#include <set>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "Error.h"
#include "modules/Kitchen.h"
#include "ModuleFactory.h"
#include "Core.h"
using namespace DFHack;

#include "DataDefs.h"
#include "df/world.h"
#include "df/plotinfost.h"
#include "df/item_type.h"
#include "df/plant_raw.h"

using namespace df::enums;
using df::global::world;
using df::global::plotinfo;

void Kitchen::debug_print(color_ostream &out)
{
    out.print("Kitchen Exclusions\n");
    for(std::size_t i = 0; i < size(); ++i)
    {
        out.print("%2zu: IT:%2i IS:%i MT:%3i MI:%2i ET:%i %s\n",
                       i,
                       plotinfo->kitchen.item_types[i],
                       plotinfo->kitchen.item_subtypes[i],
                       plotinfo->kitchen.mat_types[i],
                       plotinfo->kitchen.mat_indices[i],
                       plotinfo->kitchen.exc_types[i].whole,
                       (plotinfo->kitchen.mat_types[i] >= 419 && plotinfo->kitchen.mat_types[i] <= 618) ? world->raws.plants.all[plotinfo->kitchen.mat_indices[i]]->id.c_str() : "n/a"
        );
    }
    out.print("\n");
}

static df::kitchen_exc_type get_cook_type(){
    df::kitchen_exc_type cook_type;
    cook_type.bits.Cook = true;
    return cook_type;
}

void Kitchen::allowPlantSeedCookery(int32_t plant_id)
{
    if (plant_id < 0 || (size_t)plant_id >= world->raws.plants.all.size())
        return;

    df::plant_raw *type = world->raws.plants.all[plant_id];
    static df::kitchen_exc_type cook_type = get_cook_type();

    removeExclusion(cook_type, item_type::SEEDS, -1,
        type->material_defs.type[plant_material_def::seed],
        type->material_defs.idx[plant_material_def::seed]);

    removeExclusion(cook_type, item_type::PLANT, -1,
        type->material_defs.type[plant_material_def::basic_mat],
        type->material_defs.idx[plant_material_def::basic_mat]);
}

void Kitchen::denyPlantSeedCookery(int32_t plant_id)
{
    if (plant_id < 0 || (size_t)plant_id >= world->raws.plants.all.size())
        return;

    df::plant_raw *type = world->raws.plants.all[plant_id];
    static df::kitchen_exc_type cook_type = get_cook_type();

    addExclusion(cook_type, item_type::SEEDS, -1,
        type->material_defs.type[plant_material_def::seed],
        type->material_defs.idx[plant_material_def::seed]);

    addExclusion(cook_type, item_type::PLANT, -1,
        type->material_defs.type[plant_material_def::basic_mat],
        type->material_defs.idx[plant_material_def::basic_mat]);
}

bool Kitchen::isPlantCookeryAllowed(int32_t plant_id) {
    if (plant_id < 0 || (size_t)plant_id >= world->raws.plants.all.size())
        return false;

    df::plant_raw *type = world->raws.plants.all[plant_id];
    static df::kitchen_exc_type cook_type = get_cook_type();

    return findExclusion(cook_type, item_type::PLANT, -1,
            type->material_defs.type[plant_material_def::basic_mat],
            type->material_defs.idx[plant_material_def::basic_mat]) < 0;
}

bool Kitchen::isSeedCookeryAllowed(int32_t plant_id) {
    if (plant_id < 0 || (size_t)plant_id >= world->raws.plants.all.size())
        return false;

    df::plant_raw *type = world->raws.plants.all[plant_id];
    static df::kitchen_exc_type cook_type = get_cook_type();

    return findExclusion(cook_type, item_type::SEEDS, -1,
                type->material_defs.type[plant_material_def::seed],
                type->material_defs.idx[plant_material_def::seed]) < 0;
}

size_t Kitchen::size()
{
    return plotinfo->kitchen.item_types.size();
}

int Kitchen::findExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index)
{
    for (size_t i = 0; i < size(); i++)
    {
        if (plotinfo->kitchen.item_types[i] == item_type &&
            plotinfo->kitchen.item_subtypes[i] == item_subtype &&
            plotinfo->kitchen.mat_types[i] == mat_type &&
            plotinfo->kitchen.mat_indices[i] == mat_index &&
            plotinfo->kitchen.exc_types[i].whole == type.whole)
        {
            return int(i);
        }
    }
    return -1;
}

bool Kitchen::addExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index)
{
    // exactly one flag must be set
    if (!type.whole || type.whole > 2)
        return false;

    if (findExclusion(type, item_type, item_subtype, mat_type, mat_index) >= 0)
        return false;

    plotinfo->kitchen.item_types.push_back(item_type);
    plotinfo->kitchen.item_subtypes.push_back(item_subtype);
    plotinfo->kitchen.mat_types.push_back(mat_type);
    plotinfo->kitchen.mat_indices.push_back(mat_index);
    plotinfo->kitchen.exc_types.push_back(type);
    return true;
}

bool Kitchen::removeExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index)
{
    int i = findExclusion(types, item_type, item_subtype, mat_type, mat_index);
    if (i < 0)
        return false;

    plotinfo->kitchen.item_types.erase(plotinfo->kitchen.item_types.begin() + i);
    plotinfo->kitchen.item_subtypes.erase(plotinfo->kitchen.item_subtypes.begin() + i);
    plotinfo->kitchen.mat_types.erase(plotinfo->kitchen.mat_types.begin() + i);
    plotinfo->kitchen.mat_indices.erase(plotinfo->kitchen.mat_indices.begin() + i);
    plotinfo->kitchen.exc_types.erase(plotinfo->kitchen.exc_types.begin() + i);
    return true;
}
