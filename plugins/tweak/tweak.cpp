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
#include "df/plotinfost.h"
#include "df/world.h"
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
#include "df/buildreq.h"
#include "df/gamest.h"
#include "df/building_trapst.h"
#include "df/building_workshopst.h"
#include "df/item_actual.h"
#include "df/item_crafted.h"
#include "df/item_armorst.h"
#include "df/item_helmst.h"
#include "df/item_glovesst.h"
#include "df/item_shoesst.h"
#include "df/item_pantsst.h"
#include "df/item_drinkst.h"
#include "df/item_globst.h"
#include "df/item_liquid_miscst.h"
#include "df/item_powder_miscst.h"
#include "df/item_barst.h"
#include "df/item_threadst.h"
#include "df/item_clothst.h"
#include "df/item_sheetst.h"
#include "df/spatter.h"
#include "df/layer_object.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_reagent_flags.h"
#include "df/reaction_product_itemst.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_layer_assigntradest.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/job.h"
#include "df/general_ref_building_holderst.h"
#include "df/unit_health_info.h"
#include "df/caste_body_info.h"

#include <stdlib.h>
#include <unordered_map>

#include "tweaks/adamantine-cloth-wear.h"
#include "tweaks/advmode-contained.h"
#include "tweaks/block-labors.h"
#include "tweaks/burrow-name-cancel.h"
#include "tweaks/cage-butcher.h"
#include "tweaks/civ-agreement-plotinfo.h"
#include "tweaks/condition-material.h"
#include "tweaks/craft-age-wear.h"
#include "tweaks/do-job-now.h"
#include "tweaks/eggs-fertile.h"
#include "tweaks/embark-profile-name.h"
#include "tweaks/farm-plot-select.h"
#include "tweaks/fast-heat.h"
#include "tweaks/fast-trade.h"
#include "tweaks/fps-min.h"
#include "tweaks/hide-priority.h"
#include "tweaks/hotkey-clear.h"
#include "tweaks/import-priority-category.h"
#include "tweaks/kitchen-prefs-all.h"
#include "tweaks/kitchen-prefs-color.h"
#include "tweaks/kitchen-prefs-empty.h"
#include "tweaks/max-wheelbarrow.h"
#include "tweaks/military-assign.h"
#include "tweaks/nestbox-color.h"
#include "tweaks/partial-items.h"
#include "tweaks/pausing-fps-counter.h"
#include "tweaks/reaction-gloves.h"
#include "tweaks/shift-8-scroll.h"
#include "tweaks/stable-cursor.h"
#include "tweaks/stone-status-all.h"
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
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(ui_building_in_assign);
REQUIRE_GLOBAL(ui_building_in_resize);
REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_look_cursor);
REQUIRE_GLOBAL(ui_menu_width);
REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(ui_unit_view_mode);
REQUIRE_GLOBAL(ui_workshop_in_add);
REQUIRE_GLOBAL(world);

using namespace DFHack::Gui;

class tweak_onupdate_hookst {
public:
    typedef void(*T_callback)(void);
tweak_onupdate_hookst(std::string name_, T_callback cb)
        :enabled(false), name(name_), callback(cb) {}
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
        "tweak",
        "Various tweaks for minor bugs.",
        tweak));

    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_armor_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_helm_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_gloves_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_shoes_hook, incWearTimer);
    TWEAK_HOOK("adamantine-cloth-wear", adamantine_cloth_wear_pants_hook, incWearTimer);

    TWEAK_HOOK("advmode-contained", advmode_contained_hook, feed);

    TWEAK_HOOK("block-labors", block_labors_hook, feed);
    TWEAK_HOOK("block-labors", block_labors_hook, render);

    TWEAK_HOOK("burrow-name-cancel", burrow_name_cancel_hook, feed);

    TWEAK_HOOK("cage-butcher", cage_butcher_hook, feed);
    TWEAK_HOOK("cage-butcher", cage_butcher_hook, render);

    TWEAK_HOOK("civ-view-agreement", civ_agreement_view_hook, render);

    TWEAK_HOOK("condition-material", condition_material_hook, feed);

    TWEAK_HOOK("craft-age-wear", craft_age_wear_hook, ageItem);

    TWEAK_HOOK("do-job-now", do_job_now_hook, feed);
    TWEAK_HOOK("do-job-now", do_job_now_hook, render);

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

    TWEAK_HOOK("hide-priority", hide_priority_hook, feed);
    TWEAK_HOOK("hide-priority", hide_priority_hook, render);

    TWEAK_HOOK("hotkey-clear", hotkey_clear_hook, feed);
    TWEAK_HOOK("hotkey-clear", hotkey_clear_hook, render);

    TWEAK_HOOK("import-priority-category", takerequest_hook, feed);
    TWEAK_HOOK("import-priority-category", takerequest_hook, render);

    TWEAK_HOOK("kitchen-prefs-all", kitchen_prefs_all_hook, feed);
    TWEAK_HOOK("kitchen-prefs-all", kitchen_prefs_all_hook, render);

    TWEAK_HOOK("kitchen-prefs-color", kitchen_prefs_color_hook, render);

    TWEAK_HOOK("kitchen-prefs-empty", kitchen_prefs_empty_hook, render);

    TWEAK_HOOK("max-wheelbarrow", max_wheelbarrow_hook, render);
    TWEAK_HOOK("max-wheelbarrow", max_wheelbarrow_hook, feed);

    TWEAK_HOOK("military-color-assigned", military_assign_hook, render);

    TWEAK_HOOK("military-stable-assign", military_assign_hook, feed);

    TWEAK_HOOK("nestbox-color", nestbox_color_hook, drawBuilding);

    TWEAK_HOOK("partial-items", partial_items_hook_bar, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_drink, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_glob, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_liquid_misc, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_powder_misc, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_cloth, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_sheet, getItemDescription);
    TWEAK_HOOK("partial-items", partial_items_hook_thread, getItemDescription);

    TWEAK_HOOK("pausing-fps-counter", dwarfmode_pausing_fps_counter_hook, render);
    TWEAK_HOOK("pausing-fps-counter", title_pausing_fps_counter_hook, render);

    TWEAK_HOOK("reaction-gloves", reaction_gloves_hook, produce);

    TWEAK_HOOK("shift-8-scroll", shift_8_scroll_hook, feed);

    TWEAK_HOOK("stable-cursor", stable_cursor_hook, feed);

    TWEAK_HOOK("stone-status-all", stone_status_all_hook, feed);
    TWEAK_HOOK("stone-status-all", stone_status_all_hook, render);

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
            unit->flags1.bits.inactive = 1;
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
            unit->civ_id = plotinfo->civ_id;

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
            unit->civ_id = plotinfo->civ_id;
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
    else
    {
        return enable_tweak(cmd, out, parameters);
    }

    return CR_OK;
}
