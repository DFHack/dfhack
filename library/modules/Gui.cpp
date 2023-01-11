/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "modules/Gui.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"
#include "MiscUtils.h"
using namespace DFHack;

#include "modules/Job.h"
#include "modules/Screen.h"
#include "modules/Maps.h"
#include "modules/Units.h"

#include "DataDefs.h"

#include "df/announcement_flags.h"
#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/building_furnacest.h"
#include "df/building_trapst.h"
#include "df/building_type.h"
#include "df/building_workshopst.h"
#include "df/d_init.h"
#include "df/game_mode.h"
#include "df/general_ref.h"
#include "df/global_objects.h"
#include "df/graphic.h"
#include "df/graphic_viewportst.h"
#include "df/historical_figure.h"
#include "df/interfacest.h"
#include "df/item_corpsepiecest.h"
#include "df/item_corpsest.h"
#include "df/job.h"
#include "df/occupation.h"
#include "df/plant.h"
#include "df/popup_message.h"
#include "df/report.h"
#include "df/report_zoom_type.h"
#include "df/route_stockpile_link.h"
#include "df/stop_depart_condition.h"
#include "df/adventurest.h"
#include "df/buildreq.h"
#include "df/ui_look_list.h"
#include "df/gamest.h"
#include "df/ui_unit_view_mode.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

const size_t MAX_REPORTS_SIZE = 3000; // DF clears old reports to maintain this vector size
const int32_t RECENT_REPORT_TICKS = 500; // used by UNIT_COMBAT_REPORT_ALL_ACTIVE
const int32_t ANNOUNCE_LINE_DURATION = 100; // time to display each line in announcement bar; 3.3 sec at 30 GFPS
const int16_t ANNOUNCE_DISPLAY_TIME = 2000; // DF uses this value for most announcements; 66.6 sec at 30 GFPS

namespace DFHack
{
    DBG_DECLARE(core, gui, DebugCategory::LINFO);
}

using namespace df::enums;

using df::global::gamemode;
using df::global::gps;
using df::global::gview;
using df::global::init;
using df::global::selection_rect;
using df::global::plotinfo;
using df::global::ui_menu_width;
using df::global::game;
using df::global::world;

/* TODO: understand how this changes for v50
static df::layer_object_listst *getLayerList(df::viewscreen_layer *layer, int idx)
{
    return virtual_cast<df::layer_object_listst>(vector_get(layer->layer_objects,idx));
}
*/

static std::string getNameChunk(virtual_identity *id, int start, int end)
{
    if (!id)
        return "UNKNOWN";
    const char *name = id->getName();
    int len = strlen(name);
    if (len > start + end)
        return std::string(name+start, len-start-end);
    else
        return name;
}

/*
 * Classifying focus context by means of a string path.
 */

typedef void (*getFocusStringHandler)(std::string &str, df::viewscreen *screen);
static std::map<virtual_identity*, getFocusStringHandler> getFocusStringHandlers;

#define VIEWSCREEN(name) df::viewscreen_##name##st
#define DEFINE_GET_FOCUS_STRING_HANDLER(screen_type) \
    static void getFocusString_##screen_type(std::string &focus, VIEWSCREEN(screen_type) *screen);\
    DFHACK_STATIC_ADD_TO_MAP(\
        &getFocusStringHandlers, &VIEWSCREEN(screen_type)::_identity, \
        (getFocusStringHandler)getFocusString_##screen_type \
    ); \
    static void getFocusString_##screen_type(std::string &focus, VIEWSCREEN(screen_type) *screen)

DEFINE_GET_FOCUS_STRING_HANDLER(dwarfmode)
{
/* TODO: understand how this changes for v50
    using namespace df::enums::ui_sidebar_mode;

    using df::global::ui_workshop_in_add;
    using df::global::ui_build_selector;
    using df::global::ui_selected_unit;
    using df::global::ui_look_list;
    using df::global::ui_look_cursor;
    using df::global::ui_building_item_cursor;
    using df::global::ui_building_assign_type;
    using df::global::ui_building_assign_is_marked;
    using df::global::ui_building_assign_units;
    using df::global::ui_building_assign_items;
    using df::global::ui_building_in_assign;

    focus += "/" + enum_item_key(plotinfo->main.mode);

    switch (plotinfo->main.mode)
    {
    case QueryBuilding:
        if (df::building *selected = world->selected_building)
        {
            if (!selected->jobs.empty() &&
                selected->jobs[0]->job_type == job_type::DestroyBuilding)
            {
                focus += "/Destroying";
                break;
            }

            focus += "/Some";

            virtual_identity *id = virtual_identity::get(selected);

            bool jobs = false;

            if (id == &df::building_workshopst::_identity ||
                id == &df::building_furnacest::_identity)
            {
                focus += "/Workshop";
                jobs = true;
            }
            else if (id == &df::building_trapst::_identity)
            {
                auto trap = (df::building_trapst*)selected;
                focus += "/" + enum_item_key(trap->trap_type);
                if (trap->trap_type == trap_type::Lever)
                    jobs = true;
            }
            else if (ui_building_in_assign && *ui_building_in_assign &&
                     ui_building_assign_type && ui_building_assign_units &&
                     ui_building_assign_type->size() == ui_building_assign_units->size())
            {
                focus += "/Assign";
                if (ui_building_item_cursor)
                {
                    auto unit = vector_get(*ui_building_assign_units, *ui_building_item_cursor);
                    focus += unit ? "/Unit" : "/None";
                }
            }
            else
                focus += "/" + enum_item_key(selected->getType());

            if (jobs)
            {
                if (ui_workshop_in_add && *ui_workshop_in_add)
                    focus += "/AddJob";
                else if (!selected->jobs.empty())
                    focus += "/Job";
                else
                    focus += "/Empty";
            }
        }
        else
            focus += "/None";
        break;

    case Build:
        if (ui_build_selector)
        {
            // Not selecting, or no choices?
            if (ui_build_selector->building_type < 0)
                focus += "/Type";
            else if (ui_build_selector->stage != 2)
            {
                if (ui_build_selector->stage != 1)
                    focus += "/NoMaterials";
                else
                    focus += "/Position";

                focus += "/" + enum_item_key(ui_build_selector->building_type);
            }
            else
            {
                focus += "/Material";
                if (ui_build_selector->is_grouped)
                    focus += "/Groups";
                else
                    focus += "/Items";
            }
        }
        break;

    case ViewUnits:
        if (ui_selected_unit)
        {
            if (vector_get(world->units.active, *ui_selected_unit))
            {
                focus += "/Some";

                using df::global::ui_unit_view_mode;

                if (ui_unit_view_mode)
                    focus += "/" + enum_item_key(ui_unit_view_mode->value);
            }
            else
                focus += "/None";
        }
        break;

    case LookAround:
        if (ui_look_list && ui_look_cursor)
        {
            auto item = vector_get(ui_look_list->items, *ui_look_cursor);
            if (item)
                focus += "/" + enum_item_key(item->type);
            else
                focus += "/None";
        }
        break;

    case BuildingItems:
        if (VIRTUAL_CAST_VAR(selected, df::building_actual, world->selected_building))
        {
            if (selected->contained_items.empty())
                focus += "/Some/Empty";
            else
                focus += "/Some/Item";
        }
        else
            focus += "/None";
        break;

    case ZonesPenInfo:
        if (ui_building_assign_type && ui_building_assign_units &&
            ui_building_assign_is_marked && ui_building_assign_items &&
            ui_building_assign_type->size() == ui_building_assign_units->size())
        {
            focus += "/Assign";
            if (ui_building_item_cursor)
            {
                if (vector_get(*ui_building_assign_units, *ui_building_item_cursor))
                    focus += "/Unit";
                else if (vector_get(*ui_building_assign_items, *ui_building_item_cursor))
                    focus += "/Vermin";
                else
                    focus += "/None";
            }
        }
        break;

    case Burrows:
        if (plotinfo->burrows.in_confirm_delete)
            focus += "/ConfirmDelete";
        else if (plotinfo->burrows.in_add_units_mode)
            focus += "/AddUnits";
        else if (plotinfo->burrows.in_edit_name_mode)
            focus += "/EditName";
        else if (plotinfo->burrows.in_define_mode)
            focus += "/Define";
        else
            focus += "/List";
        break;

    case Hauling:
        if (plotinfo->hauling.in_assign_vehicle)
        {
            auto vehicle = vector_get(plotinfo->hauling.vehicles, plotinfo->hauling.cursor_vehicle);
            focus += "/AssignVehicle/" + std::string(vehicle ? "Some" : "None");
        }
        else
        {
            int idx = plotinfo->hauling.cursor_top;
            auto route = vector_get(plotinfo->hauling.view_routes, idx);
            auto stop = vector_get(plotinfo->hauling.view_stops, idx);
            std::string tag = stop ? "Stop" : (route ? "Route" : "None");

            if (plotinfo->hauling.in_name)
                focus += "/Rename/" + tag;
            else if (plotinfo->hauling.in_stop)
            {
                int sidx = plotinfo->hauling.cursor_stop;
                auto cond = vector_get(plotinfo->hauling.stop_conditions, sidx);
                auto link = vector_get(plotinfo->hauling.stop_links, sidx);

                focus += "/DefineStop";

                if (cond)
                    focus += "/Cond/" + enum_item_key(cond->mode);
                else if (link)
                {
                    focus += "/Link/";
                    if (link->mode.bits.give) focus += "Give";
                    if (link->mode.bits.take) focus += "Take";
                }
                else
                    focus += "/None";
            }
            else
                focus += "/Select/" + tag;
        }
        break;

    default:
        break;
    }
*/
}

