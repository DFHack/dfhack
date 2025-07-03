// cursecheck plugin
//
// check unit or whole map/world for cursed creatures by checking if a valid curse date (!=-1) is set
// if a unit is selected only the selected unit will be observed
// otherwise the whole map will be checked
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


#include "Console.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Units.h"

#include "df/incident.h"
#include "df/syndrome.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/unit_syndrome.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("cursecheck");
REQUIRE_GLOBAL(world);

enum curses {
    None = 0,
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
    if( (unit->uwss_add_caste_flag.bits.OPPOSED_TO_LIFE || unit->uwss_add_caste_flag.bits.NOT_LIVING)
        && !unit->uwss_add_caste_flag.bits.BLOODSUCKER)
        cursetype = curses::Zombie;

    // necromancers: alive, don't eat, don't drink, don't age
    if (!unit->uwss_add_caste_flag.bits.NOT_LIVING
        && unit->uwss_add_caste_flag.bits.NO_EAT
        && unit->uwss_add_caste_flag.bits.NO_DRINK
        && unit->uwss_add_property.bits.NO_AGING
        )
        cursetype = curses::Necromancer;

    // werecreatures: subjected to a were syndrome. The curse effects are active only when
    // in were form.
    for (auto active_syndrome : unit->syndromes.active) {
        auto syndrome = df::syndrome::find(active_syndrome->type);
        if (syndrome) {
            for (auto classname : syndrome->syn_class)
                if (classname && *classname == "WERECURSE") {
                    cursetype = curses::Werebeast;
                    break;
                }
        }
    }

    // vampires: bloodsucker (obvious enough)
    if (unit->uwss_add_caste_flag.bits.BLOODSUCKER)
        cursetype = curses::Vampire;

    return cursetype;
}

command_result cursecheck(color_ostream& out, vector <string>& parameters)
{
    df::unit* selected_unit = Gui::getSelectedUnit(out, true);

    bool giveDetails = false;
    bool giveUnitID = false;
    bool giveNick = false;
    bool ignoreDead = true;
    bool verbose = false;
    size_t cursecount = 0;

    for (auto parameter : parameters)
    {
        if (parameter == "help" || parameter == "?")
            return CR_WRONG_USAGE;
        if (parameter == "detail")
            giveDetails = true;
        if (parameter == "ids")
            giveUnitID = true;
        if (parameter == "nick")
            giveNick = true;
        if (parameter == "all")
            ignoreDead = false;
        if (parameter == "verbose")
        {
            // verbose makes no sense without enabling details
            giveDetails = true;
            verbose = true;
        }
    }

    // check whole map if no unit is selected
    vector<df::unit*> to_check;
    if (selected_unit)
        to_check.push_back(selected_unit);
    for (df::unit* unit : to_check.size() ? to_check : world->units.all)
    {
        // filter out all "living" units that are currently removed from play
        // don't spam all completely dead creatures if not explicitly wanted
        if ((!Units::isActive(unit) && !Units::isKilled(unit)) || (Units::isKilled(unit) && ignoreDead))
        {
            continue;
        }

        curses cursetype = determineCurse(unit);

        if (cursetype != curses::None)
        {
            cursecount++;

            if (giveNick)
            {
                Units::setNickname(unit, curse_names[cursetype]); //"CURSED");
            }

            if (giveDetails)
            {
                out << DF2CONSOLE(Units::getReadableName(unit));

                auto death = df::incident::find(unit->counters.death_id);
                out.print(", born in %d, cursed in %d to be a %s. (%s%s)\n",
                    unit->birth_year,
                    unit->curse_year,
                    curse_names[cursetype].c_str(),
                    // technically most cursed creatures are undead,
                    // therefore output 'active' if they are not completely dead
                    unit->flags2.bits.killed ? "deceased" : "active",
                    death && !death->flags.bits.discovered ? "-missing" : ""
                );

                // dump all curse flags on demand
                if (verbose)
                {
                    out << "Curse flags: "
                        << bitfield_to_string(unit->uwss_add_caste_flag) << std::endl
                        << bitfield_to_string(unit->uwss_add_property) << std::endl;
                }
            }

            if (giveUnitID)
            {
                out.print("Creature %d, race %d (%x)\n", unit->id, unit->race, unit->race);
            }
        }
    }

    if (selected_unit && !giveDetails)
        out.print("Selected unit is %scursed\n", cursecount == 0 ? "not " : "");
    else if (!selected_unit)
        out.print("%zd cursed creatures on map\n", cursecount);

    return CR_OK;
}
