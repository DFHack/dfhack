// This does not work with Linux Dwarf Fortress
// With thanks to peterix for DFHack and Quietust for information http://www.bay12forums.com/smf/index.php?topic=91166.msg2605147#msg2605147

#include <map>
#include <string>
#include <vector>
#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/World.h"
#include "modules/kitchen.h"
#include "VersionInfo.h"
#include "df/world.h"
#include "df/plant_raw.h"
#include "df/item_flags.h"
#include "df/items_other_id.h"

using namespace std;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("seedwatch");
DFHACK_PLUGIN_IS_ENABLED(running); // whether seedwatch is counting the seeds or not

REQUIRE_GLOBAL(world);

const int buffer = 20; // seed number buffer - 20 is reasonable

// abbreviations for the standard plants
map<string, string> abbreviations;

bool ignoreSeeds(df::item_flags& f) // seeds with the following flags should not be counted
{
    return
        f.bits.dump ||
        f.bits.forbid ||
        f.bits.garbage_collect ||
        f.bits.hidden ||
        f.bits.hostile ||
        f.bits.on_fire ||
        f.bits.rotten ||
        f.bits.trader ||
        f.bits.in_building ||
        f.bits.in_job;
};

void printHelp(color_ostream &out) // prints help
{
    out.print(
        "Watches the numbers of seeds available and enables/disables seed and plant cooking.\n"
        "Each plant type can be assigned a limit. If their number falls below,\n"
        "the plants and seeds of that type will be excluded from cookery.\n"
        "If the number rises above the limit + %i, then cooking will be allowed.\n", buffer
        );
    out.printerr(
        "The plugin needs a fortress to be loaded and will deactivate automatically otherwise.\n"
        "You have to reactivate with 'seedwatch start' after you load the game.\n"
        );
    out.print(
        "Options:\n"
        "seedwatch all   - Adds all plants from the abbreviation list to the watch list.\n"
        "seedwatch start - Start watching.\n"
        "seedwatch stop  - Stop watching.\n"
        "seedwatch info  - Display whether seedwatch is watching, and the watch list.\n"
        "seedwatch clear - Clears the watch list.\n\n"
        );
    if(!abbreviations.empty())
    {
        out.print("You can use these abbreviations for the plant tokens:\n");
        for(map<string, string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
        {
            out.print("%s -> %s\n", i->first.c_str(), i->second.c_str());
        }
    }
    out.print(
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
string searchAbbreviations(string in)
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

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    running = enable;
    return CR_OK;
}

command_result df_seedwatch(color_ostream &out, vector<string>& parameters)
{
    CoreSuspender suspend;

    map<string, t_materialIndex> materialsReverser;
    for(size_t i = 0; i < world->raws.plants.all.size(); ++i)
    {
        materialsReverser[world->raws.plants.all[i]->id] = i;
    }

    t_gamemodes gm;
    World::ReadGameMode(gm);// FIXME: check return value

    // if game mode isn't fortress mode
    if(gm.g_mode != game_mode::DWARF || !World::isFortressMode(gm.g_type))
    {
        // just print the help
        printHelp(out);
        return CR_OK;
    }

    string par;
    int limit;
    switch(parameters.size())
    {
    case 0:
        printHelp(out);
        return CR_WRONG_USAGE;

    case 1:
        par = parameters[0];
        if ((par == "help") || (par == "?"))
        {
            printHelp(out);
            return CR_WRONG_USAGE;
        }
        else if(par == "start")
        {
            running = true;
            out.print("seedwatch supervision started.\n");
        }
        else if(par == "stop")
        {
            running = false;
            out.print("seedwatch supervision stopped.\n");
        }
        else if(par == "clear")
        {
            Kitchen::clearLimits();
            out.print("seedwatch watchlist cleared\n");
        }
        else if(par == "info")
        {
            out.print("seedwatch Info:\n");
            if(running)
            {
                out.print("seedwatch is supervising.  Use 'seedwatch stop' to stop supervision.\n");
            }
            else
            {
                out.print("seedwatch is not supervising.  Use 'seedwatch start' to start supervision.\n");
            }
            map<t_materialIndex, unsigned int> watchMap;
            Kitchen::fillWatchMap(watchMap);
            if(watchMap.empty())
            {
                out.print("The watch list is empty.\n");
            }
            else
            {
                out.print("The watch list is:\n");
                for(map<t_materialIndex, unsigned int>::const_iterator i = watchMap.begin(); i != watchMap.end(); ++i)
                {
                    out.print("%s : %u\n", world->raws.plants.all[i->first]->id.c_str(), i->second);
                }
            }
        }
        else if(par == "debug")
        {
            map<t_materialIndex, unsigned int> watchMap;
            Kitchen::fillWatchMap(watchMap);
            Kitchen::debug_print(out);
        }
        /*
        else if(par == "dumpmaps")
        {
            out.print("Plants:\n");
            for(auto i = plantMaterialTypes.begin(); i != plantMaterialTypes.end(); i++)
            {
                auto t = materialsModule.df_organic->at(i->first);
                out.print("%s : %u %u\n", organics[i->first].id.c_str(), i->second, t->material_basic_mat);
            }
            out.print("Seeds:\n");
            for(auto i = seedMaterialTypes.begin(); i != seedMaterialTypes.end(); i++)
            {
                auto t = materialsModule.df_organic->at(i->first);
                out.print("%s : %u %u\n", organics[i->first].id.c_str(), i->second, t->material_seed);
            }
        }
        */
        else
        {
            string token = searchAbbreviations(par);
            if(materialsReverser.count(token) > 0)
            {
                Kitchen::removeLimit(materialsReverser[token]);
                out.print("%s is not being watched\n", token.c_str());
            }
            else
            {
                out.print("%s has not been found as a material.\n", token.c_str());
            }
        }
        break;
    case 2:
        limit = atoi(parameters[1].c_str());
        if(limit < 0) limit = 0;
        if(parameters[0] == "all")
        {
            for(map<string, string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
            {
                if(materialsReverser.count(i->second) > 0) Kitchen::setLimit(materialsReverser[i->second], limit);
            }
        }
        else
        {
            string token = searchAbbreviations(parameters[0]);
            if(materialsReverser.count(token) > 0)
            {
                Kitchen::setLimit(materialsReverser[token], limit);
                out.print("%s is being watched.\n", token.c_str());
            }
            else
            {
                out.print("%s has not been found as a material.\n", token.c_str());
            }
        }
        break;
    default:
        printHelp(out);
        return CR_WRONG_USAGE;
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand>& commands)
{
    commands.push_back(PluginCommand("seedwatch", "Toggles seed cooking based on quantity available", df_seedwatch));
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
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
    case SC_MAP_UNLOADED:
        if (running)
            out.printerr("seedwatch deactivated due to game load/unload\n");
        running = false;
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (running)
    {
        // reduce processing rate
        static int counter = 0;
        if (++counter < 500)
            return CR_OK;
        counter = 0;

        t_gamemodes gm;
        World::ReadGameMode(gm);// FIXME: check return value
        // if game mode isn't fortress mode
        if(gm.g_mode != game_mode::DWARF || !World::isFortressMode(gm.g_type))
        {
            // stop running.
            running = false;
            out.printerr("seedwatch deactivated due to game mode switch\n");
            return CR_OK;
        }
        // this is dwarf mode, continue
        map<t_materialIndex, unsigned int> seedCount; // the number of seeds

        // count all seeds and plants by RAW material
        for(size_t i = 0; i < world->items.other[items_other_id::SEEDS].size(); ++i)
        {
            df::item * item = world->items.other[items_other_id::SEEDS][i];
            t_materialIndex materialIndex = item->getMaterialIndex();
            if(!ignoreSeeds(item->flags)) ++seedCount[materialIndex];
        }

        map<t_materialIndex, unsigned int> watchMap;
        Kitchen::fillWatchMap(watchMap);
        for(auto i = watchMap.begin(); i != watchMap.end(); ++i)
        {
            if(seedCount[i->first] <= i->second)
            {
                Kitchen::denyPlantSeedCookery(i->first);
            }
            else if(i->second + buffer < seedCount[i->first])
            {
                Kitchen::allowPlantSeedCookery(i->first);
            }
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(Core* pCore)
{
    return CR_OK;
}
