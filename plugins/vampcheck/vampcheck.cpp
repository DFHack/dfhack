// vampcheck plugin
//
// check single tile or whole map/world for vampires by checking if a valid curse date (!=-1) is set
// if a cursor is active only the selected tile will be observed
// without cursor the whole map will be checked
// by default vampires will be only counted
//
// Options:
//   detail  - print full name, date of birth, date of curse (vamp might use fake identity, though)
//   nick    - set nickname to 'CURSED' (does not always show up ingame, some vamps don't like nicknames)

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include <modules/Translation.h>
#include "modules/Gui.h"
#include "MiscUtils.h"

#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/assumed_identity.h"
#include "df/language_name.h"
#include "df/world.h"
#include "df/world_raws.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;

command_result vampcheck (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("vampcheck");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("vampcheck",
        "Checks for curses (vampires).",
        vampcheck, false ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


// code for setting nicknames is borrowed from rename.cpp
// will not work in all cases, some vampires don't like to get nicks

static void set_nickname(df::language_name *name, std::string nick)
{
    if (!name->has_name)
    {
        *name = df::language_name();

        name->language = 0;
        name->has_name = true;
    }

    name->nickname = nick;
}

void setUnitNickname(df::unit *unit, const std::string &nick)
{
    // There are >=3 copies of the name, and the one
    // in the unit is not the authoritative one.
    // This is the reason why military units often
    // lose nicknames set from Dwarf Therapist.
    set_nickname(&unit->name, nick);

    if (unit->status.current_soul)
        set_nickname(&unit->status.current_soul->name, nick);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);
    if (figure)
    {
        set_nickname(&figure->name, nick);

        // v0.34.01: added the vampire's assumed identity
        if (figure->info && figure->info->reputation)
        {
            auto identity = df::assumed_identity::find(figure->info->reputation->cur_identity);

            if (identity)
            {
                auto id_hfig = df::historical_figure::find(identity->histfig_id);

                if (id_hfig)
                {
                    // Even DF doesn't do this bit, because it's apparently
                    // only used for demons masquerading as gods, so you
                    // can't ever change their nickname in-game.
                    set_nickname(&id_hfig->name, nick);
                }
                else
                    set_nickname(&identity->name, nick);
            }
        }
    }
}

command_result vampcheck (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);

    // check command options
    // by default cursed creatures are only counted
    bool giveDetails = false;
    bool giveNick = false;
    size_t cursecount = 0;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print(  "  Search for vampires. With map cursor active only the current tile\n"
                        "  will be checked. Without cursor the whole map/world will be scanned.\n"
                        "  By default cursed creatures are only counted to make it more interesting.\n"
                        "Options:\n"
                        "  detail - show details (name and age shown ingame might differ)\n"
                        "  nick   - try to set nickname to CURSED (does not always work)\n"
                        );
            return CR_OK;
        }
        if(parameters[i] == "detail")
            giveDetails = true;
        if(parameters[i] == "nick")
            giveNick = true;
    }

    // check whole map if no cursor is active
    bool checkWholeMap = false;
    if(cursorX == -30000)
    {
        out.print("No cursor; will check all units on the map.\n");
        checkWholeMap = true;
    }

    for(size_t i = 0; i < world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        // bail out if we have a map cursor and creature is not at that specific position
        if ( !checkWholeMap && (unit->pos.x != cursorX || unit->pos.y != cursorY || unit->pos.z != cursorZ) )
        {
            continue;
        }

        // non-vampires have curse_year == -1
        if(unit->relations.curse_year != -1)
        {
            cursecount++;
            
            if(giveDetails)
            {
                if(unit->name.has_name)
                {
                    string firstname = unit->name.first_name;
                    string restofname = Translation::TranslateName(&unit->name, false);
                    firstname[0] = toupper(firstname[0]);

                    // if creature has no nickname, restofname will already contain firstname
                    // no need for double output
                    if(restofname.compare(0, firstname.length(),firstname) != 0)
                        out.print("%s ", firstname.c_str());
                    out.print("%s ", restofname.c_str());
                }
                else
                {
                    // can that even happen? well, we have unnamed slabs, so maybe it can
                    out.print("Unnamed creature ");
                }

                out.print("born in %d, cursed in %d. (%s)\n",
                    unit->relations.birth_year,
                    unit->relations.curse_year,
                    unit->counters.death_id == -1 ? "alive" : "deceased"
                    );
            }

            if(giveNick)
                setUnitNickname(unit, "CURSED");
        }
    }

    if (checkWholeMap)
        out.print("Number of cursed creatures on map: %d \n", cursecount);
    else
        out.print("Number of cursed creatures on tile: %d \n", cursecount);
    
    return CR_OK;
}
