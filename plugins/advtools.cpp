#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"
#include "modules/World.h"
#include "modules/Translation.h"
#include "modules/Materials.h"
#include "modules/Maps.h"
#include "modules/Items.h"
#include "modules/Gui.h"
#include "modules/Units.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui_advmode.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/map_block.h"
#include "df/nemesis_record.h"
#include "df/historical_figure.h"
#include "df/general_ref_is_nemesisst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_building_civzone_assignedst.h"
#include "df/material.h"
#include "df/craft_material_class.h"
#include "df/viewscreen_optionst.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dungeon_monsterstatusst.h"
#include "df/nemesis_flags.h"

#include <math.h>

using namespace DFHack;
using namespace df::enums;

using df::nemesis_record;
using df::historical_figure;

using namespace DFHack::Translation;
/*
advtools
========
A package of different adventure mode tools.  Usage:

:list-equipped [all]:   List armor and weapons equipped by your companions.
                        If all is specified, also lists non-metal clothing.
:metal-detector [all-types] [non-trader]:
                        Reveal metal armor and weapons in shops. The options
                        disable the checks on item type and being in shop.
*/

DFHACK_PLUGIN("advtools");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui_advmode);

/*********************
 *  PLUGIN INTERFACE *
 *********************/

static bool bodyswap_hotkey(df::viewscreen *top);

command_result adv_bodyswap (color_ostream &out, std::vector <std::string> & parameters);
command_result adv_tools (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!ui_advmode)
        return CR_OK;

    commands.push_back(PluginCommand(
        "advtools", "Adventure mode tools.",
        adv_tools, false,
        "  list-equipped [all]\n"
        "    List armor and weapons equipped by your companions.\n"
        "    If all is specified, also lists non-metal clothing.\n"
        "  metal-detector [all-types] [non-trader]\n"
        "    Reveal metal armor and weapons in shops. The options\n"
        "    disable the checks on item type and being in shop.\n"
    ));

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
        "  noinherit\n"
        "    In permanent mode, don't reassign companions to the new unit.\n"
    ));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

df::nemesis_record *getPlayerNemesis(color_ostream &out, bool restore_swap);

DFHACK_PLUGIN_IS_ENABLED(in_transient_swap);

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
    case SC_WORLD_UNLOADED:
        in_transient_swap = false;
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // Revert transient swaps before trouble happens
    if (in_transient_swap)
    {
        auto screen = Core::getTopViewscreen();
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
            getPlayerNemesis(out, true);
            in_transient_swap = false;
        }
    }

    return CR_OK;
}

/*********************
 * UTILITY FUNCTIONS *
 *********************/

static bool bodyswap_hotkey(df::viewscreen *top)
{
    return !!virtual_cast<df::viewscreen_dungeonmodest>(top) ||
           !!virtual_cast<df::viewscreen_dungeon_monsterstatusst>(top);
}

bool bodySwap(color_ostream &out, df::unit *player)
{
    if (!player)
    {
        out.printerr("Unit to swap is NULL\n");
        return false;
    }

    auto &vec = world->units.active;

    int idx = linear_index(vec, player);
    if (idx < 0)
    {
        out.printerr("Unit to swap not found: %d\n", player->id);
        return false;
    }

    if (idx != 0)
        std::swap(vec[0], vec[idx]);

    return true;
}

