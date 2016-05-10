// Fixes containers in barracks to actually work as intended.

#include "Export.h"
#include "DataDefs.h"
#include "Core.h"
#include "Console.h"
#include "MiscUtils.h"
#include "PluginManager.h"
#include <VTableInterpose.h>

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/Persistent.h"
#include "modules/Screen.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/barrack_preference_category.h"
#include "df/building_armorstandst.h"
#include "df/building_boxst.h"
#include "df/building_cabinetst.h"
#include "df/building_squad_use.h"
#include "df/building_weaponrackst.h"
#include "df/general_ref_building_destinationst.h"
#include "df/general_ref_building_holderst.h"
#include "df/item_ammost.h"
#include "df/item_armorst.h"
#include "df/item_backpackst.h"
#include "df/item_flaskst.h"
#include "df/item_glovesst.h"
#include "df/item_helmst.h"
#include "df/item_pantsst.h"
#include "df/item_quiverst.h"
#include "df/item_shieldst.h"
#include "df/item_shoesst.h"
#include "df/item_weaponst.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/ui.h"
#include "df/squad.h"
#include "df/squad_ammo_spec.h"
#include "df/squad_position.h"
#include "df/unit.h"
#include "df/world.h"

#include <iostream>
#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using std::cerr;
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

static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void load_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

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

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    if (Core::getInstance().isMapLoaded())
        plugin_onstatechange(out, SC_MAP_LOADED);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    if (DFHack::Core::getInstance().isMapLoaded())
        save_config(out);
    return CR_OK;
}

/*
 * PART 1 - Stop restockpiling of items stored in the armory.
 *
 * For everything other than ammo this is quite straightforward,
 * since the uniform switch code already tries to store items
 * in barracks containers, and it is thus known what the intention
 * is. Moreover these containers know which squad and member they
 * belong to.
 *
 * For ammo there is no such code (in fact, ammo is never removed
 * from a quiver, except when it is dropped itself), so I had to
 * apply some improvisation. There is one place where BOX containers
 * with Squad Equipment set are used as an anchor location for a
 * pathfinding check when assigning ammo, so presumably that's
 * the correct place. I however wanted to also differentiate
 * training ammo, so came up with the following rules:
 *
 *  1.  Combat ammo and ammo without any allowed use can be stored
 *      in BOXes marked for Squad Equipment, either directly or via
 *      containing room. No-allowed-use ammo is assumed to be reserved
 *      for emergency combat use, or something like that; however if
 *      it is already stored in a training chest, it won't be moved.
 *  1a. If assigned to a squad position, that box can be used _only_
 *      for ammo assigned to that specific _squad_. Otherwise, if
 *      multiple squads can use this room, they will store their
 *      ammo all mixed up.
 *  2.  Training ammo can be stored in BOXes within archery ranges
 *      (designated from archery target) that are enabled for Training.
 *      Train-only ammo in particular can _only_ be stored in such
 *      boxes. The inspiration for this comes from some broken code
 *      for weapon racks in Training rooms.
 *
 * As an additional feature (partially needed due to the constraints
 * of working from an external hack), this plugin also blocks instant
 * queueing of stockpiling jobs for items blocked on the ground, if
 * these items are assigned to any squad.
 *
 * Since there apparently still are bugs that cause uniform items to be
 * momentarily dropped on ground, this delay is set not to the minimally
 * necessary 50 ticks, but to 0.5 - 1.0 in-game days, so as to provide a
 * grace period during which the items can be instantly picked up again.
 */

// Completely block the use of stockpiles
#define NO_STOCKPILES

// Check if the item is assigned to any use controlled by the military tab
static bool is_assigned_item(df::item *item)
{
    if (!ui)
        return false;

    auto type = item->getType();
    int idx = binsearch_index(ui->equipment.items_assigned[type], item->id);
    if (idx < 0)
        return false;

    return true;
}

