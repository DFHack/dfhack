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
#include "MiscUtils.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

using namespace DFHack;

DFHACK_PLUGIN("overlay");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

namespace DFHack {
    DBG_DECLARE(overlay, control, DebugCategory::LINFO);
    DBG_DECLARE(overlay, event, DebugCategory::LINFO);
}

#define SCREEN_LIST adopt_region, adventure_log, announcelist, \
    assign_display_item, barter, buildinglist, building, choose_start_site, \
    civlist, counterintelligence, createquota, customize_unit, dungeonmode, \
    dungeon_monsterstatus, dungeon_wrestle, dwarfmode, entity, \
    export_graphical_map, export_region, game_cleaner, image_creator, item, \
    joblist, jobmanagement, job, justice, kitchenpref, layer_arena_creature, \
    layer_assigntrade, layer_choose_language_name, layer_currency, \
    layer_export_play_map, layer_military, layer_musicsound, layer_noblelist, \
    layer_overall_health, layer_reaction, layer_squad_schedule, \
    layer_stockpile, layer_stone_restriction, layer_unit_action, \
    layer_unit_health, layer_unit_relationship, layer_world_gen_param_preset, \
    layer_world_gen_param, legends, loadgame, locations, meeting, movieplayer, \
    new_region, noble, option, overallstatus, petitions, pet, price, \
    reportlist, requestagreement, savegame, selectitem, setupadventure, \
    setupdwarfgame, stores, textviewer, title, \
    topicmeeting_fill_land_holder_positions, topicmeeting, \
    topicmeeting_takerequests, tradeagreement, tradegoods, tradelist, \
    treasurelist, unitlist, unit, update_region, wages, workquota_condition, \
    workquota_details, workshop_profile

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

MAP(IMPLEMENT_HOOKS, SCREEN_LIST)

#undef IMPLEMENT_HOOKS

#define INTERPOSE_HOOKS_FAILED(screen) \
    !INTERPOSE_HOOK(screen##_overlay, logic).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, feed).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, render).apply(enable)

DFhackCExport command_result plugin_enable(color_ostream &, bool enable) {
    if (is_enabled == enable)
        return CR_OK;

    DEBUG(control).print("%sing interpose hooks\n", enable ? "enabl" : "disabl");

    if (MAP_JOIN(INTERPOSE_HOOKS_FAILED, ||, SCREEN_LIST))
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
