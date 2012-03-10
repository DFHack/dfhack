
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
#include "modules/kitchen.h"
#include "ModuleFactory.h"
#include "Core.h"
using namespace DFHack;

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/item_type.h"
#include "df/plant_raw.h"

using namespace df::enums;
using df::global::world;
using df::global::ui;

void Kitchen::debug_print(color_ostream &out)
{
    out.print("Kitchen Exclusions\n");
    for(std::size_t i = 0; i < size(); ++i)
    {
        out.print("%2u: IT:%2i IS:%i MT:%3i MI:%2i ET:%i %s\n",
                       i,
                       ui->kitchen.item_types[i],
                       ui->kitchen.item_subtypes[i],
                       ui->kitchen.mat_types[i],
                       ui->kitchen.mat_indices[i],
                       ui->kitchen.exc_types[i],
                       (ui->kitchen.mat_types[i] >= 419 && ui->kitchen.mat_types[i] <= 618) ? world->raws.plants.all[ui->kitchen.mat_indices[i]]->id.c_str() : "n/a"
        );
    }
    out.print("\n");
}

void Kitchen::allowPlantSeedCookery(t_materialIndex materialIndex)
{
    bool match = false;
    do
    {
        match = false;
        std::size_t matchIndex = 0;
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(ui->kitchen.mat_indices[i] == materialIndex
               && (ui->kitchen.item_types[i] == item_type::SEEDS || ui->kitchen.item_types[i] == item_type::PLANT)
               && ui->kitchen.exc_types[i] == cookingExclusion
            )
            {
                match = true;
                matchIndex = i;
            }
        }
        if(match)
        {
            ui->kitchen.item_types.erase(ui->kitchen.item_types.begin() + matchIndex);
            ui->kitchen.item_subtypes.erase(ui->kitchen.item_subtypes.begin() + matchIndex);
            ui->kitchen.mat_indices.erase(ui->kitchen.mat_indices.begin() + matchIndex);
            ui->kitchen.mat_types.erase(ui->kitchen.mat_types.begin() + matchIndex);
            ui->kitchen.exc_types.erase(ui->kitchen.exc_types.begin() + matchIndex);
        }
    } while(match);
};

void Kitchen::denyPlantSeedCookery(t_materialIndex materialIndex)
{
    df::plant_raw *type = world->raws.plants.all[materialIndex];
    bool SeedAlreadyIn = false;
    bool PlantAlreadyIn = false;
    for(std::size_t i = 0; i < size(); ++i)
    {
        if(ui->kitchen.mat_indices[i] == materialIndex
           && ui->kitchen.exc_types[i] == cookingExclusion)
        {
            if(ui->kitchen.item_types[i] == item_type::SEEDS)
                SeedAlreadyIn = true;
            else if (ui->kitchen.item_types[i] == item_type::PLANT)
                PlantAlreadyIn = true;
        }
    }
    if(!SeedAlreadyIn)
    {
        ui->kitchen.item_types.push_back(item_type::SEEDS);
        ui->kitchen.item_subtypes.push_back(organicSubtype);
        ui->kitchen.mat_types.push_back(type->material_defs.type_seed);
        ui->kitchen.mat_indices.push_back(materialIndex);
        ui->kitchen.exc_types.push_back(cookingExclusion);
    }
    if(!PlantAlreadyIn)
    {
        ui->kitchen.item_types.push_back(item_type::PLANT);
        ui->kitchen.item_subtypes.push_back(organicSubtype);
        ui->kitchen.mat_types.push_back(type->material_defs.type_basic_mat);
        ui->kitchen.mat_indices.push_back(materialIndex);
        ui->kitchen.exc_types.push_back(cookingExclusion);
    }
};

void Kitchen::fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap)
{
    watchMap.clear();
    for(std::size_t i = 0; i < size(); ++i)
    {
        if(ui->kitchen.item_subtypes[i] == limitType && ui->kitchen.item_subtypes[i] == limitSubtype && ui->kitchen.exc_types[i] == limitExclusion)
        {
            watchMap[ui->kitchen.mat_indices[i]] = (unsigned int) ui->kitchen.mat_types[i];
        }
    }
};

void Kitchen::removeLimit(t_materialIndex materialIndex)
{
    bool match = false;
    do
    {
        match = false;
        std::size_t matchIndex = 0;
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(ui->kitchen.item_types[i] == limitType
               && ui->kitchen.item_subtypes[i] == limitSubtype
               && ui->kitchen.mat_indices[i] == materialIndex
               && ui->kitchen.exc_types[i] == limitExclusion)
            {
                match = true;
                matchIndex = i;
            }
        }
        if(match)
        {
            ui->kitchen.item_types.erase(ui->kitchen.item_types.begin() + matchIndex);
            ui->kitchen.item_subtypes.erase(ui->kitchen.item_subtypes.begin() + matchIndex);
            ui->kitchen.mat_types.erase(ui->kitchen.mat_types.begin() + matchIndex);
            ui->kitchen.mat_indices.erase(ui->kitchen.mat_indices.begin() + matchIndex);
            ui->kitchen.exc_types.erase(ui->kitchen.exc_types.begin() + matchIndex);
        }
    } while(match);
};

void Kitchen::setLimit(t_materialIndex materialIndex, unsigned int limit)
{
    removeLimit(materialIndex);
    if(limit > seedLimit)
    {
        limit = seedLimit;
    }
    ui->kitchen.item_types.push_back(limitType);
    ui->kitchen.item_subtypes.push_back(limitSubtype);
    ui->kitchen.mat_indices.push_back(materialIndex);
    ui->kitchen.mat_types.push_back((t_materialType) (limit < seedLimit) ? limit : seedLimit);
    ui->kitchen.exc_types.push_back(limitExclusion);
};

void Kitchen::clearLimits()
{
    bool match = false;
    do
    {
        match = false;
        std::size_t matchIndex;
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(ui->kitchen.item_types[i] == limitType
               && ui->kitchen.item_subtypes[i] == limitSubtype
               && ui->kitchen.exc_types[i] == limitExclusion)
            {
                match = true;
                matchIndex = i;
            }
        }
        if(match)
        {
            ui->kitchen.item_types.erase(ui->kitchen.item_types.begin() + matchIndex);
            ui->kitchen.item_subtypes.erase(ui->kitchen.item_subtypes.begin() + matchIndex);
            ui->kitchen.mat_indices.erase(ui->kitchen.mat_indices.begin() + matchIndex);
            ui->kitchen.mat_types.erase(ui->kitchen.mat_types.begin() + matchIndex);
            ui->kitchen.exc_types.erase(ui->kitchen.exc_types.begin() + matchIndex);
        }
    } while(match);
};

size_t Kitchen::size()
{
    return ui->kitchen.item_types.size();
};