df::nemesis_record *getPlayerNemesis(color_ostream &out, bool restore_swap)
{
    auto real_nemesis = vector_get(world->nemesis.all, ui_advmode->player_id);
    if (!real_nemesis || !real_nemesis->unit)
    {
        out.printerr("Invalid player nemesis id: %d\n", ui_advmode->player_id);
        return NULL;
    }

    if (restore_swap)
    {
        df::unit *ctl = world->units.active[0];
        auto ctl_nemesis = Units::getNemesis(ctl);

        if (ctl_nemesis != real_nemesis)
        {
            if (!bodySwap(out, real_nemesis->unit))
                return NULL;

            auto name = TranslateName(&real_nemesis->unit->name, false);
            out.print("Returned into the body of %s.\n", name.c_str());
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

void sortCompanionNemesis(std::vector<nemesis_record*> *list, int player_id = -1)
{
    std::map<int, nemesis_record*> table;
    std::vector<nemesis_record*> output;

    output.reserve(list->size());

    if (player_id < 0)
    {
        auto real_nemesis = vector_get(world->nemesis.all, ui_advmode->player_id);
        if (real_nemesis)
            player_id = real_nemesis->id;
    }

    // Index records; find the player
    for (size_t i = 0; i < list->size(); i++)
    {
        auto item = (*list)[i];
        if (item->id == player_id)
            output.push_back(item);
        else
            table[item->figure->id] = item;
    }

    // Pull out the items by the persistent sort order
    auto &order_vec = ui_advmode->companions.all_histfigs;
    for (size_t i = 0; i < order_vec.size(); i++)
    {
        auto it = table.find(order_vec[i]);
        if (it == table.end())
            continue;
        output.push_back(it->second);
        table.erase(it);
    }

    // The remaining ones in reverse id order
    for (auto it = table.rbegin(); it != table.rend(); ++it)
        output.push_back(it->second);

    list->swap(output);
}

void listCompanions(color_ostream &out, std::vector<nemesis_record*> *list, bool units = true)
{
    nemesis_record *player = getPlayerNemesis(out, false);
    if (!player)
        return;

    list->push_back(player);

    for (size_t i = 0; i < player->companions.size(); i++)
    {
        auto item = nemesis_record::find(player->companions[i]);
        if (item && (item->unit || !units))
            list->push_back(item);
    }
}

std::string getUnitNameProfession(df::unit *unit)
{
    std::string name = TranslateName(&unit->name, false) + ", ";
    if (unit->custom_profession.empty())
        name += ENUM_ATTR_STR(profession, caption, unit->profession);
    else
        name += unit->custom_profession;
    return name;
}

enum InventoryMode {
    INV_HAULED,
    INV_WEAPON,
    INV_WORN,
    INV_IN_CONTAINER
};

typedef std::pair<df::item*,InventoryMode> inv_item;

static void listContainerInventory(std::vector<inv_item> *list, df::item *container)
{
    auto &refs = container->general_refs;
    for (size_t i = 0; i < refs.size(); i++)
    {
        auto ref = refs[i];
        if (!strict_virtual_cast<df::general_ref_contains_itemst>(ref))
            continue;

        df::item *child = ref->getItem();
        if (!child) continue;

        list->push_back(inv_item(child, INV_IN_CONTAINER));
        listContainerInventory(list, child);
    }
}

void listUnitInventory(std::vector<inv_item> *list, df::unit *unit)
{
    auto &items = unit->inventory;
    for (size_t i = 0; i < items.size(); i++)
    {
        auto item = items[i];
        InventoryMode mode;

        switch (item->mode) {
        case df::unit_inventory_item::Hauled:
            mode = INV_HAULED;
            break;
        case df::unit_inventory_item::Weapon:
            mode = INV_WEAPON;
            break;
        default:
            mode = INV_WORN;
        }

        list->push_back(inv_item(item->item, mode));
        listContainerInventory(list, item->item);
    }
}

bool isShopItem(df::item *item)
{
    for (size_t k = 0; k < item->general_refs.size(); k++)
    {
        auto ref = item->general_refs[k];
        if (virtual_cast<df::general_ref_building_civzone_assignedst>(ref))
            return true;
    }

    return false;
}

bool isWeaponArmor(df::item *item)
{
    using namespace df::enums::item_type;

    switch (item->getType()) {
    case HELM:
    case ARMOR:
    case WEAPON:
    case AMMO:
    case GLOVES:
    case PANTS:
    case SHOES:
        return true;
    default:
        return false;
    }
}

int containsMetalItems(df::item *item, bool all, bool non_trader, bool rec = false)
{
    int cnt = 0;

    auto &refs = item->general_refs;
    for (size_t i = 0; i < refs.size(); i++)
    {
        auto ref = refs[i];

        if (strict_virtual_cast<df::general_ref_unit_holderst>(ref))
            return 0;
        if (!rec && strict_virtual_cast<df::general_ref_contained_in_itemst>(ref))
            return 0;

        if (strict_virtual_cast<df::general_ref_contains_itemst>(ref))
        {
            df::item *child = ref->getItem();
            if (!child) continue;

            cnt += containsMetalItems(child, all, non_trader, true);
        }
    }

    if (!non_trader && !isShopItem(item))
        return cnt;
    if (!all && !isWeaponArmor(item))
        return cnt;

    MaterialInfo minfo(item);
    if (minfo.getCraftClass() != craft_material_class::Metal)
        return cnt;

    return ++cnt;
}

void joinCounts(std::map<df::coord, int> &counts)
{
    for (auto it = counts.begin(); it != counts.end(); it++)
    {
        df::coord pt = it->first;
        while (pt.x > 0 && counts.count(pt - df::coord(1,0,0)))
            pt.x--;
        while (pt.y > 0 &&counts.count(pt - df::coord(0,1,0)))
            pt.y--;
        while (pt.x < 0 && counts.count(pt + df::coord(1,0,0)))
            pt.x++;
        while (pt.y < 0 &&counts.count(pt + df::coord(0,1,0)))
            pt.y++;

        if (pt == it->first)
            continue;

        counts[pt] += it->second;
        it->second = 0;
    }
}

/*********************
 *     FORMATTING    *
 *********************/

static void printCompanionHeader(color_ostream &out, size_t i, df::unit *unit)
{
    out.color(COLOR_GREY);

    if (i < 28)
        out << char('a'+i);
    else
        out << i;

    out << ": " << getUnitNameProfession(unit);
    if (unit->flags1.bits.dead)
        out << " (DEAD)";
    if (unit->flags3.bits.ghostly)
        out << " (GHOST)";
    out << endl;

    out.reset_color();
}

static size_t formatSize(std::vector<std::string> *out, const std::map<std::string, int> in, size_t *cnt)
{
    size_t len = 0;

    for (auto it = in.begin(); it != in.end(); ++it)
    {
        std::string line = it->first;
        if (it->second != 1)
            line += stl_sprintf(" [%d]", it->second);
        len = std::max(len, line.size());
        out->push_back(line);
    }

    if (out->empty())
    {
        out->push_back("(none)");
        len = 6;
    }

    if (cnt)
        *cnt = std::max(*cnt, out->size());

    return len;
}

static std::string formatDirection(df::coord delta)
{
    std::string ns, ew, dir;

    if (delta.x > 0)
        ew = "E";
    else if (delta.x < 0)
        ew = "W";

    if (delta.y > 0)
        ns = "S";
    else if (delta.y < 0)
        ns = "N";

    if (abs(delta.x) > abs(delta.y)*5)
        dir = ew;
    else if (abs(delta.y) > abs(delta.x)*5)
        dir = ns;
    else if (abs(delta.x) > abs(delta.y)*2)
        dir = ew + ns + ew;
    else if (abs(delta.y) > abs(delta.x)*2)
        dir = ns + ns + ew;
    else if (delta.x || delta.y)
        dir = ns + ew;
    else
        dir = "***";

    int dist = (int)sqrt((double)(delta.x*delta.x + delta.y*delta.y));
    return stl_sprintf("%d away %s %+d", dist, dir.c_str(), delta.z);
}

static void printEquipped(color_ostream &out, df::unit *unit, bool all)
{
    std::vector<inv_item> items;
    listUnitInventory(&items, unit);

    std::map<std::string, int> head, body, legs, weapons;

    for (auto it = items.begin(); it != items.end(); ++it)
    {
        df::item *item = it->first;

        // Skip non-worn non-weapons
        ItemTypeInfo iinfo(item);

        bool is_weapon = (it->second == INV_WEAPON || iinfo.type == item_type::AMMO);
        if (!(is_weapon || it->second == INV_WORN))
            continue;

        // Skip non-metal, unless all
        MaterialInfo minfo(item);
        df::craft_material_class mclass = minfo.getCraftClass();

        bool is_metal = (mclass == craft_material_class::Metal);
        if (!(is_weapon || all || is_metal))
            continue;

        // Format the name
        std::string name;
        if (is_metal)
            name = minfo.toString() + " ";
        else if (mclass != craft_material_class::None)
            name = toLower(ENUM_KEY_STR(craft_material_class,mclass)) + " ";
        name += iinfo.toString();

        // Add to the right table
        int count = item->getStackSize();

        if (is_weapon)
        {
            weapons[name] += count;
            continue;
        }

        switch (iinfo.type) {
        case item_type::HELM:
            head[name] += count;
            break;
        case item_type::ARMOR:
        case item_type::GLOVES:
        case item_type::BACKPACK:
        case item_type::QUIVER:
            body[name] += count;
            break;
        case item_type::PANTS:
        case item_type::SHOES:
            legs[name] += count;
            break;
        default:
            weapons[name] += count;
        }
    }

    std::vector<std::string> cols[4];
    size_t sizes[4];
    size_t lines = 0;

    sizes[0] = formatSize(&cols[0], head, &lines);
    sizes[1] = formatSize(&cols[1], body, &lines);
    sizes[2] = formatSize(&cols[2], legs, &lines);
    sizes[3] = formatSize(&cols[3], weapons, &lines);

    for (size_t i = 0; i < lines; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            size_t sz = std::max(sizes[j], size_t(18));
            out << "| " << std::left << std::setw(sz) << vector_get(cols[j],i) << " ";
        }

        out << "|" << std::endl;
    }
}

/*********************
 *      COMMANDS     *
 *********************/

command_result adv_bodyswap (color_ostream &out, std::vector <std::string> & parameters)
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
        else if (item == "noinherit")
            no_make_leader = true;
        else
            return CR_WRONG_USAGE;
    }

    // Get the real player; undo previous transient swap
    auto real_nemesis = getPlayerNemesis(out, true);
    if (!real_nemesis)
        return CR_FAILURE;

    // Get the unit to swap to
    auto new_unit = Gui::getSelectedUnit(out, true);
    auto new_nemesis = Units::getNemesis(new_unit);

    if (!new_nemesis)
    {
        if (new_unit)
        {
            out.printerr("Cannot swap into a non-historical unit.\n");
            return CR_FAILURE;
        }

        return CR_OK;
    }

    if (new_nemesis == real_nemesis)
        return CR_OK;

    // Verify it's a companion
    if (!force && linear_index(real_nemesis->companions, new_nemesis->id) < 0)
    {
        out.printerr("This is not your companion - use force to bodyswap.\n");
        return CR_FAILURE;
    }

    // Swap
    if (!bodySwap(out, new_nemesis->unit))
        return CR_FAILURE;

    auto name = TranslateName(&new_nemesis->unit->name, false);
    out.print("Swapped into the body of %s.\n", name.c_str());

    // Permanently re-link everything
    if (permanent)
    {
        using namespace df::enums::nemesis_flags;

        ui_advmode->player_id = linear_index(world->nemesis.all, new_nemesis);

        // Flag 0 appears to be the 'active adventurer' flag, and
        // the player_id field above seems to be computed using it
        // when a savegame is loaded.
        // Also, unless this is set, it is impossible to retire.
        real_nemesis->flags.set(ACTIVE_ADVENTURER, false);
        new_nemesis->flags.set(ACTIVE_ADVENTURER, true);

        real_nemesis->flags.set(RETIRED_ADVENTURER, true); // former retired adventurer
        new_nemesis->flags.set(ADVENTURER, true); // blue color in legends

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

command_result adv_tools (color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    const auto &command = parameters[0];
    if (command == "list-equipped")
    {
        bool all = false;
        for (size_t i = 1; i < parameters.size(); i++)
        {
            if (parameters[i] == "all")
                all = true;
            else
                return CR_WRONG_USAGE;
        }

        std::vector<nemesis_record*> list;

        listCompanions(out, &list);
        sortCompanionNemesis(&list);

        for (size_t i = 0; i < list.size(); i++)
        {
            auto item = list[i];
            auto unit = item->unit;

            printCompanionHeader(out, i, unit);
            printEquipped(out, unit, all);
        }

        return CR_OK;
    }
    else if (command == "metal-detector")
    {
        bool all = false, non_trader = false;
        for (size_t i = 1; i < parameters.size(); i++)
        {
            if (parameters[i] == "all-types")
                all = true;
            else if (parameters[i] == "non-trader")
                non_trader = true;
            else
                return CR_WRONG_USAGE;
        }

        auto *player = getPlayerNemesis(out, false);
        if (!player)
            return CR_FAILURE;

        df::coord player_pos = player->unit->pos;

        int total = 0;
        std::map<df::coord,int> counts;

        auto &items = world->items.all;
        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            int num = containsMetalItems(item, all, non_trader);
            if (!num)
                continue;

            df::map_block *block = Maps::getTileBlock(item->pos);
            if (!block)
                continue;

            total += num;
            counts[(item->pos - player_pos)/10] += num;

            auto &designations = block->designation;
            auto &dgn = designations[item->pos.x%16][item->pos.y%16];

            dgn.bits.hidden = 0; // revealed
            dgn.bits.pile = 1; // visible
        }

        joinCounts(counts);

        out.print("%d items of metal merchandise found in the vicinity.\n", total);
        for (auto it = counts.begin(); it != counts.end(); it++)
        {
            if (!it->second)
                continue;

            df::coord delta = it->first * 10;
            out.print("  %s: %d\n", formatDirection(delta).c_str(), it->second);
        }

        return CR_OK;
    }
    else
        return CR_WRONG_USAGE;
}