/* TODO: understand how this changes for v50
DEFINE_GET_FOCUS_STRING_HANDLER(dungeonmode)
{
    using df::global::adventure;

    if (!adventure)
        return;

    focus += "/" + enum_item_key(adventure->menu);
}

DEFINE_GET_FOCUS_STRING_HANDLER(unitlist)
{
    focus += "/" + enum_item_key(screen->page);
}

DEFINE_GET_FOCUS_STRING_HANDLER(layer_military)
{
    auto list1 = getLayerList(screen, 0);
    auto list2 = getLayerList(screen, 1);
    auto list3 = getLayerList(screen, 2);
    if (!list1 || !list2 || !list3) return;

    focus += "/" + enum_item_key(screen->page);

    int cur_list;
    if (list1->active) cur_list = 0;
    else if (list2->active) cur_list = 1;
    else if (list3->active) cur_list = 2;
    else return;

    switch (screen->page)
    {
    case df::viewscreen_layer_militaryst::Positions:
        {
            static const char *lists[] = { "/Squads", "/Positions", "/Candidates" };
            focus += lists[cur_list];
            break;
        }

    case df::viewscreen_layer_militaryst::Equip:
        {
            focus += "/" + enum_item_key(screen->equip.mode);

            switch (screen->equip.mode)
            {
            case df::viewscreen_layer_militaryst::T_equip::Customize:
                {
                    if (screen->equip.edit_mode < 0)
                        focus += "/View";
                    else
                        focus += "/" + enum_item_key(screen->equip.edit_mode);
                    break;
                }
            case df::viewscreen_layer_militaryst::T_equip::Uniform:
                break;
            case df::viewscreen_layer_militaryst::T_equip::Priority:
                {
                    if (screen->equip.prio_in_move >= 0)
                        focus += "/Move";
                    else
                        focus += "/View";
                    break;
                }
            }

            static const char *lists[] = { "/Squads", "/Positions", "/Choices" };
            focus += lists[cur_list];
            break;
        }

    default:
        break;
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(workshop_profile)
{
    typedef df::viewscreen_workshop_profilest::T_tab T_tab;
    switch(screen->tab)
    {
    case T_tab::Workers:
        focus += "/Unit";
        break;
    case T_tab::Orders:
        focus += "/Orders";
        break;
    case T_tab::Restrictions:
        focus += "/Restrictions";
        break;
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(layer_noblelist)
{
    auto list1 = getLayerList(screen, 0);
    auto list2 = getLayerList(screen, 1);
    if (!list1 || !list2) return;

    focus += "/" + enum_item_key(screen->mode);
}

DEFINE_GET_FOCUS_STRING_HANDLER(pet)
{
    focus += "/" + enum_item_key(screen->mode);

    switch (screen->mode)
    {
    case df::viewscreen_petst::List:
        focus += vector_get(screen->is_vermin, screen->cursor) ? "/Vermin" : "/Unit";
        break;

    case df::viewscreen_petst::SelectTrainer:
        if (vector_get(screen->trainer_unit, screen->trainer_cursor))
            focus += "/Unit";
        break;

    default:
        break;
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(layer_overall_health)
{
    auto list1 = getLayerList(screen, 0);
    if (!list1) return;

    focus += "/Units";
}

DEFINE_GET_FOCUS_STRING_HANDLER(tradegoods)
{
    if (!screen->has_traders || screen->is_unloading)
        focus += "/NoTraders";
    else if (screen->in_edit_count)
        focus += "/EditCount";
    else
        focus += (screen->in_right_pane ? "/Items/Broker" : "/Items/Trader");
}

DEFINE_GET_FOCUS_STRING_HANDLER(layer_assigntrade)
{
    auto list1 = getLayerList(screen, 0);
    auto list2 = getLayerList(screen, 1);
    if (!list1 || !list2) return;

    int list_idx = vector_get(screen->visible_lists, list1->cursor, (int16_t)-1);
    unsigned num_lists = sizeof(screen->lists)/sizeof(screen->lists[0]);
    if (unsigned(list_idx) >= num_lists)
        return;

    if (list1->active)
        focus += "/Groups";
    else
        focus += "/Items";
}

DEFINE_GET_FOCUS_STRING_HANDLER(stores)
{
    if (!screen->in_right_list)
        focus += "/Categories";
    else if (screen->in_group_mode)
        focus += "/Groups";
    else
        focus += "/Items";
}

DEFINE_GET_FOCUS_STRING_HANDLER(layer_stockpile)
{
    auto list1 = getLayerList(screen, 0);
    auto list2 = getLayerList(screen, 1);
    auto list3 = getLayerList(screen, 2);
    if (!list1 || !list2 || !list3 || !screen->settings) return;

    auto group = screen->cur_group;
    if (group != vector_get(screen->group_ids, list1->cursor))
        return;

    focus += "/" + enum_item_key(group);

    auto bits = vector_get(screen->group_bits, list1->cursor);
    if (bits.whole && !(bits.whole & screen->settings->flags.whole))
    {
        focus += "/Off";
        return;
    }

    focus += "/On";

    if (list2->active || list3->active || screen->list_ids.empty()) {
        focus += "/" + enum_item_key(screen->cur_list);

        if (list3->active)
            focus += (screen->item_names.empty() ? "/None" : "/Item");
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(locations)
{
    focus += "/" + enum_item_key(screen->menu);
}

DEFINE_GET_FOCUS_STRING_HANDLER(jobmanagement)
{
    focus += (screen->in_max_workshops ? "/MaxWorkshops" : "/Main");
}

DEFINE_GET_FOCUS_STRING_HANDLER(workquota_condition)
{
    focus += "/" + enum_item_key(screen->mode);
    if (screen->item_count_edit)
        focus += "/EditCount";
}
*/

std::string Gui::getFocusString(df::viewscreen *top)
{
    if (!top)
        return "";

    if (dfhack_viewscreen::is_instance(top))
    {
        auto name = static_cast<dfhack_viewscreen*>(top)->getFocusString();
        return name.empty() ? "dfhack" : "dfhack/"+name;
    }
    else if (virtual_identity *id = virtual_identity::get(top))
    {
        std::string name = getNameChunk(id, 11, 2);

        auto handler = map_find(getFocusStringHandlers, id);
        if (handler)
            handler(name, top);

        return name;
    }
    else
    {
        Core &core = Core::getInstance();
        std::string name = core.p->readClassName(*(void**)top);
        return name.substr(11, name.size()-11-2);
    }
}

