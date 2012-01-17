
#include "Internal.h"

#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <map>
#include <set>
using namespace std;

#include "Types.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/kitchen.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Virtual.h"
#include "df/item_type.h"

namespace DFHack
{
namespace Kitchen
{
    class Exclusions::Private
    {
    public:
        Private (DFHack::Core& core_)
            : core(core_)
            , itemTypes       (*((std::vector<t_itemType     >*)(addr(core_,0))))
            , itemSubtypes    (*((std::vector<t_itemSubtype  >*)(addr(core_,1))))
            , materialTypes   (*((std::vector<t_materialType >*)(addr(core_,2))))
            , materialIndices (*((std::vector<t_materialIndex>*)(addr(core_,3))))
            , exclusionTypes  (*((std::vector<t_exclusionType>*)(addr(core_,4))))
        {
        };
        DFHack::Core& core;
        std::vector<t_itemType>& itemTypes; // the item types vector of the kitchen exclusion list
        std::vector<t_itemSubtype>& itemSubtypes; // the item subtype vector of the kitchen exclusion list
        std::vector<t_materialType>& materialTypes; // the material subindex vector of the kitchen exclusion list
        std::vector<t_materialIndex>& materialIndices; // the material index vector of the kitchen exclusion list
        std::vector<t_exclusionType>& exclusionTypes; // the exclusion type vector of the kitchen excluions list

        static void * addr(const DFHack::Core& core, int index)
        {
            static char * start = core.vinfo->getAddress("kitchen_limits");
            return start + sizeof(std::vector<int>) * index;
        };
    };

    Exclusions::Exclusions(Core & c)
    {
        d = new Private(c);
    };

    Exclusions::~Exclusions()
    {
        delete d;
    };

    void Exclusions::debug_print() const
    {
        d->core.con.print("Kitchen Exclusions\n");
        Materials& materialsModule= *d->core.getMaterials();
        for(std::size_t i = 0; i < size(); ++i)
        {
            d->core.con.print("%2u: IT:%2i IS:%i MT:%3i MI:%2i ET:%i %s\n",
                           i,
                           d->itemTypes[i],
                           d->itemSubtypes[i],
                           d->materialTypes[i],
                           d->materialIndices[i],
                           d->exclusionTypes[i],
                           materialsModule.df_organic->at(d->materialIndices[i])->ID.c_str()
            );
        }
        d->core.con.print("\n");
    }

    void Exclusions::allowPlantSeedCookery(t_materialIndex materialIndex)
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex = 0;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(d->materialIndices[i] == materialIndex
                   && (d->itemTypes[i] == df::item_type::SEEDS || d->itemTypes[i] == df::item_type::PLANT)
                   && d->exclusionTypes[i] == cookingExclusion
                )
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                d->itemTypes.erase(d->itemTypes.begin() + matchIndex);
                d->itemSubtypes.erase(d->itemSubtypes.begin() + matchIndex);
                d->materialIndices.erase(d->materialIndices.begin() + matchIndex);
                d->materialTypes.erase(d->materialTypes.begin() + matchIndex);
                d->exclusionTypes.erase(d->exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };

    void Exclusions::denyPlantSeedCookery(t_materialIndex materialIndex)
    {
        Materials *mats = d->core.getMaterials();
        df_plant_type *type = mats->df_organic->at(materialIndex);
        bool SeedAlreadyIn = false;
        bool PlantAlreadyIn = false;
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(d->materialIndices[i] == materialIndex
               && d->exclusionTypes[i] == cookingExclusion)
            {
                if(d->itemTypes[i] == df::item_type::SEEDS)
                    SeedAlreadyIn = true;
                else if (d->itemTypes[i] == df::item_type::PLANT)
                    PlantAlreadyIn = true;
            }
        }
        if(!SeedAlreadyIn)
        {
            d->itemTypes.push_back(df::item_type::SEEDS);
            d->itemSubtypes.push_back(organicSubtype);
            d->materialTypes.push_back(type->material_type_seed);
            d->materialIndices.push_back(materialIndex);
            d->exclusionTypes.push_back(cookingExclusion);
        }
        if(!PlantAlreadyIn)
        {
            d->itemTypes.push_back(df::item_type::PLANT);
            d->itemSubtypes.push_back(organicSubtype);
            d->materialTypes.push_back(type->material_type_basic_mat);
            d->materialIndices.push_back(materialIndex);
            d->exclusionTypes.push_back(cookingExclusion);
        }
    };

    void Exclusions::fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap) const
    {
        watchMap.clear();
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(d->itemTypes[i] == limitType && d->itemSubtypes[i] == limitSubtype && d->exclusionTypes[i] == limitExclusion)
            {
                watchMap[d->materialIndices[i]] = (unsigned int) d->materialTypes[i];
            }
        }
    };

    void Exclusions::removeLimit(t_materialIndex materialIndex)
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex = 0;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(d->itemTypes[i] == limitType
                   && d->itemSubtypes[i] == limitSubtype
                   && d->materialIndices[i] == materialIndex
                   && d->exclusionTypes[i] == limitExclusion)
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                d->itemTypes.erase(d->itemTypes.begin() + matchIndex);
                d->itemSubtypes.erase(d->itemSubtypes.begin() + matchIndex);
                d->materialTypes.erase(d->materialTypes.begin() + matchIndex);
                d->materialIndices.erase(d->materialIndices.begin() + matchIndex);
                d->exclusionTypes.erase(d->exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };

    void Exclusions::setLimit(t_materialIndex materialIndex, unsigned int limit)
    {
        removeLimit(materialIndex);
        if(limit > seedLimit)
        {
            limit = seedLimit;
        }
        d->itemTypes.push_back(limitType);
        d->itemSubtypes.push_back(limitSubtype);
        d->materialIndices.push_back(materialIndex);
        d->materialTypes.push_back((t_materialType) (limit < seedLimit) ? limit : seedLimit);
        d->exclusionTypes.push_back(limitExclusion);
    };

    void Exclusions::clearLimits()
    {
        bool match = false;
        do
        {
            match = false;
            std::size_t matchIndex;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(d->itemTypes[i] == limitType
                   && d->itemSubtypes[i] == limitSubtype
                   && d->exclusionTypes[i] == limitExclusion)
                {
                    match = true;
                    matchIndex = i;
                }
            }
            if(match)
            {
                d->itemTypes.erase(d->itemTypes.begin() + matchIndex);
                d->itemSubtypes.erase(d->itemSubtypes.begin() + matchIndex);
                d->materialIndices.erase(d->materialIndices.begin() + matchIndex);
                d->materialTypes.erase(d->materialTypes.begin() + matchIndex);
                d->exclusionTypes.erase(d->exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };
    size_t Exclusions::size() const
    {
        return d->itemTypes.size();
    };
}
};