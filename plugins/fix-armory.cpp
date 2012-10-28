// Fixes containers in barracks to actually work as intended.

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/World.h"

#include "MiscUtils.h"

#include "DataDefs.h"
#include <VTableInterpose.h>
#include "df/ui.h"
#include "df/world.h"
#include "df/squad.h"
#include "df/unit.h"
#include "df/squad_position.h"
#include "df/item_weaponst.h"
#include "df/item_armorst.h"
#include "df/item_helmst.h"
#include "df/item_pantsst.h"
#include "df/item_shoesst.h"
#include "df/item_glovesst.h"
#include "df/item_shieldst.h"
#include "df/item_flaskst.h"
#include "df/item_backpackst.h"
#include "df/item_quiverst.h"
#include "df/building_weaponrackst.h"
#include "df/building_armorstandst.h"
#include "df/building_cabinetst.h"
#include "df/building_boxst.h"
#include "df/job.h"
#include "df/general_ref_building_holderst.h"
#include "df/barrack_preference_category.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;
using df::global::gamemode;
using df::global::ui_build_selector;
using df::global::ui_menu_width;
using df::global::ui_area_map_width;

using namespace DFHack::Gui;
using Screen::Pen;

static command_result fix_armory(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("fix-armory");

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "fix-armory", "Enables or disables the fix-armory plugin.", fix_armory, false,
        "  fix-armory enable\n"
        "    Enables the tweaks.\n"
        "  fix-armory disable\n"
        "    Disables the tweaks. All equipment will be hauled off to stockpiles.\n"
    ));

    if (Core::getInstance().isMapLoaded())
        plugin_onstatechange(out, SC_MAP_LOADED);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

static bool belongs_to_position(df::item *item, df::building *holder)
{
    int sid = holder->getSpecificSquad();
    if (sid < 0)
        return false;

    auto squad = df::squad::find(sid);
    if (!squad)
        return false;

    int position = holder->getSpecificPosition();

    if (position == -1 && holder->getType() == building_type::Weaponrack)
    {
        for (size_t i = 0; i < squad->positions.size(); i++)
        {
            if (binsearch_index(squad->positions[i]->assigned_items, item->id) >= 0)
                return true;
        }
    }
    else
    {
        auto cpos = vector_get(squad->positions, position);
        if (cpos && binsearch_index(cpos->assigned_items, item->id) >= 0)
            return true;
    }

    return false;
}

static bool is_in_armory(df::item *item)
{
    if (item->flags.bits.in_inventory || item->flags.bits.on_ground)
        return false;

    auto holder_ref = Items::getGeneralRef(item, general_ref_type::BUILDING_HOLDER);
    if (!holder_ref)
        return false;

    auto holder = holder_ref->getBuilding();
    if (!holder)
        return false;

    return belongs_to_position(item, holder);
}

template<class Item> struct armory_hook : Item {
    typedef Item interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, isCollected, ())
    {
        if (is_in_armory(this))
            return false;

        return INTERPOSE_NEXT(isCollected)();
    }
};

template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_weaponst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_armorst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_helmst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_shoesst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_pantsst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_glovesst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_shieldst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_flaskst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_backpackst>, isCollected);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_quiverst>, isCollected);

static bool can_store_item(df::item *item)
{
    if (!item || item->stockpile_countdown > 0)
        return false;

    if (item->flags.bits.in_job ||
        item->flags.bits.removed ||
        item->flags.bits.in_building ||
        item->flags.bits.encased ||
        item->flags.bits.owned ||
        item->flags.bits.forbid ||
        item->flags.bits.on_fire)
        return false;

    auto top = item;

    while (top->flags.bits.in_inventory)
    {
        auto parent = Items::getContainer(top);
        if (!parent) break;
        top = parent;
    }

    if (Items::getGeneralRef(top, general_ref_type::UNIT_HOLDER))
        return false;

    if (is_in_armory(item))
        return false;

    return true;
}