// Predefined common guard functions

bool Gui::default_hotkey(df::viewscreen *top)
{
    // Default hotkey guard function
    for (;top ;top = top->parent)
    {
        if (strict_virtual_cast<df::viewscreen_dwarfmodest>(top))
            return true;
/* TODO: understand how this changes for v50
        if (strict_virtual_cast<df::viewscreen_dungeonmodest>(top))
            return true;
*/
    }
    return false;
}

bool Gui::anywhere_hotkey(df::viewscreen *) {
    return true;
}

bool Gui::dwarfmode_hotkey(df::viewscreen *top)
{
    // Require the main dwarf mode screen
    return !!strict_virtual_cast<df::viewscreen_dwarfmodest>(top);
}

bool Gui::unitjobs_hotkey(df::viewscreen *top)
{
/* TODO: understand how this changes for v50
    // Require the unit or jobs list
    return !!strict_virtual_cast<df::viewscreen_joblistst>(top) ||
           !!strict_virtual_cast<df::viewscreen_unitlistst>(top);
*/ return false;
}

bool Gui::item_details_hotkey(df::viewscreen *top)
{
/* TODO: understand how this changes for v50
    // Require the main dwarf mode screen
    return !!strict_virtual_cast<df::viewscreen_itemst>(top);
*/ return false;
}

static bool has_cursor()
{
    return df::global::cursor && df::global::cursor->x != -30000;
}

bool Gui::cursor_hotkey(df::viewscreen *top)
{
    if (!dwarfmode_hotkey(top))
        return false;

    // Also require the cursor.
    if (!has_cursor())
        return false;

    return true;
}

bool Gui::workshop_job_hotkey(df::viewscreen *top)
{
    if (!dwarfmode_hotkey(top))
        return false;

/* TODO: understand how this changes for v50
    using namespace ui_sidebar_mode;
    using df::global::ui_workshop_in_add;
    using df::global::ui_workshop_job_cursor;

    switch (plotinfo->main.mode) {
    case QueryBuilding:
        {
            if (!ui_workshop_job_cursor) // allow missing
                return false;

            df::building *selected = world->selected_building;
            if (!virtual_cast<df::building_workshopst>(selected) &&
                !virtual_cast<df::building_furnacest>(selected))
                return false;

            // No jobs?
            if (selected->jobs.empty() ||
                selected->jobs[0]->job_type == job_type::DestroyBuilding)
                return false;

            // Add job gui activated?
            if (ui_workshop_in_add && *ui_workshop_in_add)
                return false;

            return true;
        };
    default:
        return false;
    }
*/ return false;
}

bool Gui::build_selector_hotkey(df::viewscreen *top)
{
    if (!dwarfmode_hotkey(top))
        return false;

/* TODO: understand how this changes for v50
    using namespace ui_sidebar_mode;
    using df::global::ui_build_selector;

    switch (plotinfo->main.mode) {
    case Build:
        {
            if (!ui_build_selector) // allow missing
                return false;

            // Not selecting, or no choices?
            if (ui_build_selector->building_type < 0 ||
                ui_build_selector->stage != 2 ||
                ui_build_selector->choices.empty())
                return false;

            return true;
        };
    default:
        return false;
    }
*/ return false;
}

bool Gui::view_unit_hotkey(df::viewscreen *top)
{
    if (!dwarfmode_hotkey(top))
        return false;
/* TODO: understand how this changes for v50
    using df::global::ui_selected_unit;

    if (plotinfo->main.mode != ui_sidebar_mode::ViewUnits)
        return false;
    if (!ui_selected_unit) // allow missing
        return false;

    return vector_get(world->units.active, *ui_selected_unit) != NULL;
*/ return false;
}

bool Gui::unit_inventory_hotkey(df::viewscreen *top)
{
    using df::global::ui_unit_view_mode;

    if (!view_unit_hotkey(top))
        return false;
    if (!ui_unit_view_mode)
        return false;

    return ui_unit_view_mode->value == df::ui_unit_view_mode::Inventory;
}

df::job *Gui::getSelectedWorkshopJob(color_ostream &out, bool quiet)
{
    using df::global::ui_workshop_job_cursor;

    if (!workshop_job_hotkey(Core::getTopViewscreen())) {
        if (!quiet)
            out.printerr("Not in a workshop, or no job is highlighted.\n");
        return NULL;
    }

    df::building *selected = world->selected_building;
    int idx = *ui_workshop_job_cursor;

    if (size_t(idx) >= selected->jobs.size())
    {
        out.printerr("Invalid job cursor index: %d\n", idx);
        return NULL;
    }

    return selected->jobs[idx];
}

bool Gui::any_job_hotkey(df::viewscreen *top)
{
/* TODO: understand how this changes for v50
    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_joblistst, top))
        return vector_get(screen->jobs, screen->cursor_pos) != NULL;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitlistst, top))
        return vector_get(screen->jobs[screen->page], screen->cursor_pos[screen->page]) != NULL;

    return workshop_job_hotkey(top);
*/ return false;
}

df::job *Gui::getSelectedJob(color_ostream &out, bool quiet)
{
/* TODO: understand how this changes for v50
    df::viewscreen *top = Core::getTopViewscreen();

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_jobst, top))
    {
        return screen->job;
    }
    if (VIRTUAL_CAST_VAR(joblist, df::viewscreen_joblistst, top))
    {
        df::job *job = vector_get(joblist->jobs, joblist->cursor_pos);

        if (!job && !quiet)
            out.printerr("Selected unit has no job\n");

        return job;
    }
    else if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, top))
    {
        int page = unitlist->page;
        df::job *job = vector_get(unitlist->jobs[page], unitlist->cursor_pos[page]);

        if (!job && !quiet)
            out.printerr("Selected unit has no job\n");

        return job;
    }
    else if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedJob();
    else
        return getSelectedWorkshopJob(out, quiet);
*/ return getSelectedWorkshopJob(out, quiet);
}