// Check if this ammo item is assigned to this squad with one of the specified uses
static bool is_squad_ammo(df::item *item, df::squad *squad, bool combat, bool train)
{
    for (size_t i = 0; i < squad->ammunition.size(); i++)
    {
        auto spec = squad->ammunition[i];
        bool cs = spec->flags.bits.use_combat;
        bool ts = spec->flags.bits.use_training;

        // no-use ammo assumed to fit any category
        if (((cs || !ts) && combat) || ((ts || !cs) && train))
        {
            if (binsearch_index(spec->assigned, item->id) >= 0)
                return true;
        }
    }

    return false;
}

// Recursively check room parents to find out if this ammo item is allowed here
static bool can_store_ammo_rec(df::item *item, df::building *holder, int squad_id)
{
    auto squads = holder->getSquads();

    if (squads)
    {
        for (size_t i = 0; i < squads->size(); i++)
        {
            auto use = (*squads)[i];

            // For containers assigned to a squad, only consider that squad
            if (squad_id >= 0 && use->squad_id != squad_id)
                continue;

            // Squad Equipment -> combat
            bool combat = use->mode.bits.squad_eq;
            bool train = false;

            if (combat || train)
            {
                auto squad = df::squad::find(use->squad_id);

                if (squad && is_squad_ammo(item, squad, combat, train))
                    return true;
            }
        }
    }
    // Ugh, archery targets don't actually have a squad use vector
    else if (holder->getType() == building_type::ArcheryTarget)
    {
        auto &squads = world->squads.all;

        for (size_t si = 0; si < squads.size(); si++)
        {
            auto squad = squads[si];

            // For containers assigned to a squad, only consider that squad
            if (squad_id >= 0 && squad->id != squad_id)
                continue;

            for (size_t j = 0; j < squad->rooms.size(); j++)
            {
                auto use = squad->rooms[j];

                if (use->building_id != holder->id)
                    continue;

                // Squad Equipment -> combat
                bool combat = use->mode.bits.squad_eq;
                // Archery target with Train -> training
                bool train = use->mode.bits.train;

                if (combat || train)
                {
                    if (is_squad_ammo(item, squad, combat, train))
                        return true;
                }

                break;
            }
        }
    }

    for (size_t i = 0; i < holder->parents.size(); i++)
        if (can_store_ammo_rec(item, holder->parents[i], squad_id))
            return true;

    return false;
}

// Check if the ammo item can be stored in this container
static bool can_store_ammo(df::item *item, df::building *holder)
{
    // Only chests
    if (holder->getType() != building_type::Box)
        return false;

    // with appropriate flags set
    return can_store_ammo_rec(item, holder, holder->getSpecificSquad());
}

