#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"
#include "modules/World.h"
#include "modules/Translation.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui_advmode.h"
#include "df/unit.h"
#include "df/nemesis_record.h"
#include "df/general_ref_is_nemesisst.h"
#include "df/viewscreen_optionst.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dungeon_monsterstatusst.h"


using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui_advmode;

using namespace DFHack::Simple::Translation;

static bool bodyswap_hotkey(Core *c, df::viewscreen *top);

command_result adv_bodyswap (Core * c, std::vector <std::string> & parameters);

DFHACK_PLUGIN("advtools");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();

    if (!ui_advmode)
        return CR_OK;

    commands.push_back(PluginCommand(
        "adv-bodyswap", "Change the adventurer unit.",
        adv_bodyswap, bodyswap_hotkey,
        " - When viewing unit details, body-swaps into that unit.\n"
        " - In the main adventure mode screen, reverts transient swap.\n"
        "Options:\n"
        "  force\n"
        "    Allow swapping into non-companion units.\n"
        "  permanent\n"
        "    Permanently change the unit to be the adventurer.\n"
        "    Otherwise it will revert if adv-bodyswap is called\n"
        "    in the main screen, or if the main menu, Fast Travel\n"
        "    or Sleep/Wait screen is opened.\n"
        "  no-make-leader\n"
        "    In permanent mode, don't swap companions to the new unit.\n"
    ));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

df::nemesis_record *getPlayerNemesis(Core *c, bool restore_swap);

static bool in_transient_swap = false;

DFhackCExport command_result plugin_onstatechange(Core* c, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
    case SC_GAME_UNLOADED:
        in_transient_swap = false;
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    // Revert transient swaps before trouble happens
    if (in_transient_swap)
    {
        auto screen = c->getTopViewscreen();
        bool revert = false;

        if (strict_virtual_cast<df::viewscreen_dungeonmodest>(screen))
        {
            using namespace df::enums::ui_advmode_menu;

            switch (ui_advmode->menu)
            {
            case Travel:
            case Sleep:
                revert = true;
                break;
            default:
                break;
            }
        }
        else if (strict_virtual_cast<df::viewscreen_optionst>(screen))
        {
            // Options may mean save game
            revert = true;
        }

        if (revert)
        {
            getPlayerNemesis(c, true);
            in_transient_swap = false;
        }
    }

    return CR_OK;
}

static bool bodyswap_hotkey(Core *c, df::viewscreen *top)
{
    return !!virtual_cast<df::viewscreen_dungeonmodest>(top) ||
           !!virtual_cast<df::viewscreen_dungeon_monsterstatusst>(top);
}

df::unit *getCurUnit(Core *c)
{
    auto top = c->getTopViewscreen();

    if (VIRTUAL_CAST_VAR(ms, df::viewscreen_dungeon_monsterstatusst, top))
        return ms->unit;

    return NULL;
}

df::nemesis_record *getNemesis(df::unit *unit)
{
    if (!unit)
        return NULL;

    for (unsigned i = 0; i < unit->refs.size(); i++)
    {
        df::nemesis_record *rv = unit->refs[i]->getNemesis();
        if (rv && rv->unit == unit)
            return rv;
    }

    return NULL;
}

bool bodySwap(Core *c, df::unit *player)
{
    if (!player)
    {
        c->con.printerr("Unit to swap is NULL\n");
        return false;
    }

    auto &vec = world->units.other[0];

    int idx = linear_index(vec, player);
    if (idx < 0)
    {
        c->con.printerr("Unit to swap not found: %d\n", player->id);
        return false;
    }

    if (idx != 0)
        std::swap(vec[0], vec[idx]);

    return true;
}

df::nemesis_record *getPlayerNemesis(Core *c, bool restore_swap)
{
    auto real_nemesis = df::nemesis_record::find(ui_advmode->player_id);
    if (!real_nemesis || !real_nemesis->unit)
    {
        c->con.printerr("Invalid player nemesis id: %d\n", ui_advmode->player_id);
        return NULL;
    }

    if (restore_swap)
    {
        df::unit *ctl = world->units.other[0][0];
        auto ctl_nemesis = getNemesis(ctl);

        if (ctl_nemesis != real_nemesis)
        {
            if (!bodySwap(c, real_nemesis->unit))
                return NULL;

            auto name = TranslateName(&real_nemesis->unit->name, false);
            c->con.print("Returned into the body of %s.\n", name.c_str());
        }

        real_nemesis->unit->relations.group_leader_id = -1;
        in_transient_swap = false;
    }

    return real_nemesis;
}