df::unit *Gui::getAnyUnit(df::viewscreen *top)
{
    using df::global::game;

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedUnit();

    if (game->main_interface.view_sheets.open
            && game->main_interface.view_sheets.active_sheet == view_sheet_type::UNIT)
        return df::unit::find(game->main_interface.view_sheets.active_id);

/* TODO: understand how this changes for v50
    using namespace ui_sidebar_mode;
    using df::global::ui_look_cursor;
    using df::global::ui_look_list;
    using df::global::ui_selected_unit;
    using df::global::ui_building_in_assign;
    using df::global::ui_building_assign_units;
    using df::global::ui_building_item_cursor;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitst, top))
    {
        return screen->unit;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_joblistst, top))
    {
        if (auto unit = vector_get(screen->units, screen->cursor_pos))
            return unit;
        if (auto job = vector_get(screen->jobs, screen->cursor_pos))
            return Job::getWorker(job);
        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_unitlistst, top))
        return vector_get(screen->units[screen->page], screen->cursor_pos[screen->page]);

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_dungeon_monsterstatusst, top))
        return screen->unit;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_unit_relationshipst, top))
    {
        if (VIRTUAL_CAST_VAR(list, df::layer_object_listst, vector_get(screen->layer_objects, 0)))
            return vector_get(screen->relation_unit, list->cursor);

        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_itemst, top))
    {
        df::general_ref *ref = vector_get(screen->entry_ref, screen->cursor_pos);
        return ref ? ref->getUnit() : NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_workshop_profilest, top))
    {
        if (screen->tab == df::viewscreen_workshop_profilest::Workers)
            return vector_get(screen->workers, screen->worker_idx);
        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_noblelistst, top))
    {
        switch (screen->mode)
        {
        case df::viewscreen_layer_noblelistst::List:
            if (auto list1 = getLayerList(screen, 0))
            {
                if (auto info = vector_get(screen->info, list1->cursor))
                    return info->unit;
            }
            return NULL;

        case df::viewscreen_layer_noblelistst::Appoint:
            if (auto list2 = getLayerList(screen, 1))
            {
                if (auto info = vector_get(screen->candidates, list2->cursor))
                    return info->unit;
            }
            return NULL;

        default:
            return NULL;
        }
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_locationsst, top))
    {
        switch (screen->menu)
        {
        case df::viewscreen_locationsst::AssignOccupation:
            return vector_get(screen->units, screen->unit_idx);
        case df::viewscreen_locationsst::Occupations:
        {
            auto occ = vector_get(screen->occupations, screen->occupation_idx);
            if (occ)
                return df::unit::find(occ->unit_id);
            return NULL;
        }
        default:
            return NULL;
        }
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_petst, top))
    {
        df::viewscreen_petst::T_animal animal_default;
        animal_default.unit = NULL;
        switch (screen->mode)
        {
        case df::viewscreen_petst::List:
            if (!vector_get(screen->is_vermin, screen->cursor))
                return vector_get(screen->animal, screen->cursor, animal_default).unit;
            return NULL;

        case df::viewscreen_petst::SelectTrainer:
            return vector_get(screen->trainer_unit, screen->trainer_cursor);

        default:
            return NULL;
        }
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_overall_healthst, top))
    {
        if (auto list1 = getLayerList(screen, 0))
            return vector_get(screen->unit, list1->cursor);
        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_textviewerst, top))
    {
        if (screen->parent)
            return getAnyUnit(screen->parent);
        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_reportlistst, top))
        return vector_get(screen->units, screen->cursor);

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_announcelistst, top))
    {
        if (world && screen->unit) {
            // in (r)eports -> enter
            auto *report = vector_get(screen->reports, screen->sel_idx);
            if (report)
            {
                for (df::unit *unit : world->units.all)
                {
                    if (unit && screen->report_type >= 0 && screen->report_type < 3
                        && unit != screen->unit) // find 'other' unit related to this report
                    {
                        for (int32_t report_id : unit->reports.log[screen->report_type])
                        {
                            if (report_id == report->id)
                                return unit;
                        }
                    }
                }
            }
        } else {
            // in (a)nnouncements
            return NULL; // cannot determine unit from reports
        }
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_militaryst, top))
    {
        if (screen->page == df::viewscreen_layer_militaryst::T_page::Positions) {
            auto positions = getLayerList(screen, 1);
            if (positions && positions->enabled && positions->active)
                return vector_get(screen->positions.assigned, positions->cursor);

            auto candidates = getLayerList(screen, 2);
            if (candidates && candidates->enabled && candidates->active)
                return vector_get(screen->positions.candidates, candidates->cursor);
        }
        if (screen->page == df::viewscreen_layer_militaryst::T_page::Equip) {
            auto positions = getLayerList(screen, 1);
            if (positions && positions->enabled && positions->active)
                return vector_get(screen->equip.units, positions->cursor);
        }
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_unit_healthst, top))
        return screen->unit;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_customize_unitst, top))
        return screen->unit;

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedUnit();

    if (!Gui::dwarfmode_hotkey(top))
        return NULL;

    if (!plotinfo)
        return NULL;

    // general assigning units in building, i.e. (q)uery cage -> (a)ssign
    if (ui_building_in_assign && *ui_building_in_assign
        && ui_building_assign_units && ui_building_item_cursor
        && plotinfo->main.mode != Zones) // dont show for (i) zone
        return vector_get(*ui_building_assign_units, *ui_building_item_cursor);

    if (plotinfo->follow_unit != -1)
        return df::unit::find(plotinfo->follow_unit);

    switch (plotinfo->main.mode) {
    case ViewUnits:
    {
        if (!ui_selected_unit || !world)
            return NULL;

        return vector_get(world->units.active, *ui_selected_unit);
    }
    case ZonesPitInfo: // (i) zone -> (P)it
    case ZonesPenInfo: // (i) zone -> pe(N)
    {
        if (!ui_building_assign_units || !ui_building_item_cursor)
            return NULL;

        return vector_get(*ui_building_assign_units, *ui_building_item_cursor);
    }
    case Burrows:
    {
        if (plotinfo->burrows.in_add_units_mode)
            return vector_get(plotinfo->burrows.list_units, plotinfo->burrows.unit_cursor_pos);

        return NULL;
    }
    case QueryBuilding:
    {
        if (df::building *building = getAnyBuilding(top))
        {
            if (VIRTUAL_CAST_VAR(cage, df::building_cagest, building))
            {
                if (ui_building_item_cursor)
                    return df::unit::find(vector_get(cage->assigned_units, *ui_building_item_cursor));
            }
        }
        return NULL;
    }
    case LookAround:
    {
        if (!ui_look_list || !ui_look_cursor)
            return NULL;

        if (auto item = vector_get(ui_look_list->items, *ui_look_cursor))
        {
            if (item->type == df::ui_look_list::T_items::Unit)
                return item->data.Unit;
            else if (item->type == df::ui_look_list::T_items::Item)
            {
                if (VIRTUAL_CAST_VAR(corpse, df::item_corpsest, item->data.Item))
                    return df::unit::find(corpse->unit_id); // loo(k) at corpse
                else if (VIRTUAL_CAST_VAR(corpsepiece, df::item_corpsepiecest, item->data.Item))
                    return df::unit::find(corpsepiece->unit_id); // loo(k) at corpse piece
            }
            else if (item->type == df::ui_look_list::T_items::Spatter)
            {
                // loo(k) at blood/ichor/.. spatter with a name
                MaterialInfo mat;
                if (mat.decode(item->spatter_mat_type, item->spatter_mat_index) && mat.figure)
                    return df::unit::find(mat.figure->unit_id);
            }
        }

        return NULL;
    }
    default:
        return NULL;
    }
*/ return NULL;
}

bool Gui::any_unit_hotkey(df::viewscreen *top)
{
    return getAnyUnit(top) != NULL;
}

df::unit *Gui::getSelectedUnit(color_ostream &out, bool quiet)
{
    df::unit *unit = getAnyUnit(Core::getTopViewscreen());

    if (!unit && !quiet)
        out.printerr("No unit is selected in the UI.\n");

    return unit;
}