// Check if the item is assigned to the squad member who owns this armory building
static bool belongs_to_position(df::item *item, df::building *holder)
{
    int sid = holder->getSpecificSquad();
    if (sid < 0)
        return false;

    auto squad = df::squad::find(sid);
    if (!squad)
        return false;

    int position = holder->getSpecificPosition();

    // Weapon racks belong to the whole squad, i.e. can be used by any position
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

// Check if the item is appropriately stored in an armory building
static bool is_in_armory(df::item *item)
{
    if (item->flags.bits.in_inventory || item->flags.bits.on_ground)
        return false;

    auto holder = Items::getHolderBuilding(item);
    if (!holder)
        return false;

    // If indeed in a building, check if it is the right one
    if (item->getType() == item_type::AMMO)
        return can_store_ammo(item, holder);
    else
        return belongs_to_position(item, holder);
}

/*
 * Hooks used to affect stockpiling code as it runs, and prevent it
 * from doing unwanted stuff.
 *
 * Toady can simply add these checks directly to the stockpiling code;
 * we have to abuse some handy item vmethods.
 */

template<class Item> struct armory_hook : Item {
    typedef Item interpose_base;

    /*
     * This vmethod is called by the actual stockpiling code before it
     * tries to queue a job, and is normally used to prevent stockpiling
     * of uncollected webs.
     */
    DEFINE_VMETHOD_INTERPOSE(bool, isCollected, ())
    {
#ifdef NO_STOCKPILES
        /*
         * Completely block any items assigned to a squad from being stored
         * in stockpiles. The reason is that I still observe haulers running
         * around with bins to pick them up for some reason. There could be
         * some unaccounted race conditions involved.
         */
        if (is_assigned_item(this))
            return false;
#else
        // Block stockpiling of items in the armory.
        if (is_in_armory(this))
            return false;

        /*
         * When an item is removed from inventory due to Pickup Equipment
         * process, the unit code directly invokes the stockpiling code
         * and thus creates the job even before the item is actually dropped
         * on the ground. We don't want this at all, especially due to the
         * grace period idea.
         *
         * With access to source, that code can just be changed to simply
         * drop the item on ground, without running stockpiling code.
         */
        if (this->flags.bits.in_inventory)
        {
            auto holder = Items::getHolderUnit(this);

            // When that call happens, the item is still in inventory
            if (holder && is_assigned_item(this))
            {
                // And its ID is is this vector
                if (::binsearch_index(holder->military.uniform_drop, this->id) >= 0)
                    return false;
            }
        }
#endif

        // Call the original vmethod
        return INTERPOSE_NEXT(isCollected)();
    }

    /*
     * This vmethod is used to actually put the item on the ground.
     * When it does that, it also adds it to a vector of items to be
     * instanly restockpiled by a loop in another part of the code.
     *
     * We don't want this either, even more than when removing from
     * uniform, because this can happen in lots of various situations,
     * including deconstructed containers etc, and we want our own
     * armory code below to have a chance to look at the item.
     *
     * The logical place for this code is in the loop that processes
     * that vector, but that part is not virtual.
     */
    DEFINE_VMETHOD_INTERPOSE(bool, moveToGround, (int16_t x, int16_t y, int16_t z))
    {
        // First, let it do its work
        bool rv = INTERPOSE_NEXT(moveToGround)(x, y, z);

        // Prevent instant restockpiling of dropped assigned items.
        if (is_assigned_item(this))
        {
            // The original vmethod adds the item to this vector to force instant check
            auto &ovec = world->items.other[items_other_id::ANY_RECENTLY_DROPPED];

            // If it is indeed there, remove it
            if (erase_from_vector(ovec, &df::item::id, this->id))
            {
                // and queue it to be checked normally in 0.5-1 in-game days
                // (this is a grace period in case the uniform is dropped just
                // for a moment due to a momentary glitch)
                this->stockpile_countdown = 12 + random_int(12);
                this->stockpile_delay = 0;
            }
        }

        return rv;
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
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_ammost>, isCollected);

template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_weaponst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_armorst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_helmst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_shoesst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_pantsst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_glovesst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_shieldst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_flaskst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_backpackst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_quiverst>, moveToGround);
template<> IMPLEMENT_VMETHOD_INTERPOSE(armory_hook<df::item_ammost>, moveToGround);

/*
 * PART 2 - Actively queue jobs to haul assigned items to the armory.
 *
 * The logical place for this of course is in the same code that decides
 * to put stuff in stockpiles, alongside the checks to prevent moving
 * stuff away from the armory. We just run it independently every 50
 * simulation frames.
 */

// Check if this item is loose and can be moved to armory
static bool can_store_item(df::item *item)
{
    // bad id, or cooldown timer still counting
    if (!item || item->stockpile_countdown > 0)
        return false;

    // bad flags?
    if (item->flags.bits.in_job ||
        item->flags.bits.removed ||
        item->flags.bits.in_building ||
        item->flags.bits.encased ||
        item->flags.bits.owned ||
        item->flags.bits.forbid ||
        item->flags.bits.on_fire)
        return false;

    // in unit inventory?
    auto top = item;

    while (top->flags.bits.in_inventory)
    {
        auto parent = Items::getContainer(top);
        if (!parent) break;
        top = parent;
    }

    if (Items::getGeneralRef(top, general_ref_type::UNIT_HOLDER))
        return false;

    // already in armory?
    if (is_in_armory(item))
        return false;

    return true;
}

// Queue a job to store the item in the building, if possible
static bool try_store_item(df::building *target, df::item *item)
{
    // Check if the dwarves can path between the target and the item
    df::coord tpos(target->centerx, target->centery, target->z);
    df::coord ipos = Items::getPosition(item);

    if (!Maps::canWalkBetween(tpos, ipos))
        return false;

    // Check if the target has enough space left
    if (!target->canStoreItem(item, true))
        return false;

    // Create the job
    auto href = df::allocate<df::general_ref_building_holderst>();
    if (!href)
        return false;

    auto job = new df::job();

    job->pos = tpos;

    bool dest = false;

    // Choose the job type - correct matching is needed so that
    // later canStoreItem calls would take the job into account.
    switch (target->getType()) {
        case building_type::Weaponrack:
            job->job_type = job_type::StoreWeapon;
            // Without this flag dwarves will pick up the item, and
            // then dismiss the job and put it back into the stockpile:
            job->flags.bits.specific_dropoff = true;
            break;
        case building_type::Armorstand:
            job->job_type = job_type::StoreArmor;
            job->flags.bits.specific_dropoff = true;
            break;
        case building_type::Cabinet:
            job->job_type = job_type::StoreOwnedItem;
            dest = true;
            break;
        default:
            job->job_type = job_type::StoreItemInHospital;
            dest = true;
            break;
    }

    // job <-> item link
    if (!Job::attachJobItem(job, item, df::job_item_ref::Hauled))
    {
        delete job;
        delete href;
        return false;
    }

    // job <-> building link
    href->building_id = target->id;
    target->jobs.push_back(job);
    job->general_refs.push_back(href);

    // Two of the jobs need this link to find the job in canStoreItem().
    // They also don't actually need BUILDING_HOLDER, but it doesn't hurt.
    if (dest)
    {
        auto rdest = df::allocate<df::general_ref_building_destinationst>();

        if (rdest)
        {
            rdest->building_id = target->id;
            job->general_refs.push_back(rdest);
        }
    }

    // add to job list
    Job::linkIntoWorld(job);
    return true;
}

// Store the item into the first building in the list that would accept it.
static void try_store_item(std::vector<int32_t> &vec, df::item *item)
{
    for (size_t i = 0; i < vec.size(); i++)
    {
        auto target = df::building::find(vec[i]);
        if (!target)
            continue;

        if (try_store_item(target, item))
            return;
    }
}

// Store the items into appropriate armory buildings
static void try_store_item_set(std::vector<int32_t> &items, df::squad *squad, df::squad_position *pos)
{
    for (size_t j = 0; j < items.size(); j++)
    {
        auto item = df::item::find(items[j]);

        // not loose
        if (!can_store_item(item))
            continue;

        // queue jobs to put it in the appropriate container
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

// For storing ammo, use a data structure sorted by free space, to even out the load
typedef std::map<int, std::set<df::building*> > ammo_box_set;

// Enumerate boxes in the room, adding them to the set
static void index_boxes(df::building *root, ammo_box_set &group, int squad_id)
{
    if (root->getType() == building_type::Box)
    {
        int id = root->getSpecificSquad();

        if (id < 0 || id == squad_id)
        {
            //color_ostream_proxy out(Core::getInstance().getConsole());
            //out.print("%08x %d\n", unsigned(root), root->getFreeCapacity(true));

            group[root->getFreeCapacity(true)].insert(root);
        }
    }

    for (size_t i = 0; i < root->children.size(); i++)
        index_boxes(root->children[i], group, squad_id);
}

// Loop over the set from most empty to least empty
static bool try_store_ammo(df::item *item, ammo_box_set &group)
{
    int volume = item->getVolume();

    for (auto it = group.rbegin(); it != group.rend(); ++it)
    {
        if (it->first < volume)
            break;

        for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        {
            auto bld = *it2;

            if (try_store_item(bld, item))
            {
                it->second.erase(bld);
                group[bld->getFreeCapacity(true)].insert(bld);
                return true;
            }
        }
    }

    return false;
}

// Collect chests for ammo storage
static void index_ammo_boxes(df::squad *squad, ammo_box_set &train_set, ammo_box_set &combat_set)
{
    for (size_t j = 0; j < squad->rooms.size(); j++)
    {
        auto room = squad->rooms[j];
        auto bld = df::building::find(room->building_id);

        // Chests in rooms marked for Squad Equipment used for combat ammo
        if (room->mode.bits.squad_eq)
            index_boxes(bld, combat_set, squad->id);

        // Chests in archery ranges used for training ammo
        if (room->mode.bits.train && bld->getType() == building_type::ArcheryTarget)
            index_boxes(bld, train_set, squad->id);
    }
}

// Store ammo into appropriate chests
static void try_store_ammo(df::squad *squad)
{
    bool indexed = false;
    ammo_box_set train_set, combat_set;

    for (size_t i = 0; i < squad->ammunition.size(); i++)
    {
        auto spec = squad->ammunition[i];
        bool cs = spec->flags.bits.use_combat;
        bool ts = spec->flags.bits.use_training;

        for (size_t j = 0; j < spec->assigned.size(); j++)
        {
            auto item = df::item::find(spec->assigned[j]);

            // not loose
            if (!can_store_item(item))
                continue;

            // compute the maps lazily
            if (!indexed)
            {
                indexed = true;
                index_ammo_boxes(squad, train_set, combat_set);
            }

            // BUG: if the same container is in both sets,
            // when a job is queued, the free space in the other
            // set will not be updated, which could lead to uneven
            // loading - but not to overflowing the container!

            // As said above, combat goes into Squad Equipment
            if (cs && try_store_ammo(item, combat_set))
                continue;
            // Training goes into Archery Range with Train
            if (ts && try_store_ammo(item, train_set))
                continue;
            // No use goes into combat
            if (!(ts || cs) && try_store_ammo(item, combat_set))
                continue;
        }
    }
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_onupdate(color_ostream &out, state_change_event event)
{
    if (!is_enabled)
        return CR_OK;

    // Process every 50th frame, sort of like regular stockpiling does
    if (DF_GLOBAL_VALUE(cur_year_tick,1) % 50 != 0)
        return CR_OK;

    // Loop over squads
    auto &squads = world->squads.all;

    for (size_t si = 0; si < squads.size(); si++)
    {
        auto squad = squads[si];

        for (size_t i = 0; i < squad->positions.size(); i++)
        {
            auto pos = squad->positions[i];

            try_store_item_set(pos->assigned_items, squad, pos);
        }

        try_store_ammo(squad);
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
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_ammost>, isCollected), enable);

    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_weaponst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_armorst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_helmst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_pantsst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_shoesst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_glovesst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_shieldst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_flaskst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_backpackst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_quiverst>, moveToGround), enable);
    enable_hook(out, INTERPOSE_HOOK(armory_hook<df::item_ammost>, moveToGround), enable);
}

static void enable_plugin(color_ostream &out)
{
    /*auto entry = World::GetPersistentData("fix-armory/enabled", NULL);
    if (!entry.isValid())
    {
        out.printerr("Could not save the status.\n");
        return;
    }*/

    enable_hooks(out, true);
}

static void disable_plugin(color_ostream &out)
{
    //auto entry = World::GetPersistentData("fix-armory/enabled");
    //World::DeletePersistentData(entry);

    enable_hooks(out, false);
}

static void load_config(color_ostream& out) {
    Json::Value& p = Persistent::get("fix-armory");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        auto entry = World::GetPersistentData("fix-armory/enabled");
        bool enable = entry.isValid();
        World::DeletePersistentData(entry);
        enable_hooks(out,enable);
    } else if ( version == 1 ) {
        bool enable = p["is_enabled"].isBool() ? p["is_enabled"].asBool() : false;
        enable_hooks(out,enable);
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
}
static void save_config(color_ostream& out) {
    Json::Value& p = Persistent::get("fix-armory");
    p["version"] = persist_version;
    p["is_enabled"] = is_enabled;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        load_config(out);
        /*if (!gamemode || *gamemode == game_mode::DWARF)
        {
            bool enable = World::GetPersistentData("fix-armory/enabled").isValid();

            if (enable)
            {
                out.print("Enabling the fix-armory plugin.\n");
                enable_hooks(out, true);
            }
            else
                enable_hooks(out, false);
        }*/
        break;
    case SC_MAP_UNLOADED:
        enable_hooks(out, false);
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enable)
        enable_plugin(out);
    else
        disable_plugin(out);

    return CR_OK;
}

static command_result fix_armory(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "enable")
        return plugin_enable(out, true);
    else if (cmd == "disable")
        return plugin_enable(out, false);
    else
        return CR_WRONG_USAGE;

    return CR_OK;
}
