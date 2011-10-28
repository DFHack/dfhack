// This does not work with Linux Dwarf Fortress
// With thanks to peterix for DFHack and Quietust for information http://www.bay12forums.com/smf/index.php?topic=91166.msg2605147#msg2605147

#include <map>
#include <string>
#include <vector>
#include "dfhack/Console.h"
#include "dfhack/Core.h"
#include "dfhack/Export.h"
#include "dfhack/PluginManager.h"
#include "dfhack/Process.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/Materials.h"

typedef int32_t t_materialIndex;
typedef int16_t t_material, t_itemType, t_itemSubtype;
typedef int8_t t_exclusionType;

const unsigned int seedLimit = 400; // a limit on the limits which can be placed on seeds
const t_itemSubtype organicSubtype = -1; // seems to fixed
const t_exclusionType cookingExclusion = 1; // seems to be fixed
const t_itemType limitType = 0; // used to store limit as an entry in the exclusion list. 0 = BAR
const t_itemSubtype limitSubtype = 0; // used to store limit as an entry in the exclusion list
const t_exclusionType limitExclusion = 4; // used to store limit as an entry in the exclusion list
const int buffer = 20; // seed number buffer - 20 is reasonable
bool running = false; // whether seedwatch is counting the seeds or not

std::map<std::string, std::string> abbreviations; // abbreviations for the standard plants