df::item *Gui::getAnyItem(df::viewscreen *top)
{
    using df::global::game;

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedItem();

    if (game->main_interface.view_sheets.open
            && game->main_interface.view_sheets.active_sheet == view_sheet_type::ITEM)
        return df::item::find(game->main_interface.view_sheets.active_id);

/* TODO: understand how this changes for v50
    using namespace ui_sidebar_mode;
    using df::global::ui_look_cursor;
    using df::global::ui_look_list;
    using df::global::ui_unit_view_mode;
    using df::global::ui_building_item_cursor;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_textviewerst, top))
    {
        // return the main item if the parent screen is a viewscreen_itemst
        if (VIRTUAL_CAST_VAR(parent_screen, df::viewscreen_itemst, screen->parent))
            return parent_screen->item;

        if (screen->parent)
            return getAnyItem(screen->parent);

        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_itemst, top))
    {
        df::general_ref *ref = vector_get(screen->entry_ref, screen->cursor_pos);
        return ref ? ref->getItem() : NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_layer_assigntradest, top))
    {
        auto list1 = getLayerList(screen, 0);
        auto list2 = getLayerList(screen, 1);
        if (!list1 || !list2 || !list2->active)
            return NULL;

        int list_idx = vector_get(screen->visible_lists, list1->cursor, (int16_t)-1);
        unsigned num_lists = sizeof(screen->lists)/sizeof(std::vector<int32_t>);
        if (unsigned(list_idx) >= num_lists)
            return NULL;

        int idx = vector_get(screen->lists[list_idx], list2->cursor, -1);
        if (auto info = vector_get(screen->info, idx))
            return info->item;

        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_tradegoodsst, top))
    {
        if (screen->in_right_pane)
            return vector_get(screen->broker_items, screen->broker_cursor);
        else
            return vector_get(screen->trader_items, screen->trader_cursor);
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_storesst, top))
    {
        if (screen->in_right_list && !screen->in_group_mode)
            return vector_get(screen->items, screen->item_cursor);

        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_assign_display_itemst, top))
    {
        if (screen->sel_column == df::viewscreen_assign_display_itemst::T_sel_column::Items)
            return vector_get(screen->items[screen->item_type[screen->sel_type]],
                screen->sel_item);

        return NULL;
    }

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_treasurelistst, top))
    {
        if (world)
            return vector_get(world->items.other[df::items_other_id::ANY_ARTIFACT], screen->sel_idx);
        return NULL;
    }

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedItem();

    if (!Gui::dwarfmode_hotkey(top))
        return NULL;

    switch (plotinfo->main.mode) {
    case ViewUnits:
    {
        if (!ui_unit_view_mode || !ui_look_cursor || !game)
            return NULL;

        if (ui_unit_view_mode->value != df::ui_unit_view_mode::Inventory)
            return NULL;

        auto inv_item = vector_get(game->unit.inv_items, *ui_look_cursor);
        return inv_item ? inv_item->item : NULL;
    }
    case LookAround:
    {
        if (!ui_look_list || !ui_look_cursor)
            return NULL;

        auto item = vector_get(ui_look_list->items, *ui_look_cursor);
        if (item && item->type == df::ui_look_list::T_items::Item)
            return item->data.Item;
        else
            return NULL;
    }
    case BuildingItems:
    {
        if (!ui_building_item_cursor)
            return NULL;

        VIRTUAL_CAST_VAR(selected, df::building_actual, world->selected_building);
        if (!selected)
            return NULL;

        auto inv_item = vector_get(selected->contained_items, *ui_building_item_cursor);
        return inv_item ? inv_item->item : NULL;
    }
    default:
        return NULL;
    }
*/ return NULL;
}

bool Gui::any_item_hotkey(df::viewscreen *top)
{
    return getAnyItem(top) != NULL;
}

df::item *Gui::getSelectedItem(color_ostream &out, bool quiet)
{
    df::item *item = getAnyItem(Core::getTopViewscreen());

    if (!item && !quiet)
        out.printerr("No item is selected in the UI.\n");

    return item;
}

df::building *Gui::getAnyBuilding(df::viewscreen *top)
{
    using df::global::game;

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedBuilding();

    if (game->main_interface.view_sheets.open
            && game->main_interface.view_sheets.active_sheet == view_sheet_type::BUILDING)
        return df::building::find(game->main_interface.view_sheets.active_id);

/* TODO: understand how this changes for v50
    using namespace ui_sidebar_mode;
    using df::global::ui_look_list;
    using df::global::ui_look_cursor;

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_buildinglistst, top))
        return vector_get(screen->buildings, screen->cursor);

    if (VIRTUAL_CAST_VAR(screen, df::viewscreen_workshop_profilest, top))
        return df::building::find(screen->building_id);

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedBuilding();

    if (!Gui::dwarfmode_hotkey(top))
        return NULL;

    switch (plotinfo->main.mode) {
    case LookAround:
    {
        if (!ui_look_list || !ui_look_cursor)
            return NULL;

        auto item = vector_get(ui_look_list->items, *ui_look_cursor);
        if (item && item->type == df::ui_look_list::T_items::Building)
            return item->data.Building;
        else
            return NULL;
    }
    case QueryBuilding:
    case BuildingItems:
    {
        return world->selected_building;
    }
    case Zones:
    case ZonesPenInfo:
    case ZonesPitInfo:
    case ZonesHospitalInfo:
    {
        if (game)
            return game->zone.selected;
        return NULL;
    }
    default:
        return NULL;
    }
*/ return NULL;
}

bool Gui::any_building_hotkey(df::viewscreen *top)
{
    return getAnyBuilding(top) != NULL;
}

df::building *Gui::getSelectedBuilding(color_ostream &out, bool quiet)
{
    df::building *building = getAnyBuilding(Core::getTopViewscreen());

    if (!building && !quiet)
        out.printerr("No building is selected in the UI.\n");

    return building;
}

df::plant *Gui::getAnyPlant(df::viewscreen *top)
{
    using df::global::cursor;

    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedPlant();

    if (Gui::dwarfmode_hotkey(top))
    {
        if (!cursor || !plotinfo || !world)
            return nullptr;

/* TODO: understand how this changes for v50
        if (plotinfo->main.mode == ui_sidebar_mode::LookAround)
        {
            return Maps::getPlantAtTile(cursor->x, cursor->y, cursor->z);
        }
*/
    }

    return nullptr;
}

bool Gui::any_plant_hotkey(df::viewscreen *top)
{
    return getAnyPlant(top) != nullptr;
}

df::plant *Gui::getSelectedPlant(color_ostream &out, bool quiet)
{
    df::plant *plant = getAnyPlant(Core::getTopViewscreen());

    if (!plant && !quiet)
        out.printerr("No plant is selected in the UI.\n");

    return plant;
}

//

DFHACK_EXPORT void Gui::writeToGamelog(std::string message)
{
    if (message.empty())
        return;

    std::ofstream fseed("gamelog.txt", std::ios::out | std::ios::app);
    if(fseed.is_open())
        fseed << message << std::endl;
    fseed.close();
}

// Utility functions for reports
static bool parseReportString(std::vector<std::string> *out, const std::string &str, size_t line_length = 73)
{   // parse a string into output strings like DF does for reports
    if (str.empty() || line_length == 0)
        return false;

    string parsed;
    size_t i = 0;

    do
    {
        if (str[i] == '&') // escape character
        {
            i++; // ignore the '&' itself
            if (i >= str.length())
                break;

            if (str[i] == 'r') // "&r" adds a blank line
            {
                word_wrap(out, parsed, line_length, WSMODE_TRIM_LEADING);
                out->push_back(" "); // DF adds a line with a space for some reason
                parsed.clear();
            }
            else if (str[i] == '&') // "&&" is '&'
                parsed.push_back('&');
            // else next char is ignored
        }
        else
            parsed.push_back(str[i]);
    }
    while (++i < str.length());

    if (parsed.length())
        word_wrap(out, parsed, line_length, WSMODE_TRIM_LEADING);

    return true;
}

static bool recent_report(df::unit *unit, df::unit_report_type slot)
{
    return unit && !unit->reports.log[slot].empty() &&
        *df::global::cur_year == unit->reports.last_year[slot] &&
        (*df::global::cur_year_tick - unit->reports.last_year_tick[slot]) <= RECENT_REPORT_TICKS;
}

static bool recent_report_any(df::unit *unit)
{
    FOR_ENUM_ITEMS(unit_report_type, slot)
    {
        if (recent_report(unit, slot))
            return true;
    }
    return false;
}

static void delete_old_reports()
{
    auto &reports = world->status.reports;
    if (reports.size() > MAX_REPORTS_SIZE)
    {
        size_t excess = reports.size() - MAX_REPORTS_SIZE;
        for (size_t i = 0; i < excess; i++)
        {
            if (reports[i] != NULL)
            {   // report destructor
                if (reports[i]->flags.bits.announcement)
                    erase_from_vector(world->status.announcements, &df::report::id, reports[i]->id);
                delete reports[i];
            }
        }
        reports.erase(reports.begin(), reports.begin() + excess);
    }
}

static int32_t check_repeat_report(vector<string> &results)
{   // returns the new repeat count, else 0
    if (*gamemode == game_mode::DWARF && !results.empty() && world->status.reports.size() >= results.size())
    {
        auto &reports = world->status.reports;
        size_t base = reports.size() - results.size(); // index where a repeat would start
        size_t offset = 0;
        while (reports[base + offset]->text == results[offset] && ++offset < results.size()); // match each report

        if (offset == results.size()) // all lines matched
        {
            reports[base]->duration = ANNOUNCE_LINE_DURATION; // display the last line again
            return ++(reports[base]->repeat_count);
        }
    }
    return 0;
}

