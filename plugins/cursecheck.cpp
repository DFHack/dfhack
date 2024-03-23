// cursecheck plugin
//
// check single tile or whole map/world for cursed creatures by checking if a valid curse date (!=-1) is set
// if a cursor is active only the selected tile will be observed
// without cursor the whole map will be checked
// by default cursed creatures will be only counted
//
// the tool was intended to help finding vampires but it will also list necromancers, werebeasts and zombies
//
// Options:
//   detail  - print full name, date of birth, date of curse (vamp might use fake identity, though)
//             show if the creature is active or dead, missing, ghostly (ghost vampires should not exist in 34.05)
//             identify type of curse (works fine for vanilla ghosts, vamps, werebeasts, zombies and necromancers)
//   nick    - set nickname to type of curse(does not always show up ingame, some vamps don't like nicknames)
//   all     - don't ignore dead and inactive creatures (former ghosts, dead necromancers, ...)
//   verbose - acts like detail but also lists all curse tags (if you want to know it all).

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
#include "df/unit_syndrome.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/identity.h"
#include "df/language_name.h"
#include "df/syndrome.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/incident.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("cursecheck");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);

enum class curses : int8_t {
    None,
    Unknown,
    Ghost,
    Zombie,
    Necromancer,
    Werebeast,
    Vampire
};

std::vector<string> curse_names = {
    "none",
    "unknown",
    "ghost",
    "zombie",
    "necromancer",
    "werebeast",
    "vampire"
};

command_result cursecheck (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "cursecheck",
        "Check for cursed creatures (undead, necromancers...)",
        cursecheck));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

curses determineCurse(df::unit * unit)
{
    curses cursetype = curses::None;

    if (unit->curse_year != -1)
    {
        cursetype = curses::Unknown;
    }

    // ghosts: ghostly, duh
    // as of DF 34.05 and higher vampire ghosts and the like should not be possible
    // if they get reintroduced later it will become necessary to watch 'ghostly' seperately
    if(unit->flags3.bits.ghostly)
        cursetype = curses::Ghost;

    // zombies: undead or hate life (according to ag), not bloodsuckers
    if( (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE || unit->curse.add_tags1.bits.NOT_LIVING)
        && !unit->curse.add_tags1.bits.BLOODSUCKER )
        cursetype = curses::Zombie;

    // necromancers: alive, don't eat, don't drink, don't age
    if(!unit->curse.add_tags1.bits.NOT_LIVING
        && unit->curse.add_tags1.bits.NO_EAT
        && unit->curse.add_tags1.bits.NO_DRINK
        && unit->curse.add_tags2.bits.NO_AGING
        )
        cursetype = curses::Necromancer;

    // werecreatures: subjected to a were syndrome. The curse effects are active only when
    // in were form.
    for (size_t i = 0; i < unit->syndromes.active.size(); i++)
    {
        for (size_t k = 0; k < world->raws.syndromes.all[unit->syndromes.active[i]->type]->syn_class.size(); k++)
        {
            if (strcmp (world->raws.syndromes.all[unit->syndromes.active[i]->type]->syn_class[k]->c_str(), "WERECURSE") == 0)
            {
                cursetype = curses::Werebeast;
                break;
            }
        }
    }

    // vampires: bloodsucker (obvious enough)
    if(unit->curse.add_tags1.bits.BLOODSUCKER)
        cursetype = curses::Vampire;

    return cursetype;
}

command_result cursecheck (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);

    bool giveDetails = false;
    bool giveUnitID = false;
    bool giveNick = false;
    bool ignoreDead = true;
    bool verbose = false;
    size_t cursecount = 0;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print(  "  Search for cursed creatures (ghosts, vampires, necromancers, zombies, werebeasts).\n"
                        "  With map cursor active only the current tile will be checked.\n"
                        "  Without an in-game cursor the whole map/world will be scanned.\n"
                        "  By default cursed creatures are only counted to make it more interesting.\n"
                        "  By default dead and passive creatures (aka really dead) are ignored.\n"
                        "Options:\n"
                        "  detail  - show details (name and age shown ingame might differ)\n"
                        "  ids     - add creature and race IDs to be show on the  output\n"
                        "  nick    - try to set cursetype as nickname (does not always work)\n"
                        "  all     - include dead and passive creatures\n"
                        "  verbose - show all curse tags (if you really want to know it all)\n"
                        );
            return CR_OK;
        }
        if(parameters[i] == "detail")
            giveDetails = true;
        if(parameters[i] == "ids")
            giveUnitID = true;
        if(parameters[i] == "nick")
            giveNick = true;
        if(parameters[i] == "all")
            ignoreDead = false;
        if(parameters[i] == "verbose")
        {
            // verbose makes no sense without enabling details
            giveDetails = true;
            verbose = true;
        }
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

        // filter out all "living" units that are currently removed from play
        // don't spam all completely dead creatures if not explicitly wanted
        if((!Units::isActive(unit) && !Units::isKilled(unit)) || (Units::isKilled(unit) && ignoreDead))
        {
            continue;
        }

        // bail out if we have a map cursor and creature is not at that specific position
        if ( !checkWholeMap && (unit->pos.x != cursorX || unit->pos.y != cursorY || unit->pos.z != cursorZ) )
        {
            continue;
        }

        curses cursetype = determineCurse(unit);

        if (cursetype != curses::None)
        {
             cursecount++;

            if(giveNick)
            {
                Units::setNickname(unit, curse_names[static_cast<size_t>(cursetype)].c_str()); //"CURSED");
            }

            if (giveDetails)
            {
                if (unit->name.has_name)
                {
                    string firstname = unit->name.first_name;
                    string restofname = Translation::TranslateName(&unit->name, false);
                    firstname[0] = toupper(firstname[0]);

                    // if creature has no nickname, restofname will already contain firstname
                    // no need for double output
                    if (restofname.compare(0, firstname.length(), firstname) != 0)
                        out.print("%s ", firstname.c_str());
                    out.print("%s ", restofname.c_str());
                }
                else
                {
                    // happens with unnamed zombies and resurrected body parts
                    out.print("Unnamed creature ");
                }

                auto death = df::incident::find(unit->counters.death_id);
                bool missing = false;
                if (death && !death->flags.bits.discovered)
                {
                    missing = true;
                }

                out.print("born in %d, cursed in %d to be a %s. (%s%s%s)\n",
                    unit->birth_year,
                    unit->curse_year,
                    curse_names [static_cast<size_t>(cursetype)].c_str(),
                    // technically most cursed creatures are undead,
                    // therefore output 'active' if they are not completely dead
                    unit->flags2.bits.killed ? "deceased" : "active",
                    unit->flags3.bits.ghostly ? "-ghostly" : "",
                    missing ? "-missing" : ""
                );

                // dump all curse flags on demand
                if (verbose)
                {
                    out << "Curse flags: "
                        << bitfield_to_string(unit->curse.add_tags1) << endl
                        << bitfield_to_string(unit->curse.add_tags2) << endl;
                }
            }

            if (giveUnitID)
            {
                out.print("Creature %d, race %d (%x)\n", unit->id, unit->race, unit->race);
            }
        }
    }

    if (checkWholeMap)
        out.print("Number of cursed creatures on map: %zd \n", cursecount);
    else
        out.print("Number of cursed creatures on tile: %zd \n", cursecount);

    return CR_OK;
}