bool ignoreSeeds(DFHack::t_itemflags& f) // seeds with the following flags should not be counted
{
    return
        f.dump ||
        f.forbid ||
        f.garbage_colect ||
        f.hidden ||
        f.hostile ||
        f.on_fire ||
        f.rotten ||
        f.trader ||
        f.in_building ||
        f.in_job;
};
void updateCountAndSubindices(DFHack::Core& core, std::map<t_materialIndex, unsigned int>& seedCount, std::map<t_materialIndex, t_material>& seedSubindices, std::map<t_materialIndex, t_material>& plantSubindices) // fills seedCount, seedSubindices and plantSubindices
{
    DFHack::Items& itemsModule = *core.getItems();
    itemsModule.Start();
    std::vector<DFHack::df_item*> items;
    itemsModule.readItemVector(items);

    DFHack::df_item * item;
    std::size_t size = items.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        item = items[i];
        t_materialIndex materialIndex = item->getMaterialIndex();
        switch(item->getType())
        {
        case DFHack::Items::SEEDS:
            seedSubindices[materialIndex] = item->getMaterial();
            if(!ignoreSeeds(item->flags)) ++seedCount[materialIndex];
            break;
        case DFHack::Items::PLANT:
            plantSubindices[materialIndex] = item->getMaterial();
            break;
        }
    }

    itemsModule.Finish();
};
void printHelp(DFHack::Core& core) // prints help
{
    core.con.print(
        "Watches the numbers of plants and seeds available and enables/disables cooking automatically.\n"
        "Each plant type can be assigned a limit. If their number falls below,\n"
        "the plants and seeds of that type will be excluded from cookery.\n"
        "If the number rises above the limit + %i, then cooking will be allowed.\n"
        "Options:\n"
        "seedwatch all   - Adds all plants from the abbreviation list to the watch list.\n"
        "seedwatch start - Start watching.\n"
        "seedwatch stop  - Stop watching.\n"
        "seedwatch info  - Display whether seedwatch is watching, and the watch list.\n"
        "seedwatch clear - Clears the watch list.\n\n"
        , buffer
    );
    if(!abbreviations.empty())
    {
        core.con.print("You can use these abbreviations for the plant tokens:\n");
        for(std::map<std::string, std::string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
        {
            core.con.print("%s -> %s\n", i->first.c_str(), i->second.c_str());
        }
    }
    core.con.print(
        "Examples:\n"
        "seedwatch MUSHROOM_HELMET_PLUMP 30\n"
        "  add MUSHROOM_HELMET_PLUMP to the watch list, limit = 30\n"
        "seedwatch MUSHROOM_HELMET_PLUMP\n"
        "  removes MUSHROOM_HELMET_PLUMP from the watch list.\n"
        "seedwatch ph 30\n"
        "  is the same as 'seedwatch MUSHROOM_HELMET_PLUMP 30'\n"
        "seedwatch all 30\n"
        "  adds all plants from the abbreviation list to the watch list, the limit being 30.\n"
    );
};

// searches abbreviations, returns expansion if so, returns original if not
std::string searchAbbreviations(std::string in) 
{
    if(abbreviations.count(in) > 0)
    {
        return abbreviations[in];
    }
    else
    {
        return in;
    }
};

// helps to organise access to the cooking exclusions
class t_kitchenExclusions
{
public:
    t_kitchenExclusions(DFHack::Core& core_) // constructor
        : core(core_)
        , itemTypes       (*((std::vector<t_itemType     >*)(addr(core_,0))))
        , itemSubtypes    (*((std::vector<t_itemSubtype  >*)(addr(core_,1))))
        , materials       (*((std::vector<t_material     >*)(addr(core_,2))))
        , materialIndices (*((std::vector<t_materialIndex>*)(addr(core_,3))))
        , exclusionTypes  (*((std::vector<t_exclusionType>*)(addr(core_,4))))
    {
    };
    void print() // print the exclusion list, with the material index also translated into its token (for organics) - for debug really
    {
        core.con.print("Kitchen Exclusions\n");
        DFHack::Materials& materialsModule= *core.getMaterials();
        materialsModule.ReadOrganicMaterials();
        for(std::size_t i = 0; i < size(); ++i)
        {
            core.con.print("%2u: IT:%2i IS:%i MI:%2i MS:%3i ET:%i %s\n", i, itemTypes[i], itemSubtypes[i], materialIndices[i], materials[i], exclusionTypes[i], materialsModule.organic[materialIndices[i]].id.c_str());
        }
        core.con.print("\n");
    };
    void allowCookery(t_materialIndex materialIndex) // remove this material from the exclusion list if it is in it
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
                materials.erase( materials.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };

    // add this material to the exclusion list, if it is not already in it
    void denyCookery(t_materialIndex materialIndex, std::map<t_materialIndex, t_material>& seedMaterials, std::map<t_materialIndex, t_material>& plantMaterials)
    {
        if( seedMaterials.count(materialIndex) > 0)
        {
            bool alreadyIn = false;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(materialIndices[i] == materialIndex
                   && itemTypes[i] == DFHack::Items::SEEDS
                   && exclusionTypes[i] == cookingExclusion)
                {
                    alreadyIn = true;
                }
            }
            if(!alreadyIn)
            {
                itemTypes.push_back(DFHack::Items::SEEDS);
                itemSubtypes.push_back(organicSubtype);
                materials.push_back( seedMaterials[materialIndex]);
                materialIndices.push_back(materialIndex);
                exclusionTypes.push_back(cookingExclusion);
            }
        }
        if( plantMaterials.count(materialIndex) > 0)
        {
            bool alreadyIn = false;
            for(std::size_t i = 0; i < size(); ++i)
            {
                if(materialIndices[i] == materialIndex
                   && itemTypes[i] == DFHack::Items::PLANT
                   && exclusionTypes[i] == cookingExclusion)
                {
                    alreadyIn = true;
                }
            }
            if(!alreadyIn)
            {
                itemTypes.push_back(DFHack::Items::PLANT);
                itemSubtypes.push_back(organicSubtype);
                materials.push_back(plantMaterials[materialIndex]);
                materialIndices.push_back(materialIndex);
                exclusionTypes.push_back(cookingExclusion);
            }
        }
    };

    // fills a map with info from the limit info storage entries in the exclusion list
    void fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap)
    {
        watchMap.clear();
        for(std::size_t i = 0; i < size(); ++i)
        {
            if(itemTypes[i] == limitType && itemSubtypes[i] == limitSubtype && exclusionTypes[i] == limitExclusion)
            {
                watchMap[materialIndices[i]] = (unsigned int) materials[i];
            }
        }
    };

    // removes a limit info storage entry from the exclusion list if it is in it
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
                materials.erase(materials.begin() + matchIndex);
                materialIndices.erase(materialIndices.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };
    // add a limit info storage item to the exclusion list, or alters an existing one
    void setLimit(t_materialIndex materialIndex, unsigned int limit) 
    {
        removeLimit(materialIndex);
        if(limit > seedLimit) limit = seedLimit;

        itemTypes.push_back(limitType);
        itemSubtypes.push_back(limitSubtype);
        materialIndices.push_back(materialIndex);
        materials.push_back((t_material) (limit < seedLimit) ? limit : seedLimit);
        exclusionTypes.push_back(limitExclusion);
    };
    void clearLimits() // clears all limit info storage items from the exclusion list
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
                materials.erase( materials.begin() + matchIndex);
                exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
            }
        } while(match);
    };