static bool add_proper_report(df::unit *unit, bool is_sparring, int report_index)
{   // add report to proper category based on is_sparring and unit current job
    if (is_sparring)
        return Gui::addCombatReport(unit, unit_report_type::Sparring, report_index);
    else if (unit->job.current_job != NULL && unit->job.current_job->job_type == job_type::Hunt)
        return Gui::addCombatReport(unit, unit_report_type::Hunting, report_index);
    else
        return Gui::addCombatReport(unit, unit_report_type::Combat, report_index);
}
// End of utility functions for reports

DFHACK_EXPORT int Gui::makeAnnouncement(df::announcement_type type, df::announcement_flags flags, df::coord pos, std::string message, int color, bool bright)
{
    using df::global::cur_year;
    using df::global::cur_year_tick;

    if (gamemode == NULL || cur_year == NULL || cur_year_tick == NULL)
    {
        return -1;
    }
    else if (message.empty())
    {
        Core::printerr("Empty announcement %u\n", type); // DF would print this to errorlog.txt
        return -1;
    }

    // Apply the requested effects

    if (flags.bits.PAUSE || flags.bits.RECENTER)
        pauseRecenter((flags.bits.RECENTER ? pos : df::coord()), flags.bits.PAUSE);

    bool adv_unconscious = (*gamemode == game_mode::ADVENTURE && !world->units.active.empty() && world->units.active[0]->counters.unconscious > 0);

    if (flags.bits.DO_MEGA && !adv_unconscious)
        showPopupAnnouncement(message, color, bright);

    vector<string> results;
    word_wrap(&results, message, init->display.grid_x - 7);

    // Check for repeat report
    int32_t repeat_count = check_repeat_report(results); // Does nothing outside dwarf mode
    if (repeat_count > 0)
    {
        if (flags.bits.D_DISPLAY)
        {
            world->status.display_timer = ANNOUNCE_DISPLAY_TIME;
            Gui::writeToGamelog("x" + to_string(repeat_count + 1));
        }
        return -1;
    }

    // Not a repeat, write the message to gamelog.txt
    writeToGamelog(message);

    // Generate the report objects
    int report_idx = world->status.reports.size();
    bool continued = false;
    bool display = ((*gamemode == game_mode::ADVENTURE && flags.bits.A_DISPLAY) || (*gamemode == game_mode::DWARF && flags.bits.D_DISPLAY));

    for (size_t i = 0; i < results.size(); i++)
    {
        auto new_rep = new df::report();
        new_rep->type = type;
        new_rep->pos = pos;

        new_rep->color = color;
        new_rep->bright = bright;
        new_rep->year = *cur_year;
        new_rep->time = *cur_year_tick;

        new_rep->flags.bits.continuation = continued;
        continued = true;

        new_rep->text = results[i];
        new_rep->id = world->status.next_report_id++;
        world->status.reports.push_back(new_rep);

        if (adv_unconscious)
            new_rep->flags.bits.unconscious = true;

        if (display)
        {
            insert_into_vector(world->status.announcements, &df::report::id, new_rep);
            new_rep->flags.bits.announcement = true;
            world->status.display_timer = ANNOUNCE_DISPLAY_TIME;
        }
    }

    delete_old_reports();
    return report_idx;
}

bool Gui::addCombatReport(df::unit *unit, df::unit_report_type slot, int report_index)
{
    CHECK_INVALID_ARGUMENT(is_valid_enum_item(slot));

    auto &vec = world->status.reports;
    auto report = vector_get(vec, report_index);

    if (!unit || !report)
        return false;

    // Check that it is a new report
    auto &rvec = unit->reports.log[slot];
    if (!rvec.empty() && rvec.back() >= report->id)
        return false;

    // Add the report
    rvec.push_back(report->id);

    unit->reports.last_year[slot] = report->year;
    unit->reports.last_year_tick[slot] = report->time;

    switch (slot) {
        case unit_report_type::Combat:
            world->status.flags.bits.combat = true;
            break;
        case unit_report_type::Hunting:
            world->status.flags.bits.hunting = true;
            break;
        case unit_report_type::Sparring:
            world->status.flags.bits.sparring = true;
            break;
    }

    // And all the continuation lines
    for (size_t i = report_index+1; i < vec.size() && vec[i]->flags.bits.continuation; i++)
        rvec.push_back(vec[i]->id);

    return true;
}

