#include "df/viewscreen_adopt_regionst.h"
#include "df/viewscreen_adventure_logst.h"
#include "df/viewscreen_announcelistst.h"
#include "df/viewscreen_assign_display_itemst.h"
#include "df/viewscreen_barterst.h"
#include "df/viewscreen_buildinglistst.h"
#include "df/viewscreen_buildingst.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_civlistst.h"
#include "df/viewscreen_counterintelligencest.h"
#include "df/viewscreen_createquotast.h"
#include "df/viewscreen_customize_unitst.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dungeon_monsterstatusst.h"
#include "df/viewscreen_dungeon_wrestlest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_entityst.h"
#include "df/viewscreen_export_graphical_mapst.h"
#include "df/viewscreen_export_regionst.h"
#include "df/viewscreen_game_cleanerst.h"
#include "df/viewscreen_image_creator_mode.h"
#include "df/viewscreen_image_creatorst.h"
#include "df/viewscreen_itemst.h"
#include "df/viewscreen_joblistst.h"
#include "df/viewscreen_jobmanagementst.h"
#include "df/viewscreen_jobst.h"
#include "df/viewscreen_justicest.h"
#include "df/viewscreen_kitchenpref_page.h"
#include "df/viewscreen_kitchenprefst.h"
#include "df/viewscreen_layer_arena_creaturest.h"
#include "df/viewscreen_layer_assigntradest.h"
#include "df/viewscreen_layer_choose_language_namest.h"
#include "df/viewscreen_layer_currencyst.h"
#include "df/viewscreen_layer_export_play_mapst.h"
#include "df/viewscreen_layer.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_layer_musicsoundst.h"
#include "df/viewscreen_layer_noblelistst.h"
#include "df/viewscreen_layer_overall_healthst.h"
#include "df/viewscreen_layer_reactionst.h"
#include "df/viewscreen_layer_squad_schedulest.h"
#include "df/viewscreen_layer_stockpilest.h"
#include "df/viewscreen_layer_stone_restrictionst.h"
#include "df/viewscreen_layer_unit_actionst.h"
#include "df/viewscreen_layer_unit_healthst.h"
#include "df/viewscreen_layer_unit_relationshipst.h"
#include "df/viewscreen_layer_world_gen_param_presetst.h"
#include "df/viewscreen_layer_world_gen_paramst.h"
#include "df/viewscreen_legendsst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_locationsst.h"
#include "df/viewscreen_meetingst.h"
#include "df/viewscreen_movieplayerst.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_noblest.h"
#include "df/viewscreen_optionst.h"
#include "df/viewscreen_overallstatusst.h"
#include "df/viewscreen_petitionsst.h"
#include "df/viewscreen_petst.h"
#include "df/viewscreen_pricest.h"
#include "df/viewscreen_reportlistst.h"
#include "df/viewscreen_requestagreementst.h"
#include "df/viewscreen_savegamest.h"
#include "df/viewscreen_selectitemst.h"
#include "df/viewscreen_setupadventurest.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_storesst.h"
#include "df/viewscreen_textviewerst.h"
#include "df/viewscreen_titlest.h"
#include "df/viewscreen_topicmeeting_fill_land_holder_positionsst.h"
#include "df/viewscreen_topicmeetingst.h"
#include "df/viewscreen_topicmeeting_takerequestsst.h"
#include "df/viewscreen_tradeagreementst.h"
#include "df/viewscreen_tradelistst.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_treasurelistst.h"
#include "df/viewscreen_unitlist_page.h"
#include "df/viewscreen_unitlistst.h"
#include "df/viewscreen_unitst.h"
#include "df/viewscreen_update_regionst.h"
#include "df/viewscreen_wagesst.h"
#include "df/viewscreen_workquota_conditionst.h"
#include "df/viewscreen_workquota_detailsst.h"
#include "df/viewscreen_workshop_profilest.h"

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

using namespace DFHack;

DFHACK_PLUGIN("overlay");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

