// A container for random minor tweaks that don't fit anywhere else,
// in order to avoid creating lots of plugins and polluting the namespace.

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "modules/Filesystem.h"

#include "MiscUtils.h"

#include "DataDefs.h"
#include <VTableInterpose.h>
#include "../uicommon.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/squad.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/identity.h"
#include "df/language_name.h"
#include "df/incident.h"
#include "df/crime.h"
#include "df/unit_inventory_item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_kitchenprefst.h"
#include "df/viewscreen_layer_unit_actionst.h"
#include "df/squad_order_trainst.h"
#include "df/ui_build_selector.h"
#include "df/ui_sidebar_menus.h"
#include "df/building_trapst.h"
#include "df/building_workshopst.h"
#include "df/item_actual.h"
#include "df/item_crafted.h"
#include "df/item_armorst.h"
#include "df/item_helmst.h"
#include "df/item_glovesst.h"
#include "df/item_shoesst.h"
#include "df/item_pantsst.h"
#include "df/item_liquid_miscst.h"
#include "df/item_powder_miscst.h"
#include "df/item_barst.h"
#include "df/item_threadst.h"
#include "df/item_clothst.h"
#include "df/spatter.h"
#include "df/layer_object.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_reagent_flags.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_layer_assigntradest.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/squad_position.h"
#include "df/job.h"
#include "df/general_ref_building_holderst.h"
#include "df/unit_health_info.h"
#include "df/activity_entry.h"
#include "df/activity_event_combat_trainingst.h"
#include "df/activity_event_individual_skill_drillst.h"
#include "df/activity_event_skill_demonstrationst.h"
#include "df/activity_event_sparringst.h"
//#include "df/building_hivest.h"

#include <stdlib.h>
#include <unordered_map>

#include "tweaks/adamantine-cloth-wear.h"
#include "tweaks/advmode-contained.h"
#include "tweaks/civ-agreement-ui.h"
#include "tweaks/craft-age-wear.h"
#include "tweaks/eggs-fertile.h"
#include "tweaks/embark-profile-name.h"
#include "tweaks/farm-plot-select.h"
#include "tweaks/fast-heat.h"
#include "tweaks/fast-trade.h"
#include "tweaks/fps-min.h"
#include "tweaks/import-priority-category.h"
#include "tweaks/kitchen-keys.h"
#include "tweaks/kitchen-prefs-color.h"
#include "tweaks/kitchen-prefs-empty.h"
#include "tweaks/manager-quantity.h"
#include "tweaks/max-wheelbarrow.h"
#include "tweaks/military-assign.h"
#include "tweaks/nestbox-color.h"
#include "tweaks/shift-8-scroll.h"
#include "tweaks/stable-cursor.h"
#include "tweaks/title-start-rename.h"
#include "tweaks/tradereq-pet-gender.h"

using std::set;
using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("tweak");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_area_map_width);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_menu_width);
REQUIRE_GLOBAL(ui_sidebar_menus);
REQUIRE_GLOBAL(ui_workshop_in_add);
REQUIRE_GLOBAL(world);

using namespace DFHack::Gui;

class tweak_onupdate_hookst {
public:
    typedef void(*T_callback)(void);
    tweak_onupdate_hookst(std::string name_, T_callback cb)
        :name(name_), callback(cb), enabled(false) {}
    bool enabled;
    std::string name;
    T_callback callback;
};
static command_result tweak(color_ostream &out, vector <string> & parameters);
static std::multimap<std::string, VMethodInterposeLinkBase> tweak_hooks;
static std::multimap<std::string, tweak_onupdate_hookst> tweak_onupdate_hooks;

#define TWEAK_HOOK(tweak, cls, func) tweak_hooks.insert(std::pair<std::string, VMethodInterposeLinkBase>\
    (tweak, INTERPOSE_HOOK(cls, func)))