bool Gui::addCombatReportAuto(df::unit *unit, df::announcement_flags mode, int report_index)
{
    auto &vec = world->status.reports;
    auto report = vector_get(vec, report_index);

    if (!unit || !report)
        return false;

    bool ok = false;

    if (mode.bits.UNIT_COMBAT_REPORT)
        ok |= add_proper_report(unit, unit->flags2.bits.sparring, report_index);

    if (mode.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
    {
        FOR_ENUM_ITEMS(unit_report_type, slot)
        {
            if (recent_report(unit, slot))
                ok |= addCombatReport(unit, slot, report_index);
        }
    }

    return ok;
}

void Gui::showAnnouncement(std::string message, int color, bool bright)
{
    df::announcement_flags mode;
    mode.bits.D_DISPLAY = mode.bits.A_DISPLAY = true;

    makeAnnouncement(df::announcement_type(), mode, df::coord(), message, color, bright);
}

void Gui::showZoomAnnouncement(
    df::announcement_type type, df::coord pos, std::string message, int color, bool bright
) {
    df::announcement_flags mode;
    mode.bits.D_DISPLAY = mode.bits.A_DISPLAY = true;

    makeAnnouncement(type, mode, pos, message, color, bright);
}

void Gui::showPopupAnnouncement(std::string message, int color, bool bright)
{
    df::popup_message *popup = new df::popup_message();
    popup->text = message;
    popup->color = color;
    popup->bright = bright;

    auto &popups = world->status.popups;
    popups.push_back(popup);

    while (popups.size() > MAX_REPORTS_SIZE)
    {   // Delete old popups
        if (popups[0] != NULL)
            delete popups[0];
        popups.erase(popups.begin());
    }
}

void Gui::showAutoAnnouncement(
    df::announcement_type type, df::coord pos, std::string message, int color, bool bright,
    df::unit *unit1, df::unit *unit2
) {
    using df::global::d_init;

    df::announcement_flags flags;
    flags.bits.D_DISPLAY = flags.bits.A_DISPLAY = true;

    if (is_valid_enum_item(type) && d_init)
        flags = d_init->announcements.flags[type];

    int id = makeAnnouncement(type, flags, pos, message, color, bright);

    addCombatReportAuto(unit1, flags, id);
    addCombatReportAuto(unit2, flags, id);
}

bool Gui::autoDFAnnouncement(df::report_init r, string message)
{   // Reverse-engineered from DF announcement code
    if (!world->allow_announcements)
    {
        DEBUG(gui).print("Skipped announcement because world->allow_announcements is false:\n%s\n", message.c_str());
        return false;
    }
    else if (!is_valid_enum_item(r.type))
    {
        WARN(gui).print("Invalid announcement type:\n%s\n", message.c_str());
        return false;
    }
    else if (message.empty())
    {
        Core::printerr("Empty announcement %u\n", r.type); // DF would print this to errorlog.txt
        return false;
    }

    df::announcement_flags a_flags = df::global::d_init->announcements.flags[r.type];

    // Check if the announcement will actually be announced
    if (*gamemode == game_mode::ADVENTURE)
    {
        if (r.pos.x != -30000 &&
            r.type != announcement_type::CREATURE_SOUND &&
            r.type != announcement_type::REGULAR_CONVERSATION &&
            r.type != announcement_type::CONFLICT_CONVERSATION &&
            r.type != announcement_type::MECHANISM_SOUND)
        {   // If not sound, make sure we can see pos
            if (world->units.active.empty() || (r.unit1 != world->units.active[0] && r.unit2 != world->units.active[0]))
            {   // Adventure mode reuses a dwarf mode digging designation bit to determine current visibility
                if (!Maps::isValidTilePos(r.pos) || (Maps::getTileDesignation(r.pos)->whole & 0x10) == 0x0)
                {
                    DEBUG(gui).print("Adventure mode announcement not detected:\n%s\n", message.c_str());
                    return false;
                }
            }
        }
    }
    else
    {   // Dwarf mode
        if ((r.unit1 != NULL || r.unit2 != NULL) && (r.unit1 == NULL || Units::isHidden(r.unit1)) && (r.unit2 == NULL || Units::isHidden(r.unit2)))
        {
            DEBUG(gui).print("Dwarf mode announcement not detected:\n%s\n", message.c_str());
            return false;
        }

        if (!a_flags.bits.D_DISPLAY)
        {
            if (a_flags.bits.UNIT_COMBAT_REPORT)
            {
                if (r.unit1 == NULL && r.unit2 == NULL)
                {
                    DEBUG(gui).print("Skipped UNIT_COMBAT_REPORT because it has no units:\n%s\n", message.c_str());
                    return false;
                }
            }
            else
            {
                if (!a_flags.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
                {
                    DEBUG(gui).print("Skipped announcement not enabled for this game mode:\n%s\n", message.c_str());
                    return false;
                }
                else if (!recent_report_any(r.unit1) && !recent_report_any(r.unit2))
                {
                    DEBUG(gui).print("Skipped UNIT_COMBAT_REPORT_ALL_ACTIVE because there's no active report:\n%s\n", message.c_str());
                    return false;
                }
            }
        }
    }

    if (a_flags.bits.PAUSE || a_flags.bits.RECENTER)
        pauseRecenter((a_flags.bits.RECENTER ? r.pos : df::coord()), a_flags.bits.PAUSE); // Does nothing outside dwarf mode

    bool adv_unconscious = (*gamemode == game_mode::ADVENTURE && !world->units.active.empty() && world->units.active[0]->counters.unconscious > 0);

    if (a_flags.bits.DO_MEGA && !adv_unconscious)
        showPopupAnnouncement(message, r.color, r.bright);

    vector<string> results;
    size_t line_length = (r.speaker_id == -1) ? (init->display.grid_x - 7) : (init->display.grid_x - 10);
    parseReportString(&results, message, line_length);

    if (results.empty()) // DF doesn't do this check
    {
        DEBUG(gui).print("Skipped announcement because it was empty after parsing:\n%s\n", message.c_str());
        return false;
    }

    // Check for repeat report
    int32_t repeat_count = check_repeat_report(results); // always returns 0 outside dwarf mode
    if (repeat_count > 0)
    {
        if (a_flags.bits.D_DISPLAY)
        {
            world->status.display_timer = r.display_timer;
            Gui::writeToGamelog("x" + to_string(repeat_count + 1));
        }
        DEBUG(gui).print("Announcement succeeded as repeat:\n%s\n", message.c_str());
        return true;
    }

    size_t new_report_index = world->status.reports.size(); // we need this for addCombatReport
    bool success = false; // only print to gamelog if report was used
    bool display = ((*gamemode == game_mode::ADVENTURE && a_flags.bits.A_DISPLAY) || (*gamemode == game_mode::DWARF && a_flags.bits.D_DISPLAY));

    for (size_t i = 0; i < results.size(); i++)
    {   // Generate report entries for each line
        auto new_report = new df::report();
        new_report->type = r.type;
        new_report->text = results[i];
        new_report->color = r.color;
        new_report->bright = r.bright;
        new_report->flags.whole = 0x0;
        new_report->zoom_type = r.zoom_type;
        new_report->pos = r.pos;
        new_report->zoom_type2 = r.zoom_type2;
        new_report->pos2 = r.pos2;
        new_report->id = world->status.next_report_id++;
        new_report->year = *df::global::cur_year;
        new_report->time = *df::global::cur_year_tick;
        new_report->unk_v40_1 = r.unk_v40_1;
        new_report->unk_v40_2 = r.unk_v40_2;
        new_report->speaker_id = r.speaker_id;
        world->status.reports.push_back(new_report);

        if (i > 0)
            new_report->flags.bits.continuation = true;

        if (adv_unconscious)
            new_report->flags.bits.unconscious = true;

        if (display)
        {
            insert_into_vector(world->status.announcements, &df::report::id, new_report);
            new_report->flags.bits.announcement = true;
            world->status.display_timer = r.display_timer;
            success = true;
        }
    }

    if (*gamemode == game_mode::DWARF) // DF does this inside the previous loop, but we're using addCombatReport instead
    {
        if (a_flags.bits.UNIT_COMBAT_REPORT)
        {
            if (r.unit1 != NULL)
                success |= add_proper_report(r.unit1, !r.flags.bits.hostile_combat, new_report_index);

            if (r.unit2 != NULL)
                success |= add_proper_report(r.unit2, !r.flags.bits.hostile_combat, new_report_index);
        }

        if (a_flags.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
        {
            FOR_ENUM_ITEMS(unit_report_type, slot)
            {
                if (recent_report(r.unit1, slot))
                    success |= addCombatReport(r.unit1, slot, new_report_index);

                if (recent_report(r.unit2, slot))
                    success |= addCombatReport(r.unit2, slot, new_report_index);
            }
        }
    }

    delete_old_reports();

    if (/*debug_gamelog &&*/ success) // TODO: Add debug_gamelog to globals?
    {
        DEBUG(gui).print("Announcement succeeded and printed to gamelog.txt:\n%s\n", message.c_str());
        Gui::writeToGamelog(message);
    }
    /*else if (success)
    {
        DEBUG(gui).print("Announcement succeeded but skipped printing to gamelog.txt because debug_gamelog is false:\n%s\n", message.c_str());
    }*/
    else // not sure if this can actually happen; our results.empty() check handles the one edge case I can think of that would get this far
    {
        DEBUG(gui).print("Announcement succeeded internally but didn't qualify to be displayed anywhere:\n%s\n", message.c_str());
    }

    return true;
}

bool Gui::autoDFAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color,
                             bool bright, df::unit *unit1, df::unit *unit2, bool is_sparring)
{
    auto r = df::report_init();
    r.type = type;
    r.color = color;
    r.bright = bright;
    r.pos = pos;
    r.display_timer = ANNOUNCE_DISPLAY_TIME;
    r.unit1 = unit1;
    r.unit2 = unit2;
    r.flags.bits.hostile_combat = !is_sparring;

    return autoDFAnnouncement(r, message);
}

static df::viewscreen * do_skip_dismissed(df::viewscreen * ws) {
    while (ws && Screen::isDismissed(ws) && ws->parent)
        ws = ws->parent;
    return ws;
}

df::viewscreen *Gui::getCurViewscreen(bool skip_dismissed)
{
    if (!gview)
        return NULL;

    df::viewscreen * ws = &gview->view;
    while (ws && ws->child)
        ws = ws->child;

    if (skip_dismissed)
        ws = do_skip_dismissed(ws);

    return ws;
}

df::viewscreen *Gui::getViewscreenByIdentity (virtual_identity &id, int n)
{
    bool limit = (n > 0);
    df::viewscreen *screen = Gui::getCurViewscreen();
    while (screen)
    {
        if (limit && n-- <= 0)
            break;
        if (id.is_instance(screen))
            return screen;
        screen = screen->parent;
    }
    return NULL;
}

df::viewscreen *Gui::getDFViewscreen(bool skip_dismissed) {
    df::viewscreen *screen = Gui::getCurViewscreen(skip_dismissed);
    while (screen && dfhack_viewscreen::is_instance(screen)) {
        screen = screen->parent;
        if (skip_dismissed)
            screen = do_skip_dismissed(screen);
    }
    return screen;
}

df::coord Gui::getViewportPos()
{
    if (!df::global::window_x || !df::global::window_y || !df::global::window_z)
        return df::coord(0,0,0);

    return df::coord(*df::global::window_x, *df::global::window_y, *df::global::window_z);
}

df::coord Gui::getCursorPos()
{
    using df::global::cursor;
    if (!cursor)
        return df::coord();

    return df::coord(cursor->x, cursor->y, cursor->z);
}

Gui::DwarfmodeDims getDwarfmodeViewDims_default()
{
    Gui::DwarfmodeDims dims;

    bool use_graphics = Screen::inGraphicsMode();
    auto dimx = use_graphics ? gps->main_viewport->dim_x : gps->dimx;
    auto dimy = use_graphics ? gps->main_viewport->dim_y : gps->dimy;

    dims.map_x1 = 0;
    dims.map_x2 = dimx - 1;
    dims.map_y1 = 0;
    dims.map_y2 = dimy - 1;

    return dims;
}

GUI_HOOK_DEFINE(Gui::Hooks::dwarfmode_view_dims, getDwarfmodeViewDims_default);
Gui::DwarfmodeDims Gui::getDwarfmodeViewDims()
{
    return GUI_HOOK_TOP(Gui::Hooks::dwarfmode_view_dims)();
}

void Gui::resetDwarfmodeView(bool pause)
{
    using df::global::cursor;

    if (plotinfo)
    {
        plotinfo->follow_unit = -1;
        plotinfo->follow_item = -1;
/* TODO: understand how this changes for v50
        plotinfo->main.mode = ui_sidebar_mode::Default;
*/
    }

    if (selection_rect)
    {
        selection_rect->start_x = -30000;
        selection_rect->end_x = -30000;
    }

    if (cursor)
        cursor->x = cursor->y = cursor->z = -30000;

    if (pause && df::global::pause_state)
        *df::global::pause_state = true;
}

bool Gui::revealInDwarfmodeMap(int32_t x, int32_t y, int32_t z, bool center)
{   // Reverse-engineered from DF announcement and scrolling code
    using df::global::window_x;
    using df::global::window_y;
    using df::global::window_z;

    if (!window_x || !window_y || !window_z || !world)
        return false;

    auto dims = getDwarfmodeViewDims();
    int32_t w = dims.map_x2 - dims.map_x1 + 1;
    int32_t h = dims.map_y2 - dims.map_y1 + 1;
    int32_t new_win_x, new_win_y, new_win_z;
    getViewCoords(new_win_x, new_win_y, new_win_z);

    if (Maps::isValidTilePos(x, y, z))
    {
        if (center)
        {
            new_win_x = x - w / 2;
            new_win_y = y - h / 2;
        }
        else // just bring it on screen
        {
            if (new_win_x > (x - 5)) // equivalent to: "while (new_win_x > x - 5) new_win_x -= 10;"
                new_win_x -= (new_win_x - (x - 5) - 1) / 10 * 10 + 10;
            if (new_win_y > (y - 5))
                new_win_y -= (new_win_y - (y - 5) - 1) / 10 * 10 + 10;
            if (new_win_x < (x + 5 - w))
                new_win_x += ((x + 5 - w) - new_win_x - 1) / 10 * 10 + 10;
            if (new_win_y < (y + 5 - h))
                new_win_y += ((y + 5 - h) - new_win_y - 1) / 10 * 10 + 10;
        }

        new_win_z = z;
    }

    *window_x = clip_range(new_win_x, 0, (world->map.x_count - w));
    *window_y = clip_range(new_win_y, 0, (world->map.y_count - h));
    *window_z = clip_range(new_win_z, 0, (world->map.z_count - 1));
    game->minimap.update = true;
    game->minimap.mustmake = true;

    return true;
}

bool Gui::pauseRecenter(int32_t x, int32_t y, int32_t z, bool pause)
{   // Reverse-engineered from DF announcement code
    if (*gamemode != game_mode::DWARF)
        return false;

    resetDwarfmodeView(pause);

    if (Maps::isValidTilePos(x, y, z))
        revealInDwarfmodeMap(x, y, z, false);

    if (init->input.pause_zoom_no_interface_ms > 0)
    {
        gview->shutdown_interface_tickcount = Core::getInstance().p->getTickCount();
        gview->shutdown_interface_for_ms = init->input.pause_zoom_no_interface_ms;
    }

    return true;
}

bool Gui::refreshSidebar()
{
    auto scr = getViewscreenByType<df::viewscreen_dwarfmodest>(0);
    if (scr)
    {
        if (df::global::window_z && *df::global::window_z == 0)
        {
            scr->feed_key(interface_key::CURSOR_UP_Z);
            scr->feed_key(interface_key::CURSOR_DOWN_Z);
        }
        else
        {
            scr->feed_key(interface_key::CURSOR_DOWN_Z);
            scr->feed_key(interface_key::CURSOR_UP_Z);
        }
        return true;
    }
    return false;
}

bool Gui::inRenameBuilding()
{
    if (!game)
        return false;
    /* TODO: understand how this changes for v50
    return game->barracks.in_rename;
    */
    return false;
}

bool Gui::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    x = *df::global::window_x;
    y = *df::global::window_y;
    z = *df::global::window_z;
    return true;
}

bool Gui::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    (*df::global::window_x) = x;
    (*df::global::window_y) = y;
    (*df::global::window_z) = z;
    return true;
}