namespace DFHack {
    DBG_DECLARE(overlay, control, DebugCategory::LINFO);
    DBG_DECLARE(overlay, event, DebugCategory::LINFO);
}

template<class T>
struct viewscreen_overlay : T {
    typedef T interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        INTERPOSE_NEXT(logic)();
    }
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input)) {
        INTERPOSE_NEXT(feed)(input);
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();
    }
};

#define IMPLEMENT_HOOKS(screen) \
    typedef viewscreen_overlay<df::viewscreen_##screen##st> screen##_overlay; \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, logic, 100); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, feed, 100); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, render, 100);

IMPLEMENT_HOOKS(adopt_region)
IMPLEMENT_HOOKS(adventure_log)
IMPLEMENT_HOOKS(announcelist)
IMPLEMENT_HOOKS(assign_display_item)
IMPLEMENT_HOOKS(barter)
IMPLEMENT_HOOKS(buildinglist)
IMPLEMENT_HOOKS(building)
IMPLEMENT_HOOKS(choose_start_site)
IMPLEMENT_HOOKS(civlist)
IMPLEMENT_HOOKS(counterintelligence)
IMPLEMENT_HOOKS(createquota)
IMPLEMENT_HOOKS(customize_unit)
IMPLEMENT_HOOKS(dungeonmode)
IMPLEMENT_HOOKS(dungeon_monsterstatus)
IMPLEMENT_HOOKS(dungeon_wrestle)
IMPLEMENT_HOOKS(dwarfmode)
IMPLEMENT_HOOKS(entity)
IMPLEMENT_HOOKS(export_graphical_map)
IMPLEMENT_HOOKS(export_region)
IMPLEMENT_HOOKS(game_cleaner)
IMPLEMENT_HOOKS(image_creator)
IMPLEMENT_HOOKS(item)
IMPLEMENT_HOOKS(joblist)
IMPLEMENT_HOOKS(jobmanagement)
IMPLEMENT_HOOKS(job)
IMPLEMENT_HOOKS(justice)
IMPLEMENT_HOOKS(kitchenpref)
IMPLEMENT_HOOKS(layer_arena_creature)
IMPLEMENT_HOOKS(layer_assigntrade)
IMPLEMENT_HOOKS(layer_choose_language_name)
IMPLEMENT_HOOKS(layer_currency)
IMPLEMENT_HOOKS(layer_export_play_map)
IMPLEMENT_HOOKS(layer_military)
IMPLEMENT_HOOKS(layer_musicsound)
IMPLEMENT_HOOKS(layer_noblelist)
IMPLEMENT_HOOKS(layer_overall_health)
IMPLEMENT_HOOKS(layer_reaction)
IMPLEMENT_HOOKS(layer_squad_schedule)
IMPLEMENT_HOOKS(layer_stockpile)
IMPLEMENT_HOOKS(layer_stone_restriction)
IMPLEMENT_HOOKS(layer_unit_action)
IMPLEMENT_HOOKS(layer_unit_health)
IMPLEMENT_HOOKS(layer_unit_relationship)
IMPLEMENT_HOOKS(layer_world_gen_param_preset)
IMPLEMENT_HOOKS(layer_world_gen_param)
IMPLEMENT_HOOKS(legends)
IMPLEMENT_HOOKS(loadgame)
IMPLEMENT_HOOKS(locations)
IMPLEMENT_HOOKS(meeting)
IMPLEMENT_HOOKS(movieplayer)
IMPLEMENT_HOOKS(new_region)
IMPLEMENT_HOOKS(noble)
IMPLEMENT_HOOKS(option)
IMPLEMENT_HOOKS(overallstatus)
IMPLEMENT_HOOKS(petitions)
IMPLEMENT_HOOKS(pet)
IMPLEMENT_HOOKS(price)
IMPLEMENT_HOOKS(reportlist)
IMPLEMENT_HOOKS(requestagreement)
IMPLEMENT_HOOKS(savegame)
IMPLEMENT_HOOKS(selectitem)
IMPLEMENT_HOOKS(setupadventure)
IMPLEMENT_HOOKS(setupdwarfgame)
IMPLEMENT_HOOKS(stores)
IMPLEMENT_HOOKS(textviewer)
IMPLEMENT_HOOKS(title)
IMPLEMENT_HOOKS(topicmeeting_fill_land_holder_positions)
IMPLEMENT_HOOKS(topicmeeting)
IMPLEMENT_HOOKS(topicmeeting_takerequests)
IMPLEMENT_HOOKS(tradeagreement)
IMPLEMENT_HOOKS(tradegoods)
IMPLEMENT_HOOKS(tradelist)
IMPLEMENT_HOOKS(treasurelist)
IMPLEMENT_HOOKS(unitlist)
IMPLEMENT_HOOKS(unit)
IMPLEMENT_HOOKS(update_region)
IMPLEMENT_HOOKS(wages)
IMPLEMENT_HOOKS(workquota_condition)
IMPLEMENT_HOOKS(workquota_details)
IMPLEMENT_HOOKS(workshop_profile)