void changeGroupLeader(df::nemesis_record *new_nemesis, df::nemesis_record *old_nemesis)
{
    auto &cvec = new_nemesis->companions;

    // Swap companions
    cvec.swap(old_nemesis->companions);

    vector_erase_at(cvec, linear_index(cvec, new_nemesis->id));
    insert_into_vector(cvec, old_nemesis->id);

    // Update follow
    new_nemesis->group_leader_id = -1;
    new_nemesis->unit->relations.group_leader_id = -1;

    for (unsigned i = 0; i < cvec.size(); i++)
    {
        auto nm = df::nemesis_record::find(cvec[i]);
        if (!nm)
            continue;

        nm->group_leader_id = new_nemesis->id;
        if (nm->unit)
            nm->unit->relations.group_leader_id = new_nemesis->unit_id;
    }
}

void copyAcquaintances(df::nemesis_record *new_nemesis, df::nemesis_record *old_nemesis)
{
    auto &svec = old_nemesis->unit->adventurer_knows;
    auto &tvec = new_nemesis->unit->adventurer_knows;

    for (unsigned i = 0; i < svec.size(); i++)
        insert_into_vector(tvec, svec[i]);

    insert_into_vector(tvec, old_nemesis->unit_id);
}

command_result adv_bodyswap (Core * c, std::vector <std::string> & parameters)
{
    // HOTKEY COMMAND; CORE IS SUSPENDED
    bool force = false;
    bool permanent = false;
    bool no_make_leader = false;

    for (unsigned i = 0; i < parameters.size(); i++)
    {
        auto &item = parameters[i];

        if (item == "force")
            force = true;
        else if (item == "permanent")
            permanent = true;
        else if (item == "no-make-leader")
            no_make_leader = true;
        else
            return CR_WRONG_USAGE;
    }

    // Get the real player; undo previous transient swap
    auto real_nemesis = getPlayerNemesis(c, true);
    if (!real_nemesis)
        return CR_FAILURE;

    // Get the unit to swap to
    auto new_unit = getCurUnit(c);
    auto new_nemesis = getNemesis(new_unit);

    if (!new_nemesis)
    {
        if (new_unit)
        {
            c->con.printerr("Cannot swap into a non-historical unit.\n");
            return CR_FAILURE;
        }

        return CR_OK;
    }

    if (new_nemesis == real_nemesis)
        return CR_OK;

    // Verify it's a companion
    if (!force && linear_index(real_nemesis->companions, new_nemesis->id) < 0)
    {
        c->con.printerr("This is not your companion - use force to bodyswap.\n");
        return CR_FAILURE;
    }

    // Swap
    if (!bodySwap(c, new_nemesis->unit))
        return CR_FAILURE;

    auto name = TranslateName(&new_nemesis->unit->name, false);
    c->con.print("Swapped into the body of %s.\n", name.c_str());

    // Permanently re-link everything
    if (permanent)
    {
        ui_advmode->player_id = new_nemesis->id;

        // Flag 0 appears to be the 'active adventurer' flag, and
        // the player_id field above seems to be computed using it
        // when a savegame is loaded.
        // Also, unless this is set, it is impossible to retire.
        real_nemesis->flags.set(0, false);
        new_nemesis->flags.set(0, true);

        real_nemesis->flags.set(1, true); // former retired adventurer
        new_nemesis->flags.set(2, true); // blue color in legends

        // Reassign companions and acquaintances
        if (!no_make_leader)
        {
            changeGroupLeader(new_nemesis, real_nemesis);
            copyAcquaintances(new_nemesis, real_nemesis);
        }
    }
    else
    {
        in_transient_swap = true;

        // Make the player unit follow around to avoid bad consequences
        // if it is unloaded before the transient swap is reverted.
        real_nemesis->unit->relations.group_leader_id = new_nemesis->unit_id;
    }

    return CR_OK;
}