bool Gui::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    x = df::global::cursor->x;
    y = df::global::cursor->y;
    z = df::global::cursor->z;
    return has_cursor();
}

bool Gui::getCursorCoords (df::coord &pos)
{
    pos.x = df::global::cursor->x;
    pos.y = df::global::cursor->y;
    pos.z = df::global::cursor->z;
    return has_cursor();
}

//FIXME: confine writing of coords to map bounds?
bool Gui::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    df::global::cursor->x = x;
    df::global::cursor->y = y;
    df::global::cursor->z = z;
    return true;
}

bool Gui::getDesignationCoords (int32_t &x, int32_t &y, int32_t &z)
{
    x = selection_rect->start_x;
    y = selection_rect->start_y;
    z = selection_rect->start_z;
    return (x == -30000) ? false : true;
}

bool Gui::setDesignationCoords (const int32_t x, const int32_t y, const int32_t z)
{
    selection_rect->start_x = x;
    selection_rect->start_y = y;
    selection_rect->start_z = z;
    return true;
}

// returns the map coordinates that the mouse cursor is over
df::coord Gui::getMousePos()
{
    df::coord pos;
    if (gps && gps->precise_mouse_x > -1) {
        pos = getViewportPos();
        if (Screen::inGraphicsMode()) {
            int32_t map_tile_pixels = gps->viewport_zoom_factor / 4;
            pos.x += gps->precise_mouse_x / map_tile_pixels;
            pos.y += gps->precise_mouse_y / map_tile_pixels;
        } else {
            pos.x += gps->mouse_x;
            pos.y += gps->mouse_y;
        }
    }
    if (!Maps::isValidTilePos(pos.x, pos.y, pos.z))
        return df::coord();
    return pos;
}

int getDepthAt_default (int32_t x, int32_t y)
{
    auto &main_vp = gps->main_viewport;
    if (x < 0 || x >= main_vp->dim_x || y < 0 || y >= main_vp->dim_y)
        return 0;
    const size_t num_viewports = gps->viewport.size();
    const size_t index = (x * main_vp->dim_y) + y;
    for (size_t depth = 0; depth < num_viewports; ++depth) {
        if (gps->viewport[depth]->screentexpos_background[index])
            return depth;
    }
    return 0;
}

GUI_HOOK_DEFINE(Gui::Hooks::depth_at, getDepthAt_default);
int Gui::getDepthAt (int32_t x, int32_t y)
{
    return GUI_HOOK_TOP(Gui::Hooks::depth_at)(x, y);
}

bool Gui::getWindowSize (int32_t &width, int32_t &height)
{
    if (gps) {
        width = gps->dimx;
        height = gps->dimy;
        return true;
    }
    else {
        width = 80;
        height = 25;
        return false;
    }
}