#undef IMPLEMENT_HOOKS

#define INTERPOSE_HOOKS_FAILED(screen) \
    !INTERPOSE_HOOK(screen##_overlay, logic).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, feed).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, render).apply(enable)

DFhackCExport command_result plugin_enable(color_ostream &, bool enable) {
    if (is_enabled == enable)
        return CR_OK;

    DEBUG(control).print("%sing interpose hooks\n", enable ? "enabl" : "disabl");

    if (INTERPOSE_HOOKS_FAILED(adopt_region) ||
            INTERPOSE_HOOKS_FAILED(adventure_log) ||
            INTERPOSE_HOOKS_FAILED(announcelist) ||
            INTERPOSE_HOOKS_FAILED(assign_display_item) ||
            INTERPOSE_HOOKS_FAILED(barter) ||
            INTERPOSE_HOOKS_FAILED(buildinglist) ||
            INTERPOSE_HOOKS_FAILED(building) ||
            INTERPOSE_HOOKS_FAILED(choose_start_site) ||
            INTERPOSE_HOOKS_FAILED(civlist) ||
            INTERPOSE_HOOKS_FAILED(counterintelligence) ||
            INTERPOSE_HOOKS_FAILED(createquota) ||
            INTERPOSE_HOOKS_FAILED(customize_unit) ||
            INTERPOSE_HOOKS_FAILED(dungeonmode) ||
            INTERPOSE_HOOKS_FAILED(dungeon_monsterstatus) ||
            INTERPOSE_HOOKS_FAILED(dungeon_wrestle) ||
            INTERPOSE_HOOKS_FAILED(dwarfmode) ||
            INTERPOSE_HOOKS_FAILED(entity) ||
            INTERPOSE_HOOKS_FAILED(export_graphical_map) ||
            INTERPOSE_HOOKS_FAILED(export_region) ||
            INTERPOSE_HOOKS_FAILED(game_cleaner) ||
            INTERPOSE_HOOKS_FAILED(image_creator) ||
            INTERPOSE_HOOKS_FAILED(item) ||
            INTERPOSE_HOOKS_FAILED(joblist) ||
            INTERPOSE_HOOKS_FAILED(jobmanagement) ||
            INTERPOSE_HOOKS_FAILED(job) ||
            INTERPOSE_HOOKS_FAILED(justice) ||
            INTERPOSE_HOOKS_FAILED(kitchenpref) ||
            INTERPOSE_HOOKS_FAILED(layer_arena_creature) ||
            INTERPOSE_HOOKS_FAILED(layer_assigntrade) ||
            INTERPOSE_HOOKS_FAILED(layer_choose_language_name) ||
            INTERPOSE_HOOKS_FAILED(layer_currency) ||
            INTERPOSE_HOOKS_FAILED(layer_export_play_map) ||
            INTERPOSE_HOOKS_FAILED(layer_military) ||
            INTERPOSE_HOOKS_FAILED(layer_musicsound) ||
            INTERPOSE_HOOKS_FAILED(layer_noblelist) ||
            INTERPOSE_HOOKS_FAILED(layer_overall_health) ||
            INTERPOSE_HOOKS_FAILED(layer_reaction) ||
            INTERPOSE_HOOKS_FAILED(layer_squad_schedule) ||
            INTERPOSE_HOOKS_FAILED(layer_stockpile) ||
            INTERPOSE_HOOKS_FAILED(layer_stone_restriction) ||
            INTERPOSE_HOOKS_FAILED(layer_unit_action) ||
            INTERPOSE_HOOKS_FAILED(layer_unit_health) ||
            INTERPOSE_HOOKS_FAILED(layer_unit_relationship) ||
            INTERPOSE_HOOKS_FAILED(layer_world_gen_param_preset) ||
            INTERPOSE_HOOKS_FAILED(layer_world_gen_param) ||
            INTERPOSE_HOOKS_FAILED(legends) ||
            INTERPOSE_HOOKS_FAILED(loadgame) ||
            INTERPOSE_HOOKS_FAILED(locations) ||
            INTERPOSE_HOOKS_FAILED(meeting) ||
            INTERPOSE_HOOKS_FAILED(movieplayer) ||
            INTERPOSE_HOOKS_FAILED(new_region) ||
            INTERPOSE_HOOKS_FAILED(noble) ||
            INTERPOSE_HOOKS_FAILED(option) ||
            INTERPOSE_HOOKS_FAILED(overallstatus) ||
            INTERPOSE_HOOKS_FAILED(petitions) ||
            INTERPOSE_HOOKS_FAILED(pet) ||
            INTERPOSE_HOOKS_FAILED(price) ||
            INTERPOSE_HOOKS_FAILED(reportlist) ||
            INTERPOSE_HOOKS_FAILED(requestagreement) ||
            INTERPOSE_HOOKS_FAILED(savegame) ||
            INTERPOSE_HOOKS_FAILED(selectitem) ||
            INTERPOSE_HOOKS_FAILED(setupadventure) ||
            INTERPOSE_HOOKS_FAILED(setupdwarfgame) ||
            INTERPOSE_HOOKS_FAILED(stores) ||
            INTERPOSE_HOOKS_FAILED(textviewer) ||
            INTERPOSE_HOOKS_FAILED(title) ||
            INTERPOSE_HOOKS_FAILED(topicmeeting_fill_land_holder_positions) ||
            INTERPOSE_HOOKS_FAILED(topicmeeting) ||
            INTERPOSE_HOOKS_FAILED(topicmeeting_takerequests) ||
            INTERPOSE_HOOKS_FAILED(tradeagreement) ||
            INTERPOSE_HOOKS_FAILED(tradegoods) ||
            INTERPOSE_HOOKS_FAILED(tradelist) ||
            INTERPOSE_HOOKS_FAILED(treasurelist) ||
            INTERPOSE_HOOKS_FAILED(unitlist) ||
            INTERPOSE_HOOKS_FAILED(unit) ||
            INTERPOSE_HOOKS_FAILED(update_region) ||
            INTERPOSE_HOOKS_FAILED(wages) ||
            INTERPOSE_HOOKS_FAILED(workquota_condition) ||
            INTERPOSE_HOOKS_FAILED(workquota_details) ||
            INTERPOSE_HOOKS_FAILED(workshop_profile))
        return CR_FAILURE;

    is_enabled = enable;
    return CR_OK;
}

#undef INTERPOSE_HOOKS_FAILED

static command_result overlay_cmd(color_ostream &out, std::vector <std::string> & parameters) {
    if (DBG_NAME(control).isEnabled(DebugCategory::LDEBUG)) {
        DEBUG(control).print("interpreting command with %zu parameters:\n",
                             parameters.size());
        for (auto &param : parameters) {
            DEBUG(control).print("  %s\n", param.c_str());
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(
        PluginCommand(
            "overlay",
            "Manage onscreen widgets.",
            overlay_cmd));

    return plugin_enable(out, true);
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}

DFhackCExport command_result plugin_onupdate (color_ostream &out) {
    return CR_OK;
}