private:
    DFHack::Core& core;
    std::vector<t_itemType>& itemTypes; // the item types vector of the kitchen exclusion list
    std::vector<t_itemSubtype>& itemSubtypes; // the item subtype vector of the kitchen exclusion list
    std::vector<t_material>& materials; // the material subindex vector of the kitchen exclusion list
    std::vector<t_materialIndex>& materialIndices; // the material index vector of the kitchen exclusion list
    std::vector<t_exclusionType>& exclusionTypes; // the exclusion type vector of the kitchen excluions list
    std::size_t size() // the size of the exclusions vectors (they are all the same size - if not, there is a problem!)
    {
        return itemTypes.size();
    };
    //FIXME: ugly, ugly hack. Use memory.xml instead.
    static uint32_t addr(const DFHack::Core& core, int index)
    {
        #ifdef LINUX_BUILD
            return 0x93f2b00 + 0xC * index;
        #else
            return core.p->getBase() + 0x014F0BB4 - 0x400000 + 0x10 * index;
        #endif
    };
};

DFhackCExport DFHack::command_result df_seedwatch(DFHack::Core* pCore, std::vector<std::string>& parameters)
{
    DFHack::Core& core = *pCore;
    if(!core.isValid())
    {
        return DFHack::CR_FAILURE;
    }
    core.Suspend();

    DFHack::Materials& materialsModule = *core.getMaterials();
    materialsModule.ReadOrganicMaterials();
    std::vector<DFHack::t_matgloss>& organics = materialsModule.organic;

    std::map<std::string, t_materialIndex> materialsReverser;
    for(std::size_t i = 0; i < organics.size(); ++i)
    {
        materialsReverser[organics[i].id] = i;
    }

    std::string par;
    int limit;
    switch(parameters.size())
    {
    case 0:
        printHelp(core);
        break;
    case 1:
        par = parameters[0];
        if(par == "help") printHelp(core);
        else if(par == "?") printHelp(core);
        else if(par == "start")
        {
            running = true;
            core.con.print("seedwatch supervision started.\n");
        }
        else if(par == "stop")
        {
            running = false;
            core.con.print("seedwatch supervision stopped.\n");
        }
        else if(par == "clear")
        {
            t_kitchenExclusions kitchenExclusions(core);
            kitchenExclusions.clearLimits();
            core.con.print("seedwatch watchlist cleared\n");
        }
        else if(par == "info")
        {
            core.con.print("seedwatch Info:\n");
            if(running)
            {
                core.con.print("seedwatch is supervising.  Use 'seedwatch stop' to stop supervision.\n");
            }
            else
            {
                core.con.print("seedwatch is not supervising.  Use 'seedwatch start' to start supervision.\n");
            }
            t_kitchenExclusions kitchenExclusions(core);
            std::map<t_materialIndex, unsigned int> watchMap;
            kitchenExclusions.fillWatchMap(watchMap);
            if(watchMap.empty())
            {
                core.con.print("The watch list is empty.\n");
            }
            else
            {
                core.con.print("The watch list is:\n");
                for(std::map<t_materialIndex, unsigned int>::const_iterator i = watchMap.begin(); i != watchMap.end(); ++i)
                {
                    core.con.print("%s : %u\n", organics[i->first].id.c_str(), i->second);
                }
            }
        }
        else if(par == "debug")
        {
            t_kitchenExclusions kitchenExclusions(core);
            std::map<t_materialIndex, unsigned int> watchMap;
            kitchenExclusions.fillWatchMap(watchMap);
            kitchenExclusions.print();
        }
        else
        {
            std::string token = searchAbbreviations(par);
            if(materialsReverser.count(token) > 0)
            {
                t_kitchenExclusions kitchenExclusions(core);
                kitchenExclusions.removeLimit(materialsReverser[token]);
                core.con.print("%s is not being watched\n", token.c_str());
            }
            else
            {
                core.con.print("%s has not been found as a material.\n", token.c_str());
            }
        }
        break;
    case 2:
        limit = atoi(parameters[1].c_str());
        if(limit < 0) limit = 0;
        if(parameters[0] == "all")
        {
            for(std::map<std::string, std::string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
            {
                t_kitchenExclusions kitchenExclusions(core);
                if(materialsReverser.count(i->second) > 0) kitchenExclusions.setLimit(materialsReverser[i->second], limit);
            }
        }
        else
        {
            std::string token = searchAbbreviations(parameters[0]);
            if(materialsReverser.count(token) > 0)
            {
                t_kitchenExclusions kitchenExclusions(core);
                kitchenExclusions.setLimit(materialsReverser[token], limit);
                core.con.print("%s is being watched.\n", token.c_str());
            }
            else
            {
                core.con.print("%s has not been found as a material.\n", token.c_str());
            }
        }
        break;
    default:
        printHelp(core);
        break;
    }

    core.Resume();
    return DFHack::CR_OK;
}

DFhackCExport const char* plugin_name(void)
{
    return "seedwatch";
}

DFhackCExport DFHack::command_result plugin_init(DFHack::Core* pCore, std::vector<DFHack::PluginCommand>& commands)
{
    commands.clear();
    commands.push_back(DFHack::PluginCommand("seedwatch", "Switches cookery based on quantity of seeds, to keep reserves", df_seedwatch));
    // fill in the abbreviations map, with abbreviations for the standard plants
    abbreviations["bs"] = "SLIVER_BARB";
    abbreviations["bt"] = "TUBER_BLOATED";
    abbreviations["bw"] = "WEED_BLADE";
    abbreviations["cw"] = "GRASS_WHEAT_CAVE";
    abbreviations["dc"] = "MUSHROOM_CUP_DIMPLE";
    abbreviations["fb"] = "BERRIES_FISHER";
    abbreviations["hr"] = "ROOT_HIDE";
    abbreviations["kb"] = "BULB_KOBOLD";
    abbreviations["lg"] = "GRASS_LONGLAND";
    abbreviations["mr"] = "ROOT_MUCK";
    abbreviations["pb"] = "BERRIES_PRICKLE";
    abbreviations["ph"] = "MUSHROOM_HELMET_PLUMP";
    abbreviations["pt"] = "GRASS_TAIL_PIG";
    abbreviations["qb"] = "BUSH_QUARRY";
    abbreviations["rr"] = "REED_ROPE";
    abbreviations["rw"] = "WEED_RAT";
    abbreviations["sb"] = "BERRY_SUN";
    abbreviations["sp"] = "POD_SWEET";
    abbreviations["vh"] = "HERB_VALLEY";
    abbreviations["ws"] = "BERRIES_STRAW_WILD";
    abbreviations["wv"] = "VINE_WHIP";
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_onupdate(DFHack::Core* pCore)
{
    if(running)
    {
        DFHack::Core& core = *pCore;
        std::map<t_materialIndex, unsigned int> seedCount; // the number of seeds
        std::map<t_materialIndex, t_material> seedSubindices; // holds information needed to add entries to the cooking exclusions
        std::map<t_materialIndex, t_material> plantSubindices; // holds information need to add entries to the cooking exclusions
        updateCountAndSubindices(core, seedCount, seedSubindices, plantSubindices);
        t_kitchenExclusions kitchenExclusions(core);
        std::map<t_materialIndex, unsigned int> watchMap;
        kitchenExclusions.fillWatchMap(watchMap);
        for(std::map<t_materialIndex, unsigned int>::const_iterator i = watchMap.begin(); i != watchMap.end(); ++i)
        {
            if(seedCount[i->first] <= i->second)
            {
                kitchenExclusions.denyCookery(i->first, seedSubindices, plantSubindices);
            }
            else if(i->second + buffer < seedCount[i->first])
            {
                kitchenExclusions.allowCookery(i->first);
            }
        }
    }
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_shutdown(DFHack::Core* pCore)
{
    return DFHack::CR_OK;
}