static void try_store_item(std::vector<int32_t> &vec, df::item *item)
{
    for (size_t i = 0; i < vec.size(); i++)
    {
        auto target = df::building::find(vec[i]);
        if (!target)
            continue;

        if (!target->canStoreItem(item, true))
            continue;

        auto href = df::allocate<df::general_ref_building_holderst>();
        if (!href)
            return;

        auto job = new df::job();

        job->pos = df::coord(target->centerx, target->centery, target->z);

        switch (target->getType()) {
            case building_type::Weaponrack:
                job->job_type = job_type::StoreWeapon;
                job->flags.bits.specific_dropoff = true;
                break;
            case building_type::Armorstand:
                job->job_type = job_type::StoreArmor;
                job->flags.bits.specific_dropoff = true;
                break;
            case building_type::Cabinet:
                job->job_type = job_type::StoreItemInCabinet;
                break;
            default:
                job->job_type = job_type::StoreItemInChest;
                break;
        }

        if (!Job::attachJobItem(job, item, df::job_item_ref::Hauled))
        {
            delete job;
            delete href;
            continue;
        }

        href->building_id = target->id;
        target->jobs.push_back(job);
        job->references.push_back(href);

        Job::linkIntoWorld(job);
        return;
    }
}

static void try_store_item_set(std::vector<int32_t> &items, df::squad *squad, df::squad_position *pos)
{
    for (size_t j = 0; j < items.size(); j++)
    {
        auto item = df::item::find(items[j]);

        if (!can_store_item(item))
            continue;

        if (item->isWeapon())
            try_store_item(squad->rack_combat, item);
        else if (item->isClothing())
            try_store_item(pos->preferences[barrack_preference_category::Cabinet], item);
        else if (item->isArmorNotClothing())
            try_store_item(pos->preferences[barrack_preference_category::Armorstand], item);
        else
            try_store_item(pos->preferences[barrack_preference_category::Box], item);
    }
}

static bool is_enabled = false;

DFhackCExport command_result plugin_onupdate(color_ostream &out, state_change_event event)
{
    if (!is_enabled)
        return CR_OK;

    if (DF_GLOBAL_VALUE(cur_year_tick,1) % 50 != 0)
        return CR_OK;

    auto &squads = df::global::world->squads.all;

    for (size_t si = 0; si < squads.size(); si++)
    {
        auto squad = squads[si];

        for (size_t i = 0; i < squad->positions.size(); i++)
        {
            auto pos = squad->positions[i];

            try_store_item_set(pos->assigned_items, squad, pos);
        }
    }

    return CR_OK;
}

static void enable_hook(color_ostream &out, VMethodInterposeLinkBase &hook, bool enable)
{
    if (!hook.apply(enable))
        out.printerr("Could not %s hook.\n", enable?"activate":"deactivate");
}

static void enable_hooks(color_ostream &out, bool enable)
{
    is_enabled = enable;

    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_weaponst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_armorst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_helmst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_pantsst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_shoesst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_glovesst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_shieldst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_flaskst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_backpackst>, isCollected), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_quiverst>, isCollected), enable);
}

static void enable_plugin(color_ostream &out)
{
    auto entry = World::GetPersistentData("fix-armory/enabled", NULL);
    if (!entry.isValid())
    {
        out.printerr("Could not save the status.\n");
        return;
    }

    enable_hooks(out, true);
}

static void disable_plugin(color_ostream &out)
{
    auto entry = World::GetPersistentData("fix-armory/enabled");
    World::DeletePersistentData(entry);

    enable_hooks(out, false);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        if (!gamemode || *gamemode == game_mode::DWARF)
        {
            bool enable = World::GetPersistentData("fix-armory/enabled").isValid();

            if (enable)
            {
                out.print("Enabling the fix-armory plugin.\n");
                enable_hooks(out, true);
            }
            else
                enable_hooks(out, false);
        }
        break;
    case SC_MAP_UNLOADED:
        enable_hooks(out, false);
    default:
        break;
    }

    return CR_OK;
}

static command_result fix_armory(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "enable")
    {
        enable_plugin(out);
    }
    else if (cmd == "disable")
    {
        disable_plugin(out);
    }
    else
        return CR_WRONG_USAGE;

    return CR_OK;
}
