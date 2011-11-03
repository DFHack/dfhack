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
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/World.h"
#include "dfhack/modules/kitchen.h"
#include <dfhack/VersionInfo.h>

using DFHack::t_materialType;
using DFHack::t_materialIndex;

const int buffer = 20; // seed number buffer - 20 is reasonable
bool running = false; // whether seedwatch is counting the seeds or not

// abbreviations for the standard plants
std::map<std::string, std::string> abbreviations;

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

void printHelp(DFHack::Core& core) // prints help
{
    core.con.print(
        "Watches the numbers of seeds available and enables/disables seed and plant cooking.\n"
        "Each plant type can be assigned a limit. If their number falls below,\n"
        "the plants and seeds of that type will be excluded from cookery.\n"
        "If the number rises above the limit + %i, then cooking will be allowed.\n", buffer
    );
    core.con.printerr(
        "The plugin needs a fortress to be loaded and will deactivate automatically otherwise.\n"
        "You have to reactivate with 'seedwatch start' after you load the game.\n"
    );
    core.con.print(
        "Options:\n"
        "seedwatch all   - Adds all plants from the abbreviation list to the watch list.\n"
        "seedwatch start - Start watching.\n"
        "seedwatch stop  - Stop watching.\n"
        "seedwatch info  - Display whether seedwatch is watching, and the watch list.\n"
        "seedwatch clear - Clears the watch list.\n\n"
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

DFhackCExport DFHack::command_result df_seedwatch(DFHack::Core* pCore, std::vector<std::string>& parameters)
{
    DFHack::Core& core = *pCore;
    if(!core.isValid())
    {
        return DFHack::CR_FAILURE;
    }
    core.Suspend();

    DFHack::Materials& materialsModule = *core.getMaterials();
    std::vector<DFHack::t_matgloss> organics;
    materialsModule.CopyOrganicMaterials(organics);

    std::map<std::string, t_materialIndex> materialsReverser;
    for(std::size_t i = 0; i < organics.size(); ++i)
    {
        materialsReverser[organics[i].id] = i;
    }

    DFHack::World *w = core.getWorld();
    DFHack::t_gamemodes gm;
    w->ReadGameMode(gm);// FIXME: check return value

    // if game mode isn't fortress mode
    if(gm.g_mode != DFHack::GAMEMODE_DWARF || gm.g_type != DFHack::GAMETYPE_DWARF_MAIN)
    {
        // just print the help
        printHelp(core);
        core.Resume();
        return DFHack::CR_OK;
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
            DFHack::Kitchen::Exclusions kitchenExclusions(core);
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
            DFHack::Kitchen::Exclusions kitchenExclusions(core);
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
            DFHack::Kitchen::Exclusions kitchenExclusions(core);
            std::map<t_materialIndex, unsigned int> watchMap;
            kitchenExclusions.fillWatchMap(watchMap);
            kitchenExclusions.debug_print();
        }
        /*
        else if(par == "dumpmaps")
        {
            
            core.con.print("Plants:\n");
            for(auto i = plantMaterialTypes.begin(); i != plantMaterialTypes.end(); i++)
            {
                auto t = materialsModule.df_organic->at(i->first);
                core.con.print("%s : %u %u\n", organics[i->first].id.c_str(), i->second, t->material_basic_mat);
            }
            core.con.print("Seeds:\n");
            for(auto i = seedMaterialTypes.begin(); i != seedMaterialTypes.end(); i++)
            {
                auto t = materialsModule.df_organic->at(i->first);
                core.con.print("%s : %u %u\n", organics[i->first].id.c_str(), i->second, t->material_seed);
            }
        }
        */
        else
        {
            std::string token = searchAbbreviations(par);
            if(materialsReverser.count(token) > 0)
            {
                DFHack::Kitchen::Exclusions kitchenExclusions(core);
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
                DFHack::Kitchen::Exclusions kitchenExclusions(core);
                if(materialsReverser.count(i->second) > 0) kitchenExclusions.setLimit(materialsReverser[i->second], limit);
            }
        }
        else
        {
            std::string token = searchAbbreviations(parameters[0]);
            if(materialsReverser.count(token) > 0)
            {
                DFHack::Kitchen::Exclusions kitchenExclusions(core);
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
        DFHack::World *w = core.getWorld();
        DFHack::t_gamemodes gm;
        w->ReadGameMode(gm);// FIXME: check return value
        // if game mode isn't fortress mode
        if(gm.g_mode != DFHack::GAMEMODE_DWARF || gm.g_type != DFHack::GAMETYPE_DWARF_MAIN)
        {
            // stop running.
            running = false;
            core.con.printerr("seedwatch deactivated due to game mode switch\n");
            return DFHack::CR_OK;
        }
        // this is dwarf mode, continue
        std::map<t_materialIndex, unsigned int> seedCount; // the number of seeds
        DFHack::Items& itemsModule = *core.getItems();
        itemsModule.Start();
        std::vector<DFHack::df_item*> items;
        itemsModule.readItemVector(items);
        DFHack::df_item * item;
        // count all seeds and plants by RAW material
        for(std::size_t i = 0; i < items.size(); ++i)
        {
            item = items[i];
            t_materialIndex materialIndex = item->getMaterialIndex();
            switch(item->getType())
            {
            case DFHack::Items::SEEDS:
                if(!ignoreSeeds(item->flags)) ++seedCount[materialIndex];
                break;
            case DFHack::Items::PLANT:
                break;
            }
        }
        itemsModule.Finish();

        DFHack::Kitchen::Exclusions kitchenExclusions(core);
        std::map<t_materialIndex, unsigned int> watchMap;
        kitchenExclusions.fillWatchMap(watchMap);
        for(auto i = watchMap.begin(); i != watchMap.end(); ++i)
        {
            if(seedCount[i->first] <= i->second)
            {
                kitchenExclusions.denyPlantSeedCookery(i->first);
            }
            else if(i->second + buffer < seedCount[i->first])
            {
                kitchenExclusions.allowPlantSeedCookery(i->first);
            }
        }
    }
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_shutdown(DFHack::Core* pCore)
{
    return DFHack::CR_OK;
}
