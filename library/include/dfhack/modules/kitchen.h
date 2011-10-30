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

#pragma once
/*
* kitchen settings
*/
#include "dfhack/Export.h"
#include "dfhack/Module.h"
#include "dfhack/Types.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Items.h"
#include <dfhack/Core.h>
/**
 * \defgroup grp_kitchen Kitchen settings
 * @ingroup grp_modules
 */

namespace DFHack
{
namespace Kitchen
{
typedef uint8_t t_exclusionType;

const unsigned int seedLimit = 400; // a limit on the limits which can be placed on seeds
const t_itemSubtype organicSubtype = -1; // seems to fixed
const t_exclusionType cookingExclusion = 1; // seems to be fixed
const t_itemType limitType = 0; // used to store limit as an entry in the exclusion list. 0 = BAR
const t_itemSubtype limitSubtype = 0; // used to store limit as an entry in the exclusion list
const t_exclusionType limitExclusion = 4; // used to store limit as an entry in the exclusion list

/**
 * Kitchen exclusions manipulator class. Currently geared towards plants and seeds.
 * @ingroup grp_kitchen
 */
class Exclusions
{
public:
    /// ctor
    Exclusions(DFHack::Core& core_)
        : core(core_)
        , itemTypes       (*((std::vector<t_itemType     >*)(addr(core_,0))))
        , itemSubtypes    (*((std::vector<t_itemSubtype  >*)(addr(core_,1))))
        , materialTypes   (*((std::vector<t_materialType >*)(addr(core_,2))))
        , materialIndices (*((std::vector<t_materialIndex>*)(addr(core_,3))))
        , exclusionTypes  (*((std::vector<t_exclusionType>*)(addr(core_,4))))
    {
    };

    /// print the exclusion list, with the material index also translated into its token (for organics) - for debug really
    void debug_print()
    {
        core.con.print("Kitchen Exclusions\n");
        DFHack::Materials& materialsModule= *core.getMaterials();
        for(std::size_t i = 0; i < size(); ++i)
        {
            core.con.print("%2u: IT:%2i IS:%i MT:%3i MI:%2i ET:%i %s\n",
                           i,
                           itemTypes[i],
                           itemSubtypes[i],
                           materialTypes[i],
                           materialIndices[i],
                           exclusionTypes[i],
                           materialsModule.df_organic->at(materialIndices[i])->ID.c_str()
                          );
        }
        core.con.print("\n");
    };

    /// remove this material from the exclusion list if it is in it
    void allowPlantSeedCookery(t_materialIndex materialIndex)
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex = 0;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(materialIndices[i] == materialIndex
                   && (itemTypes[i] == DFHack::Items::SEEDS || itemTypes[i] == DFHack::Items::PLANT)
                   && exclusionTypes[i] == cookingExclusion
                )
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                itemTypes.erase(itemTypes.begin() + matchIndex);
                itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
                materialIndices.erase(materialIndices.begin() + matchIndex);
                materialTypes.erase( materialTypes.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };

    /// add this material to the exclusion list, if it is not already in it
    void denyPlantSeedCookery(t_materialIndex materialIndex)
    {
        Materials *mats = core.getMaterials();
        df_plant_type *type = mats->df_organic->at(materialIndex);
        bool SeedAlreadyIn = false;
        bool PlantAlreadyIn = false;
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(materialIndices[i] == materialIndex
               && exclusionTypes[i] == cookingExclusion)
            {
                if(itemTypes[i] == DFHack::Items::SEEDS)
                    SeedAlreadyIn = true;
                else if (itemTypes[i] == DFHack::Items::PLANT)
                    PlantAlreadyIn = true;
            }
        }
        if(!SeedAlreadyIn)
        {
            itemTypes.push_back(DFHack::Items::SEEDS);
            itemSubtypes.push_back(organicSubtype);
            materialTypes.push_back(type->material_type_seed);
            materialIndices.push_back(materialIndex);
            exclusionTypes.push_back(cookingExclusion);
        }
        if(!PlantAlreadyIn)
        {
            itemTypes.push_back(DFHack::Items::PLANT);
            itemSubtypes.push_back(organicSubtype);
            materialTypes.push_back(type->material_type_basic_mat);
            materialIndices.push_back(materialIndex);
            exclusionTypes.push_back(cookingExclusion);
        }
    };

    /// fills a map with info from the limit info storage entries in the exclusion list
    void fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap)
    {
        watchMap.clear();
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(itemTypes[i] == limitType && itemSubtypes[i] == limitSubtype && exclusionTypes[i] == limitExclusion)
            {
                watchMap[materialIndices[i]] = (unsigned int) materialTypes[i];
            }
        }
    };

    /// removes a limit info storage entry from the exclusion list if it's present
    void removeLimit(t_materialIndex materialIndex)
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex = 0;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(itemTypes[i] == limitType
                   && itemSubtypes[i] == limitSubtype
                   && materialIndices[i] == materialIndex
                   && exclusionTypes[i] == limitExclusion)
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                itemTypes.erase(itemTypes.begin() + matchIndex);
                itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
                materialTypes.erase( materialTypes.begin() + matchIndex);
                materialIndices.erase(materialIndices.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };

    /// add a limit info storage item to the exclusion list, or alters an existing one
    void setLimit(t_materialIndex materialIndex, unsigned int limit)
    {
        removeLimit(materialIndex);
        if(limit > seedLimit) limit = seedLimit;

        itemTypes.push_back(limitType);
        itemSubtypes.push_back(limitSubtype);
        materialIndices.push_back(materialIndex);
        materialTypes.push_back((t_materialType) (limit < seedLimit) ? limit : seedLimit);
        exclusionTypes.push_back(limitExclusion);
    };

    /// clears all limit info storage items from the exclusion list
    void clearLimits() 
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(itemTypes[i] == limitType
                   && itemSubtypes[i] == limitSubtype
                   && exclusionTypes[i] == limitExclusion)
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                itemTypes.erase(itemTypes.begin() + matchIndex);
                itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
                materialIndices.erase(materialIndices.begin() + matchIndex);
                materialTypes.erase( materialTypes.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };
private:
    DFHack::Core& core;
    std::vector<t_itemType>& itemTypes; // the item types vector of the kitchen exclusion list
    std::vector<t_itemSubtype>& itemSubtypes; // the item subtype vector of the kitchen exclusion list
    std::vector<t_materialType>& materialTypes; // the material subindex vector of the kitchen exclusion list
    std::vector<t_materialIndex>& materialIndices; // the material index vector of the kitchen exclusion list
    std::vector<t_exclusionType>& exclusionTypes; // the exclusion type vector of the kitchen excluions list
    std::size_t size() // the size of the exclusions vectors (they are all the same size - if not, there is a problem!)
    {
        return itemTypes.size();
    };
    static uint32_t addr(const DFHack::Core& core, int index)
    {
        static uint32_t start = core.vinfo->getAddress("kitchen_limits");
        return start + sizeof(std::vector<int>) * index;
    };
};

}
}
