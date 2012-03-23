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
//   nick    - set nickname to 'CURSED' (does not always show up ingame, some vamps don't like nicknames)
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
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/assumed_identity.h"
#include "df/language_name.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/death_info.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;

command_result cursecheck (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("cursecheck");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("cursecheck",
        "Checks for cursed creatures (vampires, necromancers, zombies, ...).",
        cursecheck, false ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


// code for setting nicknames is copypasta from rename.cpp
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

void cursedump (color_ostream &out, df::unit * unit);

std::string determineCurse(df::unit * unit)
{
    string cursetype = "unknown";
            
    // ghosts: ghostly, duh
    if(unit->flags3.bits.ghostly)
        cursetype = "ghost";

    // zombies: undead or hate life (according to ag), not bloodsuckers
    if( (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE || unit->curse.add_tags1.bits.NOT_LIVING)
        && !unit->curse.add_tags1.bits.BLOODSUCKER )
        cursetype = "zombie";

    // necromancers: alive, don't eat, don't drink, don't age
    if(!unit->curse.add_tags1.bits.NOT_LIVING 
        && unit->curse.add_tags1.bits.NO_EAT 
        && unit->curse.add_tags1.bits.NO_DRINK 
        && unit->curse.add_tags2.bits.NO_AGING
        )
        cursetype = "necromancer";

    // werecreatures: alive, DO eat, DO drink, don't age
    if(!unit->curse.add_tags1.bits.NOT_LIVING 
        && !unit->curse.add_tags1.bits.NO_EAT 
        && !unit->curse.add_tags1.bits.NO_DRINK 
        && unit->curse.add_tags2.bits.NO_AGING )
        cursetype = "werebeast";

    // vampires: bloodsucker (obvious enough)
    if(unit->curse.add_tags1.bits.BLOODSUCKER)
        cursetype = "vampire";

    return cursetype;
}

command_result cursecheck (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);

    bool giveDetails = false;
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
                        "  nick    - try to set cursetype as nickname (does not always work)\n"
                        "  all     - include dead and passive creatures\n"
                        "  verbose - show all curse tags (if you really want to know it all)\n"
                        );
            return CR_OK;
        }
        if(parameters[i] == "detail")
            giveDetails = true;
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

        // don't spam all completely dead creatures if not explicitly wanted
        if(unit->flags1.bits.dead && ignoreDead)
        {
            continue;
        }

        // bail out if we have a map cursor and creature is not at that specific position
        if ( !checkWholeMap && (unit->pos.x != cursorX || unit->pos.y != cursorY || unit->pos.z != cursorZ) )
        {
            continue;
        }

        // non-cursed creatures have curse_year == -1
        if(unit->relations.curse_year != -1)
        {
            cursecount++;

            string cursetype = determineCurse(unit);
                      
            if(giveNick)
            {
                setUnitNickname(unit, cursetype); //"CURSED");
            }

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
                    // happens with unnamed zombies and resurrected body parts
                    out.print("Unnamed creature ");
                }

                auto death = df::death_info::find(unit->counters.death_id);
                bool missing = false;
                if (death && !death->flags.bits.discovered)
                {
                    missing = true;
                }
                
                out.print("born in %d, cursed in %d to be a %s. (%s%s%s)\n",
                    unit->relations.birth_year,
                    unit->relations.curse_year,
                    cursetype.c_str(),
                    // technically most cursed creatures are undead, 
                    // therefore output 'active' if they are not completely dead
                    unit->flags1.bits.dead ? "deceased" : "active",
                    unit->flags3.bits.ghostly ? "-ghostly" : "",
                    missing ? "-missing" : ""
                    );

                if (missing)
                {
                    out.print("- You can use 'tweak clear-missing' to allow engraving a memorial easier.\n");
                }

                // dump all curse flags on demand
                if (verbose)
                {
                    cursedump(out, unit);
                }
            }
        }
    }

    if (checkWholeMap)
        out.print("Number of cursed creatures on map: %d \n", cursecount);
    else
        out.print("Number of cursed creatures on tile: %d \n", cursecount);
    
    return CR_OK;
}

void cursedump (color_ostream &out, df::unit * unit)
{
    out << "Curse flags: ";
    if(unit->curse.add_tags1.bits.BLOODSUCKER)
        out << "bloodsucker ";
    if(unit->curse.add_tags1.bits.EXTRAVISION)
        out << "extravision ";
    if(unit->curse.add_tags1.bits.OPPOSED_TO_LIFE)
        out << "opposed_to_life ";
    if(unit->curse.add_tags1.bits.NOT_LIVING)
        out << "not_living ";
    if(unit->curse.add_tags1.bits.NOEXERT)
        out << "noexpert ";
    if(unit->curse.add_tags1.bits.NOPAIN)
        out << "nopain ";
    if(unit->curse.add_tags1.bits.NOBREATHE)
        out << "nobreathe ";
    if(unit->curse.add_tags1.bits.HAS_BLOOD)
        out << "has_blood ";
    if(unit->curse.add_tags1.bits.NOSTUN)
        out << "nostun ";
    if(unit->curse.add_tags1.bits.NONAUSEA)
        out << "nonausea ";
    if(unit->curse.add_tags1.bits.NO_DIZZINESS)
        out << "no_dizziness ";
    if(unit->curse.add_tags1.bits.TRANCES)
        out << "trances ";
    if(unit->curse.add_tags1.bits.NOEMOTION)
        out << "noemotion ";
    if(unit->curse.add_tags1.bits.PARALYZEIMMUNE)
        out << "paralyzeimmune ";
    if(unit->curse.add_tags1.bits.NOFEAR)
        out << "nofear ";
    if(unit->curse.add_tags1.bits.NO_EAT)
        out << "no_eat ";
    if(unit->curse.add_tags1.bits.NO_DRINK)
        out << "no_drink ";
    if(unit->curse.add_tags1.bits.MISCHIEVOUS)
        out << "mischievous ";
    if(unit->curse.add_tags1.bits.NO_PHYS_ATT_GAIN)
        out << "no_phys_att_gain ";
    if(unit->curse.add_tags1.bits.NO_PHYS_ATT_RUST)
        out << "no_phys_att_rust ";
    if(unit->curse.add_tags1.bits.NOTHOUGHT)
        out << "nothought ";
    if(unit->curse.add_tags1.bits.NO_THOUGHT_CENTER_FOR_MOVEMENT)
        out << "no_thought_center_for_movement ";
    if(unit->curse.add_tags1.bits.CAN_SPEAK)
        out << "can_speak ";
    if(unit->curse.add_tags1.bits.CAN_LEARN)
        out << "can_learn ";
    if(unit->curse.add_tags1.bits.CRAZED)
        out << "crazed ";
    if(unit->curse.add_tags1.bits.BLOODSUCKER)
        out << "bloodsucker ";
    if(unit->curse.add_tags1.bits.SUPERNATURAL)
        out << "supernatural ";

    if(unit->curse.add_tags2.bits.NO_AGING)
        out << "no_aging ";
    if(unit->curse.add_tags2.bits.STERILE)
        out << "sterile ";
    if(unit->curse.add_tags2.bits.FIT_FOR_ANIMATION)
        out << "fit_for_animation ";
    if(unit->curse.add_tags2.bits.FIT_FOR_RESURRECTION)
        out << "fit_for_resurrection ";

    out << endl << endl;
}