#define TWEAK_ONUPDATE_HOOK(tweak, func) tweak_onupdate_hooks.insert(\
    std::pair<std::string, tweak_onupdate_hookst>(tweak, tweak_onupdate_hookst(#func, func)))

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    is_enabled = true; // Allow plugin_onupdate to work (subcommands are enabled individually)
    commands.push_back(PluginCommand(
        "tweak", "Various tweaks for minor bugs.", tweak, false,
        "  tweak clear-missing\n"
        "    Remove the missing status from the selected unit.\n"
        "  tweak clear-ghostly\n"
        "    Remove the ghostly status from the selected unit.\n"
        "    Intended to fix the case where you can't engrave memorials for ghosts.\n"
        "    Note that this is very dirty and possibly dangerous!\n"
        "    Most probably does not have the positive effect of a proper burial.\n"
        "  tweak fixmigrant\n"
        "    Remove the resident/merchant flag from the selected unit.\n"
        "    Intended to fix bugged migrants/traders who stay at the\n"
        "    map edge and don't enter your fort. Only works for\n"
        "    dwarves (or generally the player's race in modded games).\n"
        "  tweak makeown\n"
        "    Force selected unit to become a member of your fort.\n"
        "    Can be abused to grab caravan merchants and escorts, even if\n"
        "    they don't belong to the player's race. Foreign sentients\n"
        "    (humans, elves) can be put to work, but you can't assign rooms\n"
        "    to them and they don't show up in DwarfTherapist because the\n"
        "    game treats them like pets.\n"
        "  tweak stable-cursor [disable]\n"
        "    Keeps exact position of dwarfmode cursor during exits to main menu.\n"
        "    E.g. allows switching between t/q/k/d without losing position.\n"
        /*"  tweak fix-dimensions [disable]\n"
        "    Fixes subtracting small amount of thread/cloth/liquid from a stack\n"
        "    by splitting the stack and subtracting from the remaining single item.\n"*/
        "  tweak adamantine-cloth-wear [disable]\n"
        "    Stops adamantine clothing from wearing out while being worn (bug 6481).\n"
        "  tweak advmode-contained [disable]\n"
        "    Fixes custom reactions with container inputs in advmode. The issue is\n"
        "    that the screen tries to force you to select the contents separately\n"
        "    from the container. This forcefully skips child reagents.\n"
        "  tweak civ-view-agreement\n"
        "    Fixes overlapping text on the \"view agreement\" screen\n"
        "  tweak craft-age-wear [disable]\n"
        "    Makes cloth and leather items wear out at the correct rate (bug 6003).\n"
        "  tweak embark-profile-name [disable]\n"
        "    Allows the use of lowercase letters when saving embark profiles\n"
        "  tweak eggs-fertile [disable]\n"
        "    Displays a fertile/infertile indicator on nestboxes\n"
        "  tweak farm-plot-select [disable]\n"
        "    Adds \"Select all\" and \"Deselect all\" options to farm plot menus\n"
        "  tweak fast-heat <max-ticks>\n"
        "    Further improves temperature updates by ensuring that 1 degree of\n"
        "    item temperature is crossed in no more than specified number of frames\n"
        "    when updating from the environment temperature. Use 0 to disable.\n"
        "  tweak fast-trade [disable]\n"
        "    Makes Shift-Enter in the Move Goods to Depot and Trade screens select\n"
        "    the current item (fully, in case of a stack), and scroll down one line.\n"
        "  tweak fps-min [disable]\n"
        "    Fixes the in-game minimum FPS setting (bug 6277)\n"
        "  tweak import-priority-category [disable]\n"
        "    When meeting with a liaison, makes Shift+Left/Right arrow adjust\n"
        "    the priority of an entire category of imports.\n"
        "  tweak kitchen-keys [disable]\n"
        "    Fixes DF kitchen meal keybindings (bug 614)\n"
        "  tweak kitchen-prefs-color [disable]\n"
        "    Changes color of enabled items to green in kitchen preferences\n"
        "  tweak kitchen-prefs-empty [disable]\n"
        "    Fixes a layout issue with empty kitchen tabs (bug 9000)\n"
        "  tweak manager-quantity [disable]\n"
        "    Removes the limit of 30 jobs per manager order\n"
        "  tweak max-wheelbarrow [disable]\n"
        "    Allows assigning more than 3 wheelbarrows to a stockpile\n"
        "  tweak nestbox-color [disable]\n"
        "    Makes built nestboxes use the color of their material\n"
        "  tweak military-color-assigned [disable]\n"
        "    Color squad candidates already assigned to other squads in brown/green\n"
        "    to make them stand out more in the list.\n"
        "  tweak military-stable-assign [disable]\n"
        "    Preserve list order and cursor position when assigning to squad,\n"
        "    i.e. stop the rightmost list of the Positions page of the military\n"
        "    screen from constantly jumping to the top.\n"
        "  tweak shift-8-scroll [disable]\n"
        "    Gives Shift+8 (or *) priority when scrolling menus, instead of \n"
        "    scrolling the map\n"
        "  tweak title-start-rename [disable]\n"
        "    Adds a safe rename option to the title screen \"Start Playing\" menu\n"
        "  tweak tradereq-pet-gender [disable]\n"
        "    Displays the gender of pets in the trade request list\n"
//        "  tweak military-training [disable]\n"
//        "    Speed up melee squad training, removing inverse dependency on unit count.\n"
    ));

    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_armor_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_helm_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_gloves_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_shoes_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_pants_hook, incWearTimer);

    TWEAK_HOOK("advmode-contained", advmode_contained_hook, feed);

    TWEAK_HOOK("civ-view-agreement", civ_agreement_view_hook, render);

    TWEAK_HOOK("craft-age-wear", craft_age_wear_hook, ageItem);

    TWEAK_HOOK("eggs-fertile", egg_fertile_hook, render);

    TWEAK_HOOK("embark-profile-name", embark_profile_name_hook, feed);

    TWEAK_HOOK("farm-plot-select", farm_select_hook, feed);
    TWEAK_HOOK("farm-plot-select", farm_select_hook, render);

    TWEAK_HOOK("fast-heat", fast_heat_hook, updateTempFromMap);
    TWEAK_HOOK("fast-heat", fast_heat_hook, updateTemperature);
    TWEAK_HOOK("fast-heat", fast_heat_hook, adjustTemperature);

    TWEAK_HOOK("fast-trade", fast_trade_assign_hook, feed);
    TWEAK_HOOK("fast-trade", fast_trade_select_hook, feed);

    TWEAK_ONUPDATE_HOOK("fps-min", fps_min_hook);

    TWEAK_HOOK("import-priority-category", takerequest_hook, feed);
    TWEAK_HOOK("import-priority-category", takerequest_hook, render);

    TWEAK_HOOK("kitchen-keys", kitchen_keys_hook, feed);
    TWEAK_HOOK("kitchen-keys", kitchen_keys_hook, render);

    TWEAK_HOOK("kitchen-prefs-color", kitchen_prefs_color_hook, render);

    TWEAK_HOOK("kitchen-prefs-empty", kitchen_prefs_empty_hook, render);

    TWEAK_HOOK("manager-quantity", manager_quantity_hook, feed);

    TWEAK_HOOK("max-wheelbarrow", max_wheelbarrow_hook, render);
    TWEAK_HOOK("max-wheelbarrow", max_wheelbarrow_hook, feed);

    TWEAK_HOOK("military-color-assigned", military_assign_hook, render);

    TWEAK_HOOK("military-stable-assign", military_assign_hook, feed);

    TWEAK_HOOK("nestbox-color", nestbox_color_hook, drawBuilding);

    TWEAK_HOOK("shift-8-scroll", shift_8_scroll_hook, feed);

    TWEAK_HOOK("stable-cursor", stable_cursor_hook, feed);

    TWEAK_HOOK("title-start-rename", title_start_rename_hook, feed);
    TWEAK_HOOK("title-start-rename", title_start_rename_hook, render);

    TWEAK_HOOK("tradereq-pet-gender", pet_gender_hook, render);

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    for (auto it = tweak_onupdate_hooks.begin(); it != tweak_onupdate_hooks.end(); ++it)
    {
        tweak_onupdate_hookst hook = it->second;
        if (hook.enabled)
            hook.callback();
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

// to be called by tweak-fixmigrant
// units forced into the fort by removing the flags do not own their clothes
// which has the result that they drop all their clothes and become unhappy because they are naked
// so we need to make them own their clothes and add them to their uniform
command_result fix_clothing_ownership(color_ostream &out, df::unit* unit)
{
    // first, find one owned item to initialize the vtable
    bool vt_initialized = false;
    size_t numItems = world->items.all.size();
    for(size_t i=0; i< numItems; i++)
    {
        df::item * item = world->items.all[i];
        if(Items::getOwner(item))
        {
            vt_initialized = true;
            break;
        }
    }
    if(!vt_initialized)
    {
        out << "fix_clothing_ownership: could not initialize vtable!" << endl;
        return CR_FAILURE;
    }

    int fixcount = 0;
    for(size_t j=0; j<unit->inventory.size(); j++)
    {
        df::unit_inventory_item* inv_item = unit->inventory[j];
        df::item* item = inv_item->item;
        // unforbid items (for the case of kidnapping caravan escorts who have their stuff forbidden by default)
        inv_item->item->flags.bits.forbid = 0;
        if(inv_item->mode == df::unit_inventory_item::T_mode::Worn)
        {
            // ignore armor?
            // it could be leather boots, for example, in which case it would not be nice to forbid ownership
            //if(item->getEffectiveArmorLevel() != 0)
            //    continue;

            if(!Items::getOwner(item))
            {
                if(Items::setOwner(item, unit))
                {
                    // add to uniform, so they know they should wear their clothes
                    insert_into_vector(unit->military.uniforms[0], item->id);
                    fixcount++;
                }
                else
                    out << "could not change ownership for item!" << endl;
            }
        }
    }
    // clear uniform_drop (without this they would drop their clothes and pick them up some time later)
    unit->military.uniform_drop.clear();
    out << "ownership for " << fixcount << " clothes fixed" << endl;
    return CR_OK;
}

static void correct_dimension(df::item_actual *self, int32_t &delta, int32_t dim)
{
    // Zero dimension or remainder?
    if (dim <= 0 || self->stack_size <= 1) return;
    int rem = delta % dim;
    if (rem == 0) return;
    // If destroys, pass through
    int intv = delta / dim;
    if (intv >= self->stack_size) return;
    // Subtract int part
    delta = rem;
    self->stack_size -= intv;
    if (self->stack_size <= 1) return;

    // If kills the item or cannot split, round up.
    if (!self->flags.bits.in_inventory || !Items::getContainer(self))
    {
        delta = dim;
        return;
    }

    // Otherwise split the stack
    color_ostream_proxy out(Core::getInstance().getConsole());
    out.print("fix-dimensions: splitting stack #%d for delta %d.\n", self->id, delta);

    auto copy = self->splitStack(self->stack_size-1, true);
    if (copy) copy->categorize(true);
}

struct dimension_liquid_hook : df::item_liquid_miscst {
    typedef df::item_liquid_miscst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, subtractDimension, (int32_t delta))
    {
        correct_dimension(this, delta, dimension);
        return INTERPOSE_NEXT(subtractDimension)(delta);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dimension_liquid_hook, subtractDimension);

struct dimension_powder_hook : df::item_powder_miscst {
    typedef df::item_powder_miscst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, subtractDimension, (int32_t delta))
    {
        correct_dimension(this, delta, dimension);
        return INTERPOSE_NEXT(subtractDimension)(delta);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dimension_powder_hook, subtractDimension);

struct dimension_bar_hook : df::item_barst {
    typedef df::item_barst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, subtractDimension, (int32_t delta))
    {
        correct_dimension(this, delta, dimension);
        return INTERPOSE_NEXT(subtractDimension)(delta);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dimension_bar_hook, subtractDimension);

struct dimension_thread_hook : df::item_threadst {
    typedef df::item_threadst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, subtractDimension, (int32_t delta))
    {
        correct_dimension(this, delta, dimension);
        return INTERPOSE_NEXT(subtractDimension)(delta);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dimension_thread_hook, subtractDimension);

struct dimension_cloth_hook : df::item_clothst {
    typedef df::item_clothst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, subtractDimension, (int32_t delta))
    {
        correct_dimension(this, delta, dimension);
        return INTERPOSE_NEXT(subtractDimension)(delta);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dimension_cloth_hook, subtractDimension);

/*
// Unit updates are executed based on an action divisor variable,
// which is computed from the alive unit count and has range 10-100.
static int adjust_unit_divisor(int value) {
    return value*10/DF_GLOBAL_FIELD(ui, unit_action_divisor, 10);
}

static bool can_spar(df::unit *unit) {
    return unit->counters2.exhaustion <= 2000 && // actually 4000, but leave a gap
           (unit->status2.limbs_grasp_count > 0 || unit->status2.limbs_grasp_max == 0) &&
           (!unit->health || (unit->health->flags.whole&0x7FF) == 0) &&
           (!unit->job.current_job || unit->job.current_job->job_type != job_type::Rest);
}

static bool has_spar_inventory(df::unit *unit, df::job_skill skill)
{
    using namespace df::enums::job_skill;

    auto type = ENUM_ATTR(job_skill, type, skill);

    if (type == job_skill_class::MilitaryWeapon)
    {
        for (size_t i = 0; i < unit->inventory.size(); i++)
        {
            auto item = unit->inventory[i];
            if (item->mode == df::unit_inventory_item::Weapon &&
                item->item->getMeleeSkill() == skill)
                return true;
        }

        return false;
    }

    switch (skill) {
        case THROW:
        case RANGED_COMBAT:
            return false;

        case SHIELD:
            for (size_t i = 0; i < unit->inventory.size(); i++)
            {
                auto item = unit->inventory[i];
                if (item->mode == df::unit_inventory_item::Weapon &&
                    item->item->getType() == item_type::SHIELD)
                    return true;
            }
            return false;

        case ARMOR:
            for (size_t i = 0; i < unit->inventory.size(); i++)
            {
                auto item = unit->inventory[i];
                if (item->mode == df::unit_inventory_item::Worn &&
                    item->item->isArmorNotClothing())
                    return true;
            }
            return false;

        default:
            return true;
    }
}

struct military_training_ct_hook : df::activity_event_combat_trainingst {
    typedef df::activity_event_combat_trainingst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, process, (df::unit *unit))
    {
        auto act = df::activity_entry::find(activity_id);
        int cur_neid = act ? act->next_event_id : 0;
        int cur_oc = organize_counter;

        INTERPOSE_NEXT(process)(unit);

        // Shorten the time it takes to organize stuff, so that in
        // reality it remains the same instead of growing proportionally
        // to the unit count.
        if (organize_counter > cur_oc && organize_counter > 0)
            organize_counter = adjust_unit_divisor(organize_counter);

        if (act && act->next_event_id > cur_neid)
        {
            // New events were added. Check them.
            for (size_t i = 0; i < act->events.size(); i++)
            {
                auto event = act->events[i];
                if (event->flags.bits.dismissed || event->event_id < cur_neid)
                    continue;

                if (auto sp = strict_virtual_cast<df::activity_event_sparringst>(event))
                {
                    // Sparring has a problem in that all of its participants decrement
                    // the countdown variable. Fix this by multiplying it by the member count.
                    sp->countdown = sp->countdown * sp->participants.units.size();
                }
                else if (auto sd = strict_virtual_cast<df::activity_event_skill_demonstrationst>(event))
                {
                    // Adjust initial counter values
                    sd->train_countdown = adjust_unit_divisor(sd->train_countdown);
                    sd->wait_countdown = adjust_unit_divisor(sd->wait_countdown);

                    // Check if the game selected the most skilled unit as the teacher
                    auto &units = sd->participants.units;
                    int maxv = -1, cur_xp = -1, minv = 0;
                    int best = -1;
                    size_t spar = 0;

                    for (size_t j = 0; j < units.size(); j++)
                    {
                        auto unit = df::unit::find(units[j]);
                        if (!unit) continue;
                        int xp = Units::getExperience(unit, sd->skill, true);
                        if (units[j] == sd->unit_id)
                            cur_xp = xp;
                        if (j == 0 || xp < minv)
                            minv = xp;
                        if (xp > maxv) {
                            maxv = xp;
                            best = j;
                        }
                        if (can_spar(unit) && has_spar_inventory(unit, sd->skill))
                            spar++;
                    }

#if 0
                    color_ostream_proxy out(Core::getInstance().getConsole());
#endif

                    // If the xp gap is low, sometimes replace with sparring
                    if ((maxv - minv) < 64*15 && spar == units.size() &&
                        random_int(45) >= 30 + (maxv-minv)/64)
                    {
#if 0
                        out.print("Replacing %s demonstration (xp %d-%d, gap %d) with sparring.\n",
                                  ENUM_KEY_STR(job_skill, sd->skill).c_str(), minv, maxv, maxv-minv);
#endif

                        if (auto spar = df::allocate<df::activity_event_sparringst>())
                        {
                            spar->event_id = sd->event_id;
                            spar->activity_id = sd->activity_id;
                            spar->parent_event_id = sd->parent_event_id;
                            spar->flags = sd->flags;
                            spar->participants = sd->participants;
                            spar->building_id = sd->building_id;
                            spar->countdown = 300*units.size();

                            delete sd;
                            act->events[i] = spar;

                            continue;
                        }
                    }

                    // If the teacher has less xp than somebody else, switch
                    if (best >= 0 && maxv > cur_xp)
                    {
#if 0
                        out.print("Replacing %s teacher %d (%d xp) with %d (%d xp); xp gap %d.\n",
                                  ENUM_KEY_STR(job_skill, sd->skill).c_str(),
                                  sd->unit_id, cur_xp, units[best], maxv, maxv-minv);
#endif

                        sd->hist_figure_id = sd->participants.histfigs[best];
                        sd->unit_id = units[best];
                    }
                    else
                    {
#if 0
                        out.print("Not changing %s demonstration (xp %d-%d, gap %d).\n",
                                  ENUM_KEY_STR(job_skill, sd->skill).c_str(),
                                  minv, maxv, maxv-minv);
#endif
                    }
                }
            }
        }
    }
};
*/

/*
IMPLEMENT_VMETHOD_INTERPOSE(military_training_ct_hook, process);

struct military_training_sd_hook : df::activity_event_skill_demonstrationst {
    typedef df::activity_event_skill_demonstrationst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, process, (df::unit *unit))
    {
        int cur_oc = organize_counter;
        int cur_tc = train_countdown;

        INTERPOSE_NEXT(process)(unit);

        // Shorten the counters if they changed
        if (organize_counter > cur_oc && organize_counter > 0)
            organize_counter = adjust_unit_divisor(organize_counter);
        if (train_countdown > cur_tc)
            train_countdown = adjust_unit_divisor(train_countdown);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(military_training_sd_hook, process);

template<class T>
bool is_done(T *event, df::unit *unit)
{
    return event->flags.bits.dismissed ||
           binsearch_index(event->participants.units, unit->id) < 0;
}

struct military_training_sp_hook : df::activity_event_sparringst {
    typedef df::activity_event_sparringst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, process, (df::unit *unit))
    {
        INTERPOSE_NEXT(process)(unit);

        // Since there are no counters to fix, repeat the call
        int cnt = (DF_GLOBAL_FIELD(ui, unit_action_divisor, 10)+5) / 10;
        for (int i = 1; i < cnt && !is_done(this, unit); i++)
            INTERPOSE_NEXT(process)(unit);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(military_training_sp_hook, process);

struct military_training_id_hook : df::activity_event_individual_skill_drillst {
    typedef df::activity_event_individual_skill_drillst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, process, (df::unit *unit))
    {
        INTERPOSE_NEXT(process)(unit);

        // Since there are no counters to fix, repeat the call
        int cnt = (DF_GLOBAL_FIELD(ui, unit_action_divisor, 10)+5) / 10;
        for (int i = 1; i < cnt && !is_done(this, unit); i++)
            INTERPOSE_NEXT(process)(unit);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(military_training_id_hook, process);
*/

static void enable_hook(color_ostream &out, VMethodInterposeLinkBase &hook, vector <string> &parameters)
{
    if (vector_get(parameters, 1) == "disable")
    {
        hook.remove();
        fprintf(stderr, "Disabled tweak %s (%s)\n", parameters[0].c_str(), hook.name());
        fflush(stderr);
    }
    else
    {
        if (hook.apply())
        {
            fprintf(stderr, "Enabled tweak %s (%s)\n", parameters[0].c_str(), hook.name());
            fflush(stderr);
        }
        else
            out.printerr("Could not activate tweak %s (%s)\n", parameters[0].c_str(), hook.name());
    }
}

static command_result enable_tweak(string tweak, color_ostream &out, vector <string> &parameters)
{
    bool recognized = false;
    string cmd = parameters[0];
    for (auto it = tweak_hooks.begin(); it != tweak_hooks.end(); ++it)
    {
        if (it->first == cmd)
        {
            recognized = true;
            enable_hook(out, it->second, parameters);
        }
    }
    for (auto it = tweak_onupdate_hooks.begin(); it != tweak_onupdate_hooks.end(); ++it)
    {
        if (it->first == cmd)
        {
            bool state = (vector_get(parameters, 1) != "disable");
            recognized = true;
            tweak_onupdate_hookst &hook = it->second;
            hook.enabled = state;
            fprintf(stderr, "%s tweak %s (%s)\n", state ? "Enabled" : "Disabled", cmd.c_str(), hook.name.c_str());
            fflush(stderr);
        }
    }
    if (!recognized)
    {
        out.printerr("Unrecognized tweak: %s\n", cmd.c_str());
        out.print("Run 'help tweak' to display a full list\n");
        return CR_FAILURE; // Avoid dumping usage information
    }
    return CR_OK;
}

static command_result tweak(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "clear-missing")
    {
        df::unit *unit = getSelectedUnit(out, true);
        if (!unit)
            return CR_FAILURE;

        auto death = df::incident::find(unit->counters.death_id);

        if (death)
        {
            death->flags.bits.discovered = true;

            auto crime = df::crime::find(death->crime_id);
            if (crime)
                crime->flags.bits.discovered = true;
        }
    }
    else if (cmd == "clear-ghostly")
    {
        df::unit *unit = getSelectedUnit(out, true);
        if (!unit)
            return CR_FAILURE;

        // don't accidentally kill non-ghosts!
        if (unit->flags3.bits.ghostly)
        {
            // remove ghostly, set to dead instead
            unit->flags3.bits.ghostly = 0;
            unit->flags1.bits.dead = 1;
        }
        else
        {
            out.print("That's not a ghost!\n");
            return CR_FAILURE;
        }
    }
    else if (cmd == "fixmigrant")
    {
        df::unit *unit = getSelectedUnit(out, true);
        if (!unit)
            return CR_FAILURE;

        if(!Units::isOwnRace(unit))
        {
            out << "Selected unit does not belong to your race!" << endl;
            return CR_FAILURE;
        }

        // case #1: migrants who have the resident flag set
        // see http://dffd.wimbli.com/file.php?id=6139 for a save
        if (unit->flags2.bits.resident)
            unit->flags2.bits.resident = 0;

        // case #2: migrants who have the merchant flag
        // happens on almost all maps after a few migrant waves
        if(Units::isMerchant(unit))
            unit->flags1.bits.merchant = 0;

        // this one is a cheat, but bugged migrants usually have the same civ_id
        // so it should not be triggered in most cases
        // if it happens that the player has 'foreign' units of the same race
        // (vanilla df: dwarves not from mountainhome) on his map, just grab them
        if(!Units::isOwnCiv(unit))
            unit->civ_id = ui->civ_id;

        return fix_clothing_ownership(out, unit);
    }
    else if (cmd == "makeown")
    {
        // force a unit into your fort, regardless of civ or race
        // allows to "steal" caravan guards etc
        df::unit *unit = getSelectedUnit(out, true);
        if (!unit)
            return CR_FAILURE;

        if (unit->flags2.bits.resident)
            unit->flags2.bits.resident = 0;
        if(Units::isMerchant(unit))
            unit->flags1.bits.merchant = 0;
        if(Units::isForest(unit))
            unit->flags1.bits.forest = 0;
        if(!Units::isOwnCiv(unit))
            unit->civ_id = ui->civ_id;
        if(unit->profession == df::profession::MERCHANT)
            unit->profession = df::profession::TRADER;
        if(unit->profession2 == df::profession::MERCHANT)
            unit->profession2 = df::profession::TRADER;
        return fix_clothing_ownership(out, unit);
    }
    else if (cmd == "fast-heat")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;
        max_heat_ticks = atoi(parameters[1].c_str());
        if (max_heat_ticks <= 0)
            parameters[1] = "disable";
        enable_tweak(cmd, out, parameters);
        return CR_OK;
    }
    /*else if (cmd == "fix-dimensions")
    {
        enable_hook(out, INTERPOSE_HOOK(dimension_liquid_hook, subtractDimension), parameters);
        enable_hook(out, INTERPOSE_HOOK(dimension_powder_hook, subtractDimension), parameters);
        enable_hook(out, INTERPOSE_HOOK(dimension_bar_hook, subtractDimension), parameters);
        enable_hook(out, INTERPOSE_HOOK(dimension_thread_hook, subtractDimension), parameters);
        enable_hook(out, INTERPOSE_HOOK(dimension_cloth_hook, subtractDimension), parameters);
    }*/
/*
    else if (cmd == "military-training")
    {
        enable_hook(out, INTERPOSE_HOOK(military_training_ct_hook, process), parameters);
        enable_hook(out, INTERPOSE_HOOK(military_training_sd_hook, process), parameters);
        enable_hook(out, INTERPOSE_HOOK(military_training_sp_hook, process), parameters);
        enable_hook(out, INTERPOSE_HOOK(military_training_id_hook, process), parameters);
    }*/
    else
    {
        return enable_tweak(cmd, out, parameters);
    }

    return CR_OK;
}
