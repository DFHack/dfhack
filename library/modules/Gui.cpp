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
#include "DataDefs.h"

#include "modules/Job.h"
#include "modules/Screen.h"
#include "modules/Maps.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/adventurest.h"
#include "df/announcement_alertst.h"
#include "df/announcement_flags.h"
#include "df/announcement_infost.h"
#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/building_def_workshopst.h"
#include "df/building_def_furnacest.h"
#include "df/building_furnacest.h"
#include "df/building_trapst.h"
#include "df/building_type.h"
#include "df/building_workshopst.h"
#include "df/buildreq.h"
#include "df/cri_unitst.h"
#include "df/d_init.h"
#include "df/enabler.h"
#include "df/game_mode.h"
#include "df/gamest.h"
#include "df/general_ref.h"
#include "df/global_objects.h"
#include "df/graphic.h"
#include "df/graphic_viewportst.h"
#include "df/historical_figure.h"
#include "df/init.h"
#include "df/interfacest.h"
#include "df/item_corpsepiecest.h"
#include "df/item_corpsest.h"
#include "df/job.h"
#include "df/legend_pagest.h"
#include "df/markup_text_boxst.h"
#include "df/markup_text_linkst.h"
#include "df/markup_text_wordst.h"
#include "df/occupation.h"
#include "df/plant.h"
#include "df/plotinfost.h"
#include "df/popup_message.h"
#include "df/report.h"
#include "df/report_zoom_type.h"
#include "df/route_stockpile_link.h"
#include "df/soundst.h"
#include "df/stop_depart_condition.h"
#include "df/ui_unit_view_mode.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_legendsst.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_titlest.h"
#include "df/viewscreen_worldst.h"
#include "df/world.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using namespace DFHack;

const size_t MAX_REPORTS_SIZE = 3000; // DF clears old reports to maintain this vector size
const int32_t RECENT_REPORT_TICKS = 500; // used by UNIT_COMBAT_REPORT_ALL_ACTIVE
const int32_t ANNOUNCE_LINE_DURATION = 100; // time to display each line in announcement bar; 2 sec at 50 GFPS
const int16_t ANNOUNCE_DISPLAY_TIME = 2000; // DF uses this value for most announcements; 40 sec at 50 GFPS

namespace DFHack
{
    DBG_DECLARE(core, gui, DebugCategory::LINFO);
}

using namespace df::enums;

using df::building_civzonest;
using df::global::game;
using df::global::gamemode;
using df::global::gps;
using df::global::gview;
using df::global::init;
using df::global::plotinfo;
using df::global::selection_rect;
using df::global::ui_menu_width;
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

typedef void (*getFocusStringsHandler)(std::string &str, std::vector<std::string> &strList, df::viewscreen *screen);
static std::map<virtual_identity*, getFocusStringsHandler> getFocusStringsHandlers;

#define VIEWSCREEN(name) df::viewscreen_##name##st
#define DEFINE_GET_FOCUS_STRING_HANDLER(screen_type) \
    static void getFocusStrings_##screen_type(const std::string &baseFocus, std::vector<std::string> &focusStrings, VIEWSCREEN(screen_type) *screen);\
    DFHACK_STATIC_ADD_TO_MAP(\
        &getFocusStringsHandlers, &VIEWSCREEN(screen_type)::_identity, \
        (getFocusStringsHandler)getFocusStrings_##screen_type \
    ); \
    static void getFocusStrings_##screen_type(const std::string &baseFocus, std::vector<std::string> &focusStrings, VIEWSCREEN(screen_type) *screen)

DEFINE_GET_FOCUS_STRING_HANDLER(title)
{
    if (screen->managing_mods)
        focusStrings.push_back(baseFocus + "/Mods");
    else if (game->main_interface.settings.open)
        focusStrings.push_back(baseFocus + "/Settings");

    if (focusStrings.empty())
        focusStrings.push_back(baseFocus + "/Default");
}

DEFINE_GET_FOCUS_STRING_HANDLER(new_region)
{
    if (screen->raw_load)
        focusStrings.push_back(baseFocus + "/Loading");
    else if (screen->doing_mods)
        focusStrings.push_back(baseFocus + "/Mods");
    else if (screen->doing_simple_params)
        focusStrings.push_back(baseFocus + "/Basic");
    else if (screen->doing_params)
        focusStrings.push_back(baseFocus + "/Advanced");

    if (focusStrings.empty())
        focusStrings.push_back(baseFocus);
}

DEFINE_GET_FOCUS_STRING_HANDLER(choose_start_site)
{
    if (screen->doing_site_finder)
        focusStrings.push_back(baseFocus + "/SiteFinder");
    else if (screen->choosing_civilization)
        focusStrings.push_back(baseFocus + "/ChooseCiv");
    else if (screen->choosing_reclaim)
        focusStrings.push_back(baseFocus + "/Reclaim");

    if (focusStrings.empty())
        focusStrings.push_back(baseFocus);
}

DEFINE_GET_FOCUS_STRING_HANDLER(setupdwarfgame)
{
    if (screen->doing_custom_settings)
        focusStrings.push_back(baseFocus + "/CustomSettings");
    else if (game->main_interface.options.open)
        focusStrings.push_back(baseFocus + "/Abort");
    else if (screen->initial_selection == 1)
        focusStrings.push_back(baseFocus + "/Default");
    else if (game->main_interface.name_creator.open) {
        switch (game->main_interface.name_creator.context) {
        case df::name_creator_context_type::EMBARK_FORT_NAME:
            focusStrings.push_back(baseFocus + "/FortName");
            break;
        case df::name_creator_context_type::EMBARK_GROUP_NAME:
            focusStrings.push_back(baseFocus + "/GroupName");
            break;
        case df::name_creator_context_type::IMAGE_CREATOR_NAME:
            focusStrings.push_back(baseFocus + "/ImageName");
            break;
        default:
            break;
        }
    }
    else if (game->main_interface.image_creator.open) {
        focusStrings.push_back(baseFocus + "/GroupSymbol");
    }
    else if (screen->viewing_objections != 0)
        focusStrings.push_back(baseFocus + "/Objections");
    else {
        switch (screen->mode) {
        case 0: focusStrings.push_back(baseFocus + "/Dwarves"); break;
        case 1: focusStrings.push_back(baseFocus + "/Items"); break;
        case 2: focusStrings.push_back(baseFocus + "/Animals"); break;
        default:
            break;
        }
    }

    if (focusStrings.empty())
        focusStrings.push_back(baseFocus + "/Default");
}

DEFINE_GET_FOCUS_STRING_HANDLER(legends)
{
    if (screen->init_stage != -1)
        focusStrings.push_back(baseFocus + "/Loading");
    else if (screen->page.size() <= 1)
        focusStrings.push_back(baseFocus + "/Default");
    else
        focusStrings.push_back(baseFocus + '/' + screen->page[screen->active_page_index]->header);
}

DEFINE_GET_FOCUS_STRING_HANDLER(world)
{
    focusStrings.push_back(baseFocus + '/' + enum_item_key(screen->view_mode));
}

static bool widget_is_visible(df::widget * w) {
    return w && w->flag.bits.VISIBILITY_VISIBLE;
}

static size_t get_num_children(df::widget * w) {
    if (!w)
        return 0;
    df::widget_container *container = virtual_cast<df::widget_container>(w);
    if (!container)
        return 0;
    return container->children.size();
}

static df::widget * get_visible_child(df::widget *parent) {
    df::widget_container *container = virtual_cast<df::widget_container>(parent);
    if (!container)
        return NULL;
    for (auto child : container->children) {
        if (widget_is_visible(child.get()))
            return child.get();
    }
    return NULL;
}

static df::widget_container * get_visible_child_container(df::widget *parent) {
    df::widget_container *container = virtual_cast<df::widget_container>(parent);
    if (!container)
        return NULL;
    for (auto child : container->children) {
        if (widget_is_visible(child.get()))
            return virtual_cast<df::widget_container>(child.get());
    }
    return NULL;
}

static string get_building_custom_name(df::building_workshopst * bld) {
    if (bld->type != df::workshop_type::Custom)
        return "";
    if (auto bld_def = vector_get(world->raws.buildings.workshops, bld->custom_type))
        return bld_def->code + '/';
    return "";
}

static string get_building_custom_name(df::building_furnacest * bld) {
    if (bld->type != df::furnace_type::Custom)
        return "";
    if (auto bld_def = vector_get(world->raws.buildings.furnaces, bld->custom_type))
        return bld_def->code + '/';
    return "";
}

static string get_building_custom_name(df::building_trapst * bld) {
    return "";
}

template<typename T>
static void add_profile_tab_focus_string(
    vector<string> & focusStrings, const string & base, T * bld)
{
    string fs = base + '/' + get_building_custom_name(bld);
    switch (game->main_interface.view_sheets.active_sub_tab) {
    case 0: fs += "Tasks"; break;
    case 1: fs += "Workers"; break;
    case 2: fs += "WorkOrders"; break;
    default:
        return;
    }
    focusStrings.push_back(fs);
}

static void add_main_interface_focus_strings(const string &baseFocus, vector<string> &focusStrings) {
    std::string newFocusString;

    if (game->main_interface.main_designation_selected != -1) {
        newFocusString = baseFocus;
        newFocusString += "/Designate/" + enum_item_key(game->main_interface.main_designation_selected);
        focusStrings.push_back(newFocusString);
    }
    if (get_visible_child(&game->main_interface.announcements.stack)) {
        focusStrings.push_back(baseFocus + "/Announcements");
    }
    if (game->main_interface.info.open) {
        newFocusString = baseFocus;
        newFocusString += "/Info";
        newFocusString += '/' + enum_item_key(game->main_interface.info.current_mode);

        switch(game->main_interface.info.current_mode) {
        case df::enums::info_interface_mode_type::CREATURES:
        {
            auto tab = get_visible_child_container(Gui::getWidget(&game->main_interface.info.creatures, "Tabs"));
            if (!tab) {
                WARN(gui).print("Info tab widget not found\n");
            } else if (tab->name == "Citizens") {
                if (tab->children.size() > 1)
                    newFocusString += "/ActivityDetails";
                else
                    newFocusString += "/CITIZEN";
            } else if (tab->name == "Pets/Livestock") {
                if (Gui::getWidget(tab, "Overall training"))
                    newFocusString += "/OverallTraining";
                else if (Gui::getWidget(tab, "Trainer"))
                    newFocusString += "/AddingTrainer";
                else if (Gui::getWidget(tab, "Hunting assignment"))
                    newFocusString += "/AssignWorkAnimal";
                else
                    newFocusString += "/PET";
            } else if (tab->name == "Other") {
                newFocusString += "/OTHER";
            } else if (tab->name == "Dead/Missing") {
                newFocusString += "/DECEASED";
            }
            break;
        }
        case df::enums::info_interface_mode_type::BUILDINGS:
            newFocusString += '/' + enum_item_key(game->main_interface.info.buildings.mode);
            break;
        case df::enums::info_interface_mode_type::LABOR:
        {
            auto tab = get_visible_child(Gui::getWidget(&game->main_interface.info.labor, "Tabs"));
            if (!tab) {
                WARN(gui).print("Labor tab widget not found\n");
            } else if (tab->name == "Work Details") {
                newFocusString += "/WORK_DETAILS";
                auto rp = Gui::getWidget(virtual_cast<df::labor_work_details_interfacest>(tab), "Right panel");
                if (get_num_children(rp) == 2)
                    newFocusString += "/Details";
                else
                    newFocusString += "/Default";
            } else if (tab->name == "Standing orders") {
                newFocusString += "/STANDING_ORDERS";
                auto lsoi = virtual_cast<df::labor_standing_orders_interfacest>(tab);
                newFocusString += '/' + enum_item_key(lsoi->current_category);
            } else if (tab->name == "Kitchen") {
                newFocusString += "/KITCHEN";
                auto lki = virtual_cast<df::labor_kitchen_interfacest>(tab);
                newFocusString += '/' + enum_item_key(lki->current_category);
            } else if (tab->name == "Stone use") {
                newFocusString += "/STONE_USE";
                auto lsui = virtual_cast<df::labor_stone_use_interfacest>(tab);
                newFocusString += '/' + enum_item_key(lsui->current_category);
            }
            break;
        }
        case df::enums::info_interface_mode_type::ARTIFACTS:
            newFocusString += '/' + enum_item_key(game->main_interface.info.artifacts.mode);
            break;
        case df::enums::info_interface_mode_type::JUSTICE:
        {
            auto tab = get_visible_child(Gui::getWidget(&game->main_interface.info.justice, "Tabs"));
            if (!tab) {
                WARN(gui).print("Justice tab widget not found\n");
            } else if (tab->name == "Open cases") {
                auto * container_tab = virtual_cast<df::widget_container>(tab);
                auto * right_panel = container_tab ?
                    virtual_cast<df::widget_container>(Gui::getWidget(container_tab, "Right panel")) :
                    NULL;
                bool is_valid = right_panel && right_panel->children.size() > 1;
                if (is_valid && right_panel->children[1]->name == "Interrogate")
                    newFocusString += "/Interrogating";
                else if (is_valid && right_panel->children[1]->name == "Convict")
                    newFocusString += "/Convicting";
                else
                    newFocusString += "/OPEN_CASES";
            } else if (tab->name == "Closed cases") {
                newFocusString += "/CLOSED_CASES";
            } else if (tab->name == "Cold cases") {
                auto * container_tab = virtual_cast<df::widget_container>(tab);
                auto * right_panel = container_tab ?
                    virtual_cast<df::widget_container>(Gui::getWidget(container_tab, "Right panel")) :
                    NULL;
                bool is_valid = right_panel && right_panel->children.size() > 1;
                if (is_valid && right_panel->children[1]->name == "Interrogate")
                    newFocusString += "/Interrogating";
                else if (is_valid && right_panel->children[1]->name == "Convict")
                    newFocusString += "/Convicting";
                else
                    newFocusString += "/COLD_CASES";
            } else if (tab->name == "Fortress guard") {
                newFocusString += "/FORTRESS_GUARD";
            } else if (tab->name == "Convicts") {
                newFocusString += "/CONVICTS";
            } else if (tab->name == "Intelligence") {
                newFocusString += "/COUNTERINTELLIGENCE";
            }
            break;
        }
        case df::enums::info_interface_mode_type::WORK_ORDERS:
            if (game->main_interface.info.work_orders.conditions.open) {
                newFocusString += "/Conditions";
                if (game->main_interface.info.work_orders.conditions.change_type != df::work_order_condition_change_type::NONE)
                    newFocusString += '/' + enum_item_key(game->main_interface.info.work_orders.conditions.change_type);
                else
                    newFocusString += "/Default";
            }
            else if (game->main_interface.create_work_order.open)
                newFocusString += "/Create";
            else
                newFocusString += "/Default";
            break;
        case df::enums::info_interface_mode_type::ADMINISTRATORS:
            if (game->main_interface.info.administrators.choosing_candidate)
                newFocusString += "/Candidates";
            else if (game->main_interface.info.administrators.assigning_symbol)
                newFocusString += "/Symbols";
            else
                newFocusString += "/Default";
            break;
        default:
            break;
        }

        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.view_sheets.open) {
        newFocusString = baseFocus;
        newFocusString += "/ViewSheets";
        newFocusString += '/' + enum_item_key(game->main_interface.view_sheets.active_sheet);
        switch (game->main_interface.view_sheets.active_sheet) {
        case df::view_sheet_type::UNIT:
            switch (game->main_interface.view_sheets.active_sub_tab) {
            case 0: newFocusString += "/Overview"; break;
            case 1: newFocusString += "/Items"; break;
            case 2:
                newFocusString += "/Health";
                switch (game->main_interface.view_sheets.unit_health_active_tab) {
                    case 0: newFocusString += "/Status"; break;
                    case 1: newFocusString += "/Wounds"; break;
                    case 2: newFocusString += "/Treatment"; break;
                    case 3: newFocusString += "/History"; break;
                    case 4: newFocusString += "/Description"; break;
                    default: break;
                }
                break;
            case 3:
                newFocusString += "/Skills";
                switch (game->main_interface.view_sheets.unit_skill_active_tab) {
                    case 0: newFocusString += "/Labor"; break;
                    case 1: newFocusString += "/Combat"; break;
                    case 2: newFocusString += "/Social"; break;
                    case 3: newFocusString += "/Other"; break;
                    case 4:
                        newFocusString += "/Knowledge";
                        if (game->main_interface.view_sheets.skill_description_raw_str.size())
                            newFocusString += "/Details";
                        else
                            newFocusString += "/Default";
                        break;
                    default: break;
                }
                break;
            case 4: newFocusString += "/Rooms"; break;
            case 5:
                newFocusString += "/Labor";
                switch (game->main_interface.view_sheets.unit_labor_active_tab) {
                    case 0: newFocusString += "/WorkDetails"; break;
                    case 1: newFocusString += "/Workshops"; break;
                    case 2: newFocusString += "/Locations"; break;
                    case 3: newFocusString += "/WorkAnimals"; break;
                    default: break;
                }
                break;
            case 6: newFocusString += "/Relations"; break;
            case 7: newFocusString += "/Groups"; break;
            case 8:
                newFocusString += "/Military";
                switch (game->main_interface.view_sheets.unit_military_active_tab) {
                    case 0: newFocusString += "/Squad"; break;
                    case 1: newFocusString += "/Uniform"; break;
                    case 2: newFocusString += "/Kills"; break;
                    default: break;
                }
                break;
            case 9:
                newFocusString += "/Thoughts";
                switch (game->main_interface.view_sheets.thoughts_active_tab) {
                    case 0: newFocusString += "/Recent"; break;
                    case 1: newFocusString += "/Memories"; break;
                    default: break;
                }
                break;
            case 10:
                newFocusString += "/Personality";
                switch (game->main_interface.view_sheets.personality_active_tab) {
                    case 0: newFocusString += "/Traits"; break;
                    case 1: newFocusString += "/Values"; break;
                    case 2: newFocusString += "/Preferences"; break;
                    case 3: newFocusString += "/Needs"; break;
                    default: break;
                }
                break;
            default:
                break;
            }
            break;
        case df::view_sheet_type::BUILDING:
            if (game->main_interface.view_sheets.linking_lever)
                newFocusString += "/LinkingLever";
            else if (game->main_interface.stockpile_link.open && game->main_interface.stockpile_link.adding_new_link)
                newFocusString += "/LinkingStockpile";
            else if (auto bld = df::building::find(game->main_interface.view_sheets.viewing_bldid)) {
                newFocusString += '/' + enum_item_key(bld->getType());
                auto bld_type = bld->getType();
                if (bld_type == df::enums::building_type::Trap) {
                    if (auto trap = strict_virtual_cast<df::building_trapst>(bld)) {
                        newFocusString += '/' + enum_item_key(trap->trap_type);
                        if (trap->trap_type == df::trap_type::Lever)
                            add_profile_tab_focus_string(focusStrings, newFocusString, trap);
                    }
                } else if (bld_type == df::enums::building_type::Workshop) {
                    if (auto ws = strict_virtual_cast<df::building_workshopst>(bld)) {
                        newFocusString += '/' + enum_item_key(ws->type);
                        add_profile_tab_focus_string(focusStrings, newFocusString, ws);
                    }
                } else if (bld_type == df::enums::building_type::Furnace) {
                    if (auto furn = strict_virtual_cast<df::building_furnacest>(bld)) {
                        newFocusString += '/' + enum_item_key(furn->type);
                        add_profile_tab_focus_string(focusStrings, newFocusString, furn);
                    }
                }
                if (game->main_interface.view_sheets.show_linked_buildings)
                    newFocusString += "/LinkedBuildings";
                else
                    newFocusString += "/Items";
            }
            break;
        default:
            break;
        }
        focusStrings.push_back(newFocusString);
    }

    if (game->main_interface.bottom_mode_selected != -1) {
        newFocusString = baseFocus;

        switch(game->main_interface.bottom_mode_selected) {
        case df::enums::main_bottom_mode_type::STOCKPILE:
            newFocusString += "/Stockpile";
            if (game->main_interface.stockpile.cur_bld) {
                newFocusString += "/Some";
                if (game->main_interface.stockpile_link.open)
                    newFocusString += "/Links";
                else if (game->main_interface.stockpile_tools.open)
                    newFocusString += "/Containers";
                else if (game->main_interface.custom_stockpile.open)
                    newFocusString += "/Customize";
                else
                    newFocusString += "/Default";
            }
            break;
        case df::enums::main_bottom_mode_type::STOCKPILE_PAINT:
            newFocusString += "/Stockpile/Paint";
            break;
        case df::enums::main_bottom_mode_type::HAULING:
            newFocusString += "/Hauling";
            break;
        case df::enums::main_bottom_mode_type::ZONE:
            newFocusString += "/Zone";
            if (game->main_interface.civzone.cur_bld) {
                newFocusString += "/Some";
                newFocusString += '/' + enum_item_key(game->main_interface.civzone.cur_bld->type);
            }
            break;
        case df::enums::main_bottom_mode_type::ZONE_PAINT:
            newFocusString += "/Zone/Paint/" + enum_item_key(game->main_interface.civzone.adding_new_type);
            break;
        case df::enums::main_bottom_mode_type::BURROW:
            newFocusString += "/Burrow";
            break;
        case df::enums::main_bottom_mode_type::BURROW_PAINT:
            newFocusString += "/Burrow/Paint";
            break;
        case df::enums::main_bottom_mode_type::BUILDING:
            newFocusString += "/Building";
            break;
        case df::enums::main_bottom_mode_type::BUILDING_PLACEMENT:
            newFocusString += "/Building/Placement";
            break;
        case df::enums::main_bottom_mode_type::BUILDING_PICK_MATERIALS:
            newFocusString += "/Building/PickMaterials";
            break;
        default:
            break;
        }

        focusStrings.push_back(newFocusString);
    }

    if (game->main_interface.trade.open) {
        newFocusString = baseFocus;
        newFocusString += "/Trade";
        if (game->main_interface.trade.choosing_merchant)
            newFocusString += "/ChoosingMerchant";
        else
            newFocusString += "/Default";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.job_details.open) {
        newFocusString = baseFocus;
        newFocusString += "/JobDetails/" + enum_item_key(game->main_interface.job_details.context);
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.assign_trade.open) {
        newFocusString = baseFocus;
        newFocusString += "/AssignTrade";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.diplomacy.open) {
        newFocusString = baseFocus;
        newFocusString += "/Diplomacy";
        if (game->main_interface.diplomacy.taking_requests)
            newFocusString += "/Requests";
        else if (game->main_interface.diplomacy.selecting_land_holder_position)
            newFocusString += "/ElevateLandHolder";
        else
            newFocusString += "/Default";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.petitions.open) {
        newFocusString = baseFocus;
        newFocusString += "/Petitions";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.stocks.open) {
        newFocusString = baseFocus;
        newFocusString += "/Stocks";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.assign_display_item.open) {
        newFocusString = baseFocus;
        newFocusString += "/AssignDisplayItem";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.name_creator.open) {
        newFocusString = baseFocus;
        newFocusString += "/NameCreator";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.image_creator.open) {
        newFocusString = baseFocus;
        newFocusString += "/ImageCreator";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.unit_selector.flag.bits.VISIBILITY_ACTIVE) {
        newFocusString = baseFocus;
        newFocusString += "/UnitSelector/";
        newFocusString += enum_item_key(game->main_interface.unit_selector.context);
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.announcement_alert.open) {
        newFocusString = baseFocus;
        newFocusString += "/AnnouncementAlert";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.custom_symbol.open) {
        newFocusString = baseFocus;
        newFocusString += "/CustomSymbol";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.patrol_routes.open) {
        newFocusString = baseFocus;
        newFocusString += "/PatrolRoutes";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.squad_selector.open) {
        newFocusString = baseFocus;
        newFocusString += "/SquadSelector";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.burrow_selector.open) {
        newFocusString = baseFocus;
        newFocusString += "/BurrowSelector";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.location_selector.open) {
        newFocusString = baseFocus;
        newFocusString += "/LocationSelector/";
        if (game->main_interface.location_selector.choosing_temple_religious_practice)
            newFocusString += "Temple";
        else if (game->main_interface.location_selector.choosing_craft_guild)
            newFocusString += "Guildhall";
        else
            newFocusString += "Default";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.location_details.open) {
        newFocusString = baseFocus;
        newFocusString += "/LocationDetails";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.hauling_stop_conditions.open) {
        newFocusString = baseFocus;
        newFocusString += "/HaulingStopConditions";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.assign_vehicle.open) {
        newFocusString = baseFocus;
        newFocusString += "/AssignVehicle";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.create_squad.open) {
        newFocusString = baseFocus;
        newFocusString += "/CreateSquad";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.squad_supplies.open) {
        newFocusString = baseFocus;
        newFocusString += "/SquadSupplies";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.squads.open) {
        newFocusString = baseFocus;
        newFocusString += "/Squads";
        if (game->main_interface.squads.editing_squad_schedule_id >= 0) {
            newFocusString += "/EditingSchedule";
        } else if (game->main_interface.squad_schedule.open) {
            newFocusString += "/Schedule";
        } else if (game->main_interface.squad_equipment.open) {
            newFocusString += "/Equipment";
            if (game->main_interface.squad_equipment.customizing_equipment) {
                newFocusString += "/Customizing";
                if (game->main_interface.squad_equipment.cs_setting_material)
                    newFocusString += "/Material";
                else if (game->main_interface.squad_equipment.cs_setting_color_pattern)
                    newFocusString += "/Color";
                else
                    newFocusString += "/Default";
            }
            else
                newFocusString += "/Default";
        } else {
            newFocusString += "/Default";
        }
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.assign_uniform.open) {
        newFocusString = baseFocus;
        newFocusString += "/AssignUniform";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.hotkey.open) {
        newFocusString = baseFocus;
        newFocusString += "/Hotkey";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.options.open) {
        newFocusString = baseFocus;
        newFocusString += "/Options";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.help.open) {
        newFocusString = baseFocus;
        newFocusString += "/Help";
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.settings.open) {
        newFocusString = baseFocus;
        newFocusString += "/Settings";
        newFocusString += '/' + enum_item_key(game->main_interface.settings.current_mode);
        if (game->main_interface.settings.current_mode == df::settings_tab_type::DIFFICULTY) {
            if (game->main_interface.settings.doing_custom_settings)
                newFocusString += "/CustomSettings";
            else
                newFocusString += "/Default";
        }
        focusStrings.push_back(newFocusString);
    }
    if (game->main_interface.adventure.aim_projectile.open) {
        focusStrings.push_back(baseFocus + "/AimProjectile");
    }
    if (game->main_interface.adventure.announcements.open) {
        focusStrings.push_back(baseFocus + "/Announcements");
    }
    if (game->main_interface.adventure.attack.open) {
        focusStrings.push_back(baseFocus + "/Attack");
    }
    if (game->main_interface.adventure.combat_pref.open) {
        focusStrings.push_back(baseFocus + "/CombatPref");
    }
    if (game->main_interface.adventure.companions.open) {
        focusStrings.push_back(baseFocus + "/Companions");
    }
    if (game->main_interface.adventure.conversation.open) {
        focusStrings.push_back(baseFocus + "/Conversation");
    }
    if (game->main_interface.adventure.inventory.open) {
        focusStrings.push_back(baseFocus + "/Inventory");
    }
    if (game->main_interface.adventure.jump.open) {
        focusStrings.push_back(baseFocus + "/Jump");
    }
    if (game->main_interface.adventure.movement_options.open) {
        focusStrings.push_back(baseFocus + "/MovementOptions");
    }
    if (game->main_interface.adventure.option_list.open) {
        focusStrings.push_back(baseFocus + "/OptionList");
    }
    if (game->main_interface.adventure.perform.open) {
        focusStrings.push_back(baseFocus + "/Perform");
    }
    if (game->main_interface.adventure.sleep.open) {
        focusStrings.push_back(baseFocus + "/Sleep");
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(dwarfmode)
{
    std::string newFocusString;

    if (df::global::gametype && !World::isFortressMode()) {
        newFocusString = baseFocus;
        newFocusString += '/' + enum_item_key(*df::global::gametype);
        if (*df::global::gametype == df::game_type::DWARF_ARENA) {
            if (game->main_interface.bottom_mode_selected != df::main_bottom_mode_type::NONE)
                newFocusString += "/Paint/" + enum_item_key(game->main_interface.bottom_mode_selected);
            else if (game->main_interface.arena_unit.open)
                newFocusString += "/ConfigureUnit";
            else if (game->main_interface.arena_tree.open)
                newFocusString += "/ConfigureTree";
            else
                newFocusString += "/Default";
        }
        focusStrings.push_back(newFocusString);
    }
    add_main_interface_focus_strings(baseFocus, focusStrings);

    static const string squads_default = "dwarfmode/Squads/Default";
    if (!focusStrings.size() || (focusStrings.size() == 1 && focusStrings[0] == squads_default)) {
        focusStrings.push_back(baseFocus + "/Default");
    }
}

DEFINE_GET_FOCUS_STRING_HANDLER(dungeonmode)
{
    std::string newFocusString;

    if (df::global::gametype && !World::isAdventureMode()) {
        newFocusString = baseFocus;
        newFocusString += '/' + enum_item_key(*df::global::gametype);
        focusStrings.push_back(newFocusString);
    }
    add_main_interface_focus_strings(baseFocus, focusStrings);

    if (!focusStrings.size()) {
        focusStrings.push_back(baseFocus + '/' + enum_item_key(df::global::adventure->menu));
    }
}

static std::unordered_map<df::viewscreen *, vector<string>> cached_focus_strings;

void Gui::clearFocusStringCache() {
    cached_focus_strings.clear();
}

bool Gui::matchFocusString(std::string focus_string, df::viewscreen *top) {
    if (!top)
        top = getCurViewscreen(true);

    if (!cached_focus_strings.contains(top))
        cached_focus_strings[top] = getFocusStrings(top);

    vector<string> &cached = cached_focus_strings[top];
    return std::find_if(cached.begin(), cached.end(), [&focus_string](const std::string &item) {
        return prefix_matches(focus_string, item);
    }) != cached.end();
}

static void push_dfhack_focus_string(dfhack_viewscreen *vs, std::vector<std::string> &focusStrings)
{
    auto name = vs->getFocusString();
    if (name.empty())
        name = "dfhack";
    else if (string::npos == name.find("dfhack/"))
        name = "dfhack/" + name;

    focusStrings.push_back(name);
}

std::vector<std::string> Gui::getFocusStrings(df::viewscreen* top)
{
    std::vector<std::string> focusStrings;

    if (!top)
        return focusStrings;

    if (dfhack_viewscreen::is_instance(top))
    {
        dfhack_viewscreen *vs = static_cast<dfhack_viewscreen*>(top);
        if (vs->isFocused())
        {
            push_dfhack_focus_string(vs, focusStrings);
            return focusStrings;
        }
        top = Gui::getDFViewscreen(top);
        if (dfhack_viewscreen::is_instance(top))
        {
            push_dfhack_focus_string(static_cast<dfhack_viewscreen*>(top), focusStrings);
            return focusStrings;
        }
    }

    if (virtual_identity *id = virtual_identity::get(top))
    {
        std::string name = getNameChunk(id, 11, 2);

        auto handler = map_find(getFocusStringsHandlers, id);
        if (handler)
            handler(name, focusStrings, top);
    }

    if (!focusStrings.size())
    {
        Core &core = Core::getInstance();
        std::string name = core.p->readClassName(*(void**)top);
        focusStrings.push_back(name.substr(11, name.size()-11-2));
    }
    return focusStrings;
}

// Predefined common guard functions

bool Gui::default_hotkey(df::viewscreen *top)
{
    return World::isFortressMode() || World::isAdventureMode();
}

bool Gui::anywhere_hotkey(df::viewscreen *) {
    return true;
}

bool Gui::dwarfmode_hotkey(df::viewscreen *top) {
    return matchFocusString("dwarfmode", top);
}

static bool has_cursor()
{
    return Gui::getCursorPos().isValid();
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

    df::building *selected = getAnyBuilding(top);
    if (!virtual_cast<df::building_workshopst>(selected) &&
            !virtual_cast<df::building_furnacest>(selected))
        return false;

    if (selected->jobs.empty() ||
            selected->jobs[0]->job_type == job_type::DestroyBuilding)
        return false;

    return true;
}

bool Gui::build_selector_hotkey(df::viewscreen *top)
{
    using df::global::buildreq;

    if (!dwarfmode_hotkey(top))
        return false;

    if (buildreq->building_type < 0 ||
            buildreq->stage != 2 ||
            buildreq->choices.empty())
        return false;

    return true;
}

bool Gui::view_unit_hotkey(df::viewscreen *top)
{
    if (!dwarfmode_hotkey(top))
        return false;

    return !!getAnyUnit(top);
}

bool Gui::any_job_hotkey(df::viewscreen *top)
{
    return matchFocusString("dwarfmode/Info/JOBS", top)
            || matchFocusString("dwarfmode/Info/CREATURES/CITIZEN", top)
            || workshop_job_hotkey(top);
}

df::job *Gui::getAnyWorkshopJob(df::viewscreen *top) {
    auto bld = getAnyBuilding(top);
    if (!bld)
        return NULL;

    // no way to select a specific job; just get the first one
    return bld->jobs.size() ? bld->jobs[0] : NULL;
}

df::job *Gui::getSelectedWorkshopJob(color_ostream &out, bool quiet) {
    df::job *job = getAnyWorkshopJob(Core::getTopViewscreen());

    if (!job && !quiet)
        out.printerr("No workshop selected or job cannot be found in the selected workshop.\n");

    return job;
}

df::job *Gui::getAnyJob(df::viewscreen *top) {
    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedJob();

    if (matchFocusString("dwarfmode/Info/JOBS")) {
        auto &cri_job = game->main_interface.info.jobs.cri_job;
        // no way to select specific jobs; just get the first one
        return cri_job.size() ? cri_job[0]->jb : NULL;
    }

    if (auto unit = getAnyUnit(top))
        return unit->job.current_job;

    if (auto job = getAnyWorkshopJob(top))
        return job;

    if (auto cursor = getCursorPos(); cursor.isValid()) {
        for (auto *link = world->jobs.list.next; link; link = link->next) {
            df::job *job = link->item;
            if (!job)
                continue;
            if (job->pos == cursor)
                return job;
        }
    }

    return NULL;
}

df::job *Gui::getSelectedJob(color_ostream &out, bool quiet)
{
    df::job *job = getAnyJob(Core::getTopViewscreen());

    if (!job && !quiet)
        out.printerr("No job can be found from what is selected in the UI.\n");

    return job;
}

df::unit *Gui::getAnyUnit(df::viewscreen *top)
{
    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedUnit();

    if (game->main_interface.view_sheets.open
            && game->main_interface.view_sheets.active_sheet == view_sheet_type::UNIT)
        return df::unit::find(game->main_interface.view_sheets.active_id);
    if (plotinfo->follow_unit != -1)
        return df::unit::find(plotinfo->follow_unit);

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

bool Gui::any_stockpile_hotkey(df::viewscreen* top)
{
    return getAnyStockpile(top) != NULL;
}

df::building_stockpilest* Gui::getAnyStockpile(df::viewscreen* top) {
    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedStockpile();

    if (game->main_interface.bottom_mode_selected == main_bottom_mode_type::STOCKPILE)
        return game->main_interface.stockpile.cur_bld;

    return NULL;
}

df::building_stockpilest* Gui::getSelectedStockpile(color_ostream& out, bool quiet) {
    df::building_stockpilest* stockpile = getAnyStockpile(Core::getTopViewscreen());

    if (!stockpile && !quiet)
        out.printerr("No stockpile is selected in the UI.\n");

    return stockpile;
}

bool Gui::any_civzone_hotkey(df::viewscreen* top) {
    return getAnyCivZone(top) != NULL;
}

df::building_civzonest *Gui::getAnyCivZone(df::viewscreen* top) {
    if (auto dfscreen = dfhack_viewscreen::try_cast(top))
        return dfscreen->getSelectedCivZone();

    if (game->main_interface.bottom_mode_selected == main_bottom_mode_type::ZONE)
        return game->main_interface.civzone.cur_bld;

    return NULL;
}

df::building_civzonest *Gui::getSelectedCivZone(color_ostream &out, bool quiet) {
    df::building_civzonest *civzone = getAnyCivZone(Core::getTopViewscreen());

    if (!civzone && !quiet)
        out.printerr("No zone is selected in the UI");

    return civzone;
}

df::building *Gui::getAnyBuilding(df::viewscreen *top)
{
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

    if (Gui::dwarfmode_hotkey(top) && has_cursor())
    {
        if (!cursor || !plotinfo || !world)
            return nullptr;

        return Maps::getPlantAtTile(cursor->x, cursor->y, cursor->z);
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
static bool update_ucr_alert(df::unit *unit, df::announcement_alert_type alert_type)
{   // Updates combat/sparring/hunting alert
    if (!unit)
        return false;

    df::unit_report_type ucr_type;
    // Choose correct UCR type or abort
    switch (alert_type)
    {
        case announcement_alert_type::COMBAT:
            ucr_type = unit_report_type::Combat;
            break;
        case announcement_alert_type::SPARRING:
            ucr_type = unit_report_type::Sparring;
            break;
        case announcement_alert_type::HUNTING:
            ucr_type = unit_report_type::Hunting;
            break;
        default:
            return false;
    }

    auto &alerts = world->status.announcement_alert;
    // Find existing alert of same type, else create new
    df::announcement_alertst *alert = vector_get(alerts, linear_index(alerts, &df::announcement_alertst::type, alert_type));
    if (!alert)
    {
        alert = new df::announcement_alertst();
        alert->type = alert_type;
        alerts.push_back(alert);
    }

    // Add unit id, UCR category pair to alert if not present
    for (size_t i = 0; i < alert->report_unid.size(); i++)
    {
        if (alert->report_unid[i] == unit->id && alert->report_unit_announcement_category[i] == ucr_type)
            return false;
    }

    alert->report_unid.push_back(unit->id);
    alert->report_unit_announcement_category.push_back(ucr_type);

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

static bool add_proper_report(df::unit *unit, bool is_sparring, df::report *report, bool update_alert = false)
{   // Add report to proper category based on is_sparring and unit current job
    if (is_sparring)
        return Gui::addCombatReport(unit, unit_report_type::Sparring, report, update_alert);
    else if (unit && unit->job.current_job && unit->job.current_job->job_type == job_type::Hunt)
        return Gui::addCombatReport(unit, unit_report_type::Hunting, report, update_alert);
    else
        return Gui::addCombatReport(unit, unit_report_type::Combat, report, update_alert);
}

static bool add_recent_reports(df::unit *unit, df::report *report, bool update_alert = false)
{   // Attempt to add report to all recent categories
    if (!unit || !report)
        return false;

    auto alert_type = announcement_alert_type::NONE;

    // These are ordered by priority for setting alert_type
    if (recent_report(unit, unit_report_type::Sparring))
    {
        Gui::addCombatReport(unit, unit_report_type::Sparring, report, false);
        alert_type = announcement_alert_type::SPARRING;
    }

    if (recent_report(unit, unit_report_type::Hunting))
    {
        Gui::addCombatReport(unit, unit_report_type::Hunting, report, false);
        alert_type = announcement_alert_type::HUNTING;
    }

    if (recent_report(unit, unit_report_type::Combat))
    {
        Gui::addCombatReport(unit, unit_report_type::Combat, report, false);
        alert_type = announcement_alert_type::COMBAT;
    }

    if (update_alert)
        update_ucr_alert(unit, alert_type);

    return alert_type != announcement_alert_type::NONE;
}
// End of utility functions for reports

DFHACK_EXPORT int Gui::makeAnnouncement(df::announcement_type type, df::announcement_flags flags, df::coord pos, std::string message, int color, bool bright)
{
    if (!world->allow_announcements || !is_valid_enum_item(type) || type == df::announcement_type::NONE)
        return -1;
    else if (message.empty())
    {
        Core::printerr("Empty announcement %u\n", type); // DF would print this to errorlog.txt
        return -1;
    }

    if (flags.bits.PAUSE || flags.bits.RECENTER)
        pauseRecenter((flags.bits.RECENTER ? pos : df::coord()), flags.bits.PAUSE); // Does nothing if not dwarf mode

    bool adv_unconscious = false;
    if (auto adv = World::getAdventurer())
        adv_unconscious = adv->counters.unconscious > 0;

    if (flags.bits.DO_MEGA && !adv_unconscious)
        showPopupAnnouncement(message, color, bright);

    writeToGamelog(message);

    auto &reports = world->status.reports;
    auto &alerts = world->status.announcement_alert;

    // Check for repeat report as part of an existing alert
    if (*gamemode == game_mode::DWARF && !flags.bits.ALERT)
    {
        if (linear_index(alerts, &df::announcement_alertst::type, ENUM_ATTR(announcement_type, alert_type, type)) >= 0) // Alert of type exists
        {
            for (size_t i = reports.size(); i-- > 0;)
            {
                auto &cur_report = *reports[i];
                if (cur_report.text == message) // Repeat if text matches
                {
                    cur_report.duration = ANNOUNCE_LINE_DURATION;
                    cur_report.repeat_count++;

                    if (flags.bits.D_DISPLAY)
                        world->status.display_timer = ANNOUNCE_DISPLAY_TIME;

                    return -1;
                }
            }
        }
    }

    auto new_report = new df::report();
    new_report->type = type;
    new_report->text = message;
    new_report->color = color;
    new_report->bright = bright;
    new_report->flags.whole = adv_unconscious ? df::report::T_flags::mask_unconscious : 0x0;
    new_report->pos = pos;
    new_report->id = world->status.next_report_id++;
    new_report->year = *df::global::cur_year;
    new_report->time = *df::global::cur_year_tick;
    reports.push_back(new_report);

    // Handle alerts
    if (*gamemode == game_mode::DWARF)
    {
        if (flags.bits.D_DISPLAY)
        {   // Find existing alert of same type, else create new
            auto alert_type = ENUM_ATTR(announcement_type, alert_type, type);
            df::announcement_alertst *alert = vector_get(alerts, linear_index(alerts, &df::announcement_alertst::type, alert_type));

            if (!alert)
            {
                alert = new df::announcement_alertst();
                alert->type = alert_type;
                alerts.push_back(alert);
            }

            alert->announcement_id.push_back(new_report->id);
        }

        if (flags.bits.ALERT)
        {
            world->status.alert_button_announcement_id.push_back(new_report->id);
            game->main_interface.hover_announcement_alert_button_width = 0;
        }
    }

    // Handle proper announcements
    if ((*gamemode == game_mode::ADVENTURE && flags.bits.A_DISPLAY) || (*gamemode == game_mode::DWARF && flags.bits.D_DISPLAY))
    {
        insert_into_vector(world->status.announcements, &df::report::id, new_report);
        new_report->flags.bits.announcement = true;
        world->status.display_timer = ANNOUNCE_DISPLAY_TIME;
    }

    // Delete excess reports
    while (reports.size() > MAX_REPORTS_SIZE)
    {   // Report destructor
        if (reports[0] != NULL)
        {
            if (reports[0]->flags.bits.announcement)
                erase_from_vector(world->status.announcements, &df::report::id, reports[0]->id);
            delete reports[0];
        }
        reports.erase(reports.begin());
    }

    return world->status.reports.size() - 1;
}

bool Gui::addCombatReport(df::unit *unit, df::unit_report_type slot, df::report *report, bool update_alert)
{
    CHECK_INVALID_ARGUMENT(is_valid_enum_item(slot));

    if (!unit || !report)
        return false;

    unit->reports.log[slot].push_back(report->id);
    unit->reports.last_year[slot] = report->year;
    unit->reports.last_year_tick[slot] = report->time;

    auto alert_type = announcement_alert_type::NONE;
    switch (slot)
    {
        case unit_report_type::Combat:
            world->status.flags.bits.combat = true;
            alert_type = announcement_alert_type::COMBAT;
            break;
        case unit_report_type::Hunting:
            world->status.flags.bits.hunting = true;
            alert_type = announcement_alert_type::HUNTING;
            break;
        case unit_report_type::Sparring:
            world->status.flags.bits.sparring = true;
            alert_type = announcement_alert_type::SPARRING;
            break;
    }

    if (update_alert)
        update_ucr_alert(unit, alert_type);

    return true;
}


bool Gui::addCombatReport(df::unit *unit, df::unit_report_type slot, int report_index, bool update_alert)
{
    return addCombatReport(unit, slot, vector_get(df::global::world->status.reports, report_index), update_alert);
}

bool Gui::addCombatReportAuto(df::unit *unit, df::announcement_flags mode, df::report *report)
{
    if (!unit || !report)
        return false;

    bool ok = false;

    if (mode.bits.UNIT_COMBAT_REPORT)
        ok |= add_proper_report(unit, unit->flags2.bits.sparring, report, true);

    if (mode.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
        ok |= add_recent_reports(unit, report, true);

    return ok;
}

bool Gui::addCombatReportAuto(df::unit *unit, df::announcement_flags mode, int report_index)
{
    return addCombatReportAuto(unit, mode, vector_get(df::global::world->status.reports, report_index));
}

void Gui::showAnnouncement(std::string message, int color, bool bright)
{
    df::announcement_flags mode;
    mode.bits.D_DISPLAY = mode.bits.A_DISPLAY = true;

    makeAnnouncement(df::announcement_type(), mode, df::coord(), message, color, bright);
}

void Gui::showZoomAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color, bool bright)
{
    df::announcement_flags mode;
    mode.bits.D_DISPLAY = mode.bits.A_DISPLAY = true;

    makeAnnouncement(type, mode, pos, message, color, bright);
}

void Gui::showPopupAnnouncement(std::string message, int color, bool bright)
{
    df::popup_message *popup = new df::popup_message();
    popup->text = message;
    popup->color = color; // Doesn't do anything anymore? Popups are always [C:7:0:0] gray text
    popup->bright = bright; // See: https://dwarffortressbugtracker.com/view.php?id=12672

    auto &popups = world->status.popups;
    popups.push_back(popup);

    // Delete excess popups
    while (popups.size() > MAX_REPORTS_SIZE)
    {
        if (popups[0] != NULL)
            delete popups[0];
        popups.erase(popups.begin());
    }

    Gui::MTB_clean(&world->status.mega_text);
    Gui::MTB_parse(&world->status.mega_text, popups[0]->text);
    Gui::MTB_set_width(&world->status.mega_text);

    if (gps->force_full_display_count < 2)
        gps->force_full_display_count = 2;
}

void Gui::showAutoAnnouncement(
    df::announcement_type type, df::coord pos, std::string message, int color, bool bright,
    df::unit *unit_a, df::unit *unit_d)
{
    using df::global::d_init;

    df::announcement_flags flags;
    flags.bits.D_DISPLAY = flags.bits.A_DISPLAY = true;

    if (is_valid_enum_item(type) && type != df::announcement_type::NONE && d_init)
        flags = d_init->announcements.flags[type];

    int id = makeAnnouncement(type, flags, pos, message, color, bright);

    addCombatReportAuto(unit_a, flags, id);
    addCombatReportAuto(unit_d, flags, id);
}

bool Gui::autoDFAnnouncement(df::announcement_infost info, string message)
{   // Based on reverse-engineering of "make_announcement" FUN_1400574e0 (v50.11 win64 Steam)
    if (!world->allow_announcements)
    {
        DEBUG(gui).print("Skipped announcement because world->allow_announcements is false:\n%s\n", message.c_str());
        return false;
    }
    else if (!is_valid_enum_item(info.type) || info.type == df::announcement_type::NONE)
    {
        WARN(gui).print("Invalid announcement type:\n%s\n", message.c_str());
        return false;
    }
    else if (message.empty())
    {
        Core::printerr("Empty announcement %u\n", info.type); // DF would print this to errorlog.txt
        return false;
    }

    auto a_flags = df::global::d_init->announcements.flags[info.type];

    // Check if the announcement will actually be used and written to gamelog
    if (*gamemode == game_mode::ADVENTURE)
    {
        if (!a_flags.bits.A_DISPLAY && !a_flags.bits.DO_MEGA)
        {
            DEBUG(gui).print("Skipped announcement not enabled at all for adventure mode:\n%s\n", message.c_str());
            return false;
        }

        if (info.pos.x >= 0 &&
            info.type != announcement_type::CREATURE_SOUND &&
            info.type != announcement_type::REGULAR_CONVERSATION &&
            info.type != announcement_type::CONFLICT_CONVERSATION &&
            info.type != announcement_type::MECHANISM_SOUND)
        {   // If not sound, make sure we can see pos if we're not involved
            if (auto adv = World::getAdventurer(); info.unit_a != adv && info.unit_d != adv)
            {   // Adventure mode reuses a dwarf mode digging designation bit to determine current visibility
                if (!Maps::isValidTilePos(info.pos) || (Maps::getTileDesignation(info.pos)->whole & 0x10) == 0x0)
                {
                    DEBUG(gui).print("Adventure mode announcement not detected:\n%s\n", message.c_str());
                    return false;
                }
            }
        }
    }
    else // Dwarf mode
    {
        if ((info.unit_a || info.unit_d) && (!info.unit_a || Units::isHidden(info.unit_a)) && (!info.unit_d || Units::isHidden(info.unit_d)))
        {
            DEBUG(gui).print("Dwarf mode announcement not detected:\n%s\n", message.c_str());
            return false;
        }

        if (!a_flags.bits.D_DISPLAY && !a_flags.bits.DO_MEGA) // Apparently ALERT flag is insufficent for announcement to succeed
        {
            if (a_flags.bits.UNIT_COMBAT_REPORT)
            {
                if (!info.unit_a && !info.unit_d)
                {
                    DEBUG(gui).print("Skipped UNIT_COMBAT_REPORT because it has no units:\n%s\n", message.c_str());
                    return false;
                }
            }
            else
            {
                if (!a_flags.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
                {
                    DEBUG(gui).print("Skipped announcement not enabled at all for dwarf mode:\n%s\n", message.c_str());
                    return false;
                }
                else if (!recent_report_any(info.unit_a) && !recent_report_any(info.unit_d))
                {
                    DEBUG(gui).print("Skipped UNIT_COMBAT_REPORT_ALL_ACTIVE because there's no active report:\n%s\n", message.c_str());
                    return false;
                }
            }
        }
    }

    if (a_flags.bits.PAUSE || a_flags.bits.RECENTER)
        pauseRecenter((a_flags.bits.RECENTER ? info.pos : df::coord()), a_flags.bits.PAUSE); // Does nothing if not dwarf mode

    bool adv_unconscious = false;
    if (auto adv = World::getAdventurer())
        adv_unconscious = adv->counters.unconscious > 0;

    if (a_flags.bits.DO_MEGA && !adv_unconscious)
        showPopupAnnouncement(message, info.color, info.bright);

    // Play announcement sound
    //if (!adv_unconscious) // TODO: Find out if sounds will be played while unconscious
    {
        vector<int32_t> valid_sounds;
        for(vector<df::soundst *>::iterator it = world->raws.sound.sound.begin(); it < world->raws.sound.sound.end(); it++)
        {
            auto &cur_sound = **it;
            if (vector_contains(cur_sound.announcement, info.type))
                valid_sounds.push_back(cur_sound.sound);
        }

        // Default to SOUND_ALERT (10) when vector empty if is ALERT, else no sound
        int32_t samp_index = vector_get_random(valid_sounds, (*gamemode == game_mode::DWARF && a_flags.bits.ALERT) ? 10 : -1);

        if (samp_index >= 0)
        {
            DEBUG(gui).print("Playing sound #%d for announcement.\n", samp_index);
            //play_sound(musicsound_info, samp_index, 255, true); // g_src/music_and_sound_g.h // TODO: implement sounds
        }
    }

    writeToGamelog(message);

    auto &reports = world->status.reports;
    auto &alerts = world->status.announcement_alert;

    // Check for repeat report as part of an existing alert
    // TODO: Implement fix for https://dwarffortressbugtracker.com/view.php?id=12670
    if (*gamemode == game_mode::DWARF && !a_flags.bits.ALERT)
    {
        if (linear_index(alerts, &df::announcement_alertst::type, ENUM_ATTR(announcement_type, alert_type, info.type)) >= 0) // Alert of type exists
        {
            for (size_t i = reports.size(); i-- > 0;)
            {
                auto &cur_report = *reports[i];
                if (cur_report.text == message) // Repeat if text matches
                {
                    cur_report.duration = ANNOUNCE_LINE_DURATION;
                    cur_report.repeat_count++;

                    if (a_flags.bits.D_DISPLAY)
                        world->status.display_timer = info.display_timer;

                    DEBUG(gui).print("Announcement succeeded as repeat:\n%s\n", message.c_str());
                    return true;
                }
            }
        }
    }

    auto new_report = new df::report();
    new_report->type = info.type;
    new_report->text = message;
    new_report->color = info.color;
    new_report->bright = info.bright;
    new_report->flags.whole = adv_unconscious ? df::report::T_flags::mask_unconscious : 0x0;
    new_report->zoom_type = info.zoom_type;
    new_report->pos = info.pos;
    new_report->zoom_type2 = info.zoom_type2;
    new_report->pos2 = info.pos2;
    new_report->id = world->status.next_report_id++;
    new_report->year = *df::global::cur_year;
    new_report->time = *df::global::cur_year_tick;
    new_report->activity_id = info.activity_id;
    new_report->activity_event_id = info.activity_event_id;
    new_report->speaker_id = info.speaker_id;
    reports.push_back(new_report);

    // Handle alerts and combat reports
    if (*gamemode == game_mode::DWARF)
    {
        if (a_flags.bits.UNIT_COMBAT_REPORT)
        {
            add_proper_report(info.unit_a, info.flags.bits.SPARRING_EVENT, new_report, true);
            add_proper_report(info.unit_d, info.flags.bits.SPARRING_EVENT, new_report, true);
        }

        if (a_flags.bits.UNIT_COMBAT_REPORT_ALL_ACTIVE)
        {
            add_recent_reports(info.unit_a, new_report, true);
            add_recent_reports(info.unit_d, new_report, true);
        }

        if (a_flags.bits.D_DISPLAY)
        {   // Find existing alert of same type, else create new
            auto alert_type = ENUM_ATTR(announcement_type, alert_type, info.type);
            df::announcement_alertst *alert = vector_get(alerts, linear_index(alerts, &df::announcement_alertst::type, alert_type));

            if (!alert)
            {
                alert = new df::announcement_alertst();
                alert->type = alert_type;
                alerts.push_back(alert);
            }

            alert->announcement_id.push_back(new_report->id);
        }

        if (a_flags.bits.ALERT)
        {
            world->status.alert_button_announcement_id.push_back(new_report->id);
            game->main_interface.hover_announcement_alert_button_width = 0;
        }
    }

    // Handle proper announcements
    if ((*gamemode == game_mode::ADVENTURE && a_flags.bits.A_DISPLAY) || (*gamemode == game_mode::DWARF && a_flags.bits.D_DISPLAY))
    {
        insert_into_vector(world->status.announcements, &df::report::id, new_report);
        new_report->flags.bits.announcement = true;
        world->status.display_timer = info.display_timer;
    }

    // Delete excess reports
    while (reports.size() > MAX_REPORTS_SIZE)
    {   // Report destructor
        if (reports[0] != NULL)
        {
            if (reports[0]->flags.bits.announcement)
                erase_from_vector(world->status.announcements, &df::report::id, reports[0]->id);
            delete reports[0];
        }
        reports.erase(reports.begin());
    }

    if (*gamemode == game_mode::DWARF || // Did dwarf announcement or UCR
        (*gamemode == game_mode::ADVENTURE && a_flags.bits.A_DISPLAY) || // Did adventure announcement
        (a_flags.bits.DO_MEGA && !adv_unconscious)) // Did popup
    {
        DEBUG(gui).print("Announcement succeeded and displayed:\n%s\n", message.c_str());
    }
    else
        DEBUG(gui).print("Announcement added internally and to gamelog.txt but didn't qualify to be displayed anywhere:\n%s\n", message.c_str());

    return true;
}

bool Gui::autoDFAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color,
    bool bright, df::unit *unit_a, df::unit *unit_d, bool is_sparring)
{
    auto info = df::announcement_infost();
    info.type = type;
    info.color = color;
    info.bright = bright;
    info.pos = pos;
    info.display_timer = ANNOUNCE_DISPLAY_TIME;
    info.unit_a = unit_a;
    info.unit_d = unit_d;
    info.flags.bits.SPARRING_EVENT = is_sparring;

    return autoDFAnnouncement(info, message);
}

void Gui::MTB_clean(df::markup_text_boxst *mtb)
{   // Reverse-engineered from "markup_text_boxst::clean" FUN_140056f60 (v50.11 win64 Steam)
    mtb->word.clear();
    mtb->link.clear();
    mtb->current_width = -1;
    mtb->max_y = 0;
    mtb->environment = NULL;
}

static bool insert_markup_text_wordst(vector<df::markup_text_wordst *> &vec, string &str, int32_t link_index, int32_t color)
{
    if (str.empty())
        return false;

    auto ptr = new df::markup_text_wordst();
    ptr->str = str;
    ptr->link_index = link_index;

    ptr->red = gps->uccolor[color][0];
    ptr->green = gps->uccolor[color][1];
    ptr->blue = gps->uccolor[color][2];

    vec.push_back(ptr);
    str.clear();

    return true;
}

void Gui::MTB_parse(df::markup_text_boxst *mtb, string parse_text)
{   // Reverse-engineered from "markup_text_boxst::process_string_to_lines" FUN_1409f70b0 (v50.11 win64 Steam)
    CHECK_NULL_POINTER(mtb);
    auto &word_vec = mtb->word;

    if (parse_text.empty())
    {
        auto ptr = new df::markup_text_wordst();
        ptr->flags.bits.NEW_LINE = true;
        word_vec.push_back(ptr);
        return;
    }

    string str;
    int32_t link_index = -1;
    int32_t color = curses_color::White; // fg = curses_color::White; bg = curses_color::Black; bright = false
    bool use_char, no_split_space;

    size_t i_max = parse_text.size();
    size_t i = 0;
    while (i < i_max)
    {
        char char_token = '\0';
        use_char = true;
        no_split_space = false;

        if (parse_text[i] == ']')
        {
            if (++i >= i_max) // Skip over ']'
                break;

            if (parse_text[i] != ']')
            {
                i--; // Check this char again from top
                continue;
            }
            // else "]]", append ']' to str since use_char == true
        }
        else if (parse_text[i] == '[')
        {
            if (++i >= i_max) // Skip over '['
                break;

            if (parse_text[i] == '.' || parse_text[i] == ':' || parse_text[i] == '?' || parse_text[i] == ' ' || parse_text[i] == '!') // Immediately after '['
                no_split_space = true; // Completely pointless for everything but ' '?
            else if (parse_text[i] != '[') // else "[[", append '[' to str since use_char == true
            {
                use_char = false;

                string token_buffer = grab_token_string_pos(parse_text, i, ':'); // Capture until next ':', ']', or end
                i += token_buffer.size();

                if (token_buffer == "CHAR")
                {
                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff = grab_token_string_pos(parse_text, i, ':');
                    i += buff.size();

                    char_token = (buff.size() > 1 && buff[0] == '~') ? buff[1] : atoi(buff.c_str());
                    no_split_space = true;
                    use_char = true;
                }
                else if (token_buffer == "LPAGE") // Legends page link
                {
                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff_type = grab_token_string_pos(parse_text, i, ':');
                    i += buff_type.size();

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff_id = grab_token_string_pos(parse_text, i, ':');
                    i += buff_id.size();

                    df::markup_text_link_type link_type = markup_text_link_type::NONE;
                    if (buff_type == "HF")
                        link_type = markup_text_link_type::HIST_FIG;
                    else if (buff_type == "SITE")
                        link_type = markup_text_link_type::SITE;
                    else if (buff_type == "ARTIFACT")
                        link_type = markup_text_link_type::ARTIFACT;
                    else if (buff_type == "BOOK")
                        link_type = markup_text_link_type::BOOK;
                    else if (buff_type == "SR")
                        link_type = markup_text_link_type::SUBREGION;
                    else if (buff_type == "FL")
                        link_type = markup_text_link_type::FEATURE_LAYER;
                    else if (buff_type == "ENT")
                        link_type = markup_text_link_type::ENTITY;
                    else if (buff_type == "AB")
                        link_type = markup_text_link_type::ABSTRACT_BUILDING;
                    else if (buff_type == "EPOP")
                        link_type = markup_text_link_type::ENTITY_POPULATION;
                    else if (buff_type == "ART_IMAGE")
                        link_type = markup_text_link_type::ART_IMAGE;
                    else if (buff_type == "ERA")
                        link_type = markup_text_link_type::ERA;
                    else if (buff_type == "HEC")
                        link_type = markup_text_link_type::HEC;

                    int32_t id = atoi(buff_id.c_str());
                    int32_t subid = -1;

                    if (link_type == markup_text_link_type::ABSTRACT_BUILDING || link_type == markup_text_link_type::ART_IMAGE)
                    {
                        if (++i >= i_max) // Skip over ':'
                            break;

                        string buff_subid = grab_token_string_pos(parse_text, i, ':');
                        i += buff_subid.size();

                        subid = atoi(buff_subid.c_str());
                    }

                    if (link_type != markup_text_link_type::NONE)
                    {
                        auto ptr = new df::markup_text_linkst();
                        ptr->type = link_type;
                        ptr->id = id;
                        ptr->subid = subid;
                        mtb->link.push_back(ptr);
                        link_index = mtb->link.size() - 1;
                    }
                }
                else if (token_buffer == "/LPAGE")
                {
                    insert_markup_text_wordst(word_vec, str, link_index, color);
                    link_index = -1;
                }
                else if (token_buffer == "C") // Color
                {
                    insert_markup_text_wordst(word_vec, str, link_index, color);

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff1 = grab_token_string_pos(parse_text, i, ':');
                    i += buff1.size();

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff2 = grab_token_string_pos(parse_text, i, ':');
                    i += buff2.size();

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff3 = grab_token_string_pos(parse_text, i, ':');
                    i += buff3.size();

                    if (buff1 == "VAR") // Color from dipscript var
                    {
                        DEBUG(gui).print("MTB_parse received:\n[C:VAR:%s:%s]\nwhich is for dipscripts and is unimplemented.\nThe dipscript environment itself is: %s\n",
                            buff2.c_str(), buff3.c_str(), mtb->environment ? "Active" : "NULL");
                        //MTB_set_color_on_var(mtb, buff2, buff3);
                    }
                    else
                    {
                        gps->screenf = (df::curses_color)atoi(buff1.c_str());
                        gps->screenb = (df::curses_color)atoi(buff2.c_str());
                        gps->screenbright = (bool)atoi(buff3.c_str());
                        gps->use_old_16_colors = true;
                    }

                    color = (int32_t)gps->screenf + (gps->screenbright ? 8 : 0);
                }
                else if (token_buffer == "KEY")
                {
                    insert_markup_text_wordst(word_vec, str, link_index, color);

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff = grab_token_string_pos(parse_text, i, ':');
                    i += buff.size();

                    auto ptr = new df::markup_text_wordst();
                    ptr->str = df::global::enabler->GetKeyDisplay((df::interface_key)atoi(buff.c_str()));

#define COLOR_LGREEN_INDEX (int32_t)curses_color::Green + 8
                    ptr->red = gps->uccolor[COLOR_LGREEN_INDEX][0];
                    ptr->green = gps->uccolor[COLOR_LGREEN_INDEX][1];
                    ptr->blue = gps->uccolor[COLOR_LGREEN_INDEX][2];
#undef COLOR_LGREEN_INDEX

                    word_vec.push_back(ptr);
                }
                else if (token_buffer == "VAR")
                {
                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff_format = grab_token_string_pos(parse_text, i, ':');
                    i += buff_format.size();

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff_var_type = grab_token_string_pos(parse_text, i, ':');
                    i += buff_var_type.size();

                    if (++i >= i_max) // Skip over ':'
                        break;

                    string buff_var_name = grab_token_string_pos(parse_text, i, ':');
                    i += buff_var_name.size();

                    DEBUG(gui).print("MTB_parse received:\n[VAR:%s:%s:%s]\nwhich is for dipscripts and is unimplemented.\nThe dipscript environment itself is: %s\n",
                        buff_format.c_str(), buff_var_type.c_str(), buff_var_name.c_str(), mtb->environment ? "Active" : "NULL");
                    //MTB_append_variable(mtb, str, buff_format, buff_var_type, buff_var_name);
                }
                else if (token_buffer == "R" || token_buffer == "B" || token_buffer == "P")
                {
                    insert_markup_text_wordst(word_vec, str, link_index, color);

                    auto ptr = new df::markup_text_wordst();
                    if (token_buffer == "R")
                        ptr->flags.bits.NEW_LINE = true;
                    else if (token_buffer == "B")
                        ptr->flags.bits.BLANK_LINE = true;
                    else // "P"
                        ptr->flags.bits.INDENT = true;
                    word_vec.push_back(ptr);
                }
            }
        }

        if (use_char)
        {
            char ch = (char_token == '\0') ? parse_text[i] : char_token;
            if (ch != ' ' || no_split_space)
                str += ch;
            else
                insert_markup_text_wordst(word_vec, str, link_index, color);
        }

        i++;
    }

    insert_markup_text_wordst(word_vec, str, link_index, color);

    for (size_t i = word_vec.size(); i-- > 1;) // Merge punctuation after "[/LPAGE]" left into link word
    {
        auto &cur_entry = *word_vec[i];
        if (cur_entry.link_index != -1 || cur_entry.str.empty()) // Doesn't check (cur_entry.str.size() != 1)
            continue;
        auto &prev_entry = *word_vec[i-1];
        if (prev_entry.link_index == -1 || prev_entry.str.empty())
            continue;

        if (cur_entry.str[0] == '.' || cur_entry.str[0] == ',' || cur_entry.str[0] == '?' || cur_entry.str[0] == '!')
        {
            prev_entry.str += cur_entry.str;
            word_vec.erase(word_vec.begin() + i);
            delete &cur_entry;
        }
    }

    return;
}

void Gui::MTB_set_width(df::markup_text_boxst *mtb, int32_t n_width)
{   // Reverse-engineered from void markup_text_boxst::set_width(markup_text_boxst *this,int n_width), 0x140a16b70 (v50.13 win64 Steam)

    if (mtb->current_width == n_width)
        return;

    mtb->max_y = 0;
    mtb->current_width = n_width;

    int32_t px_val = 0;
    int32_t py_val = 0;

    auto end = mtb->word.end();
    for (auto it = mtb->word.begin(); it != end; it++)
    {
        if (it[0]->flags.bits.NEW_LINE)
        {
            n_width = 0;
            continue;
        }
        if (it[0]->flags.bits.BLANK_LINE)
        {
            n_width = 0;
            px_val = 0;
            py_val++;
            continue;
        }
        if (it[0]->flags.bits.INDENT)
        {
            n_width = mtb->current_width;
            px_val = 4;
            py_val++;
            continue;
        }

        int32_t str_size = (int32_t)(it[0]->str.size());
        if (n_width < str_size)
        {
            n_width = mtb->current_width;
            px_val = 0;
            py_val++;
        }

        if (it + 1 != end && it[1]->str.size() == 1)
        {
            char ch = it[1]->str[0];
            if ((ch == '.' || ch == ',' || ch == '?' || ch == '!') &&
                0 < px_val && n_width < str_size + 2)
            {
                n_width = mtb->current_width;
                px_val = 0;
                py_val++;
            }
        }

        if (str_size == 1) {
            char ch = it[0]->str[0];
            if ((ch == '.' || ch == ',' || ch == '?' || ch == '!') &&
                0 < px_val)
            {
                it[0]->px = px_val - 1;
                it[0]->py = py_val;
                if (mtb->max_y < py_val)
                    mtb->max_y = py_val;
                n_width -= str_size;
                px_val += str_size;
                continue;
            }
        }

        it[0]->px = px_val;
        it[0]->py = py_val;
        if (mtb->max_y < py_val)
            mtb->max_y = py_val;
        px_val += str_size + 1;
        n_width -= str_size + 1;
    }

    return;
}

df::widget * Gui::getWidget(df::widget_container *container, string name) {
    CHECK_NULL_POINTER(container);
    // ensure the compiler catches the change if we ever fix the template parameters
    std::map<void *, void *> & orig_field = container->children_by_name;
    auto children_by_name = reinterpret_cast<std::map<std::string, std::shared_ptr<df::widget>> *>(&orig_field);
    if (children_by_name->contains(name))
        return (*children_by_name)[name].get();
    return NULL;
}

df::widget * Gui::getWidget(df::widget_container *container, size_t idx) {
    CHECK_NULL_POINTER(container);
    if (idx >= container->children.size())
        return NULL;
    return container->children[idx].get();
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

df::viewscreen *Gui::getDFViewscreen(bool skip_dismissed, df::viewscreen *screen) {
    if (!screen)
        screen = Gui::getCurViewscreen(skip_dismissed);
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

static void unfollow() {
    if (!plotinfo)
        return;

    plotinfo->follow_unit = -1;
    plotinfo->follow_item = -1;
}

void Gui::resetDwarfmodeView(bool pause)
{
    using df::global::cursor;

    unfollow();

    if (plotinfo)
        plotinfo->main.mode = ui_sidebar_mode::Default;

    if (selection_rect)
    {
        selection_rect->start_x = -30000;
        selection_rect->start_y = -30000;
        selection_rect->start_z = -30000;
        selection_rect->end_x = -30000;
        selection_rect->end_y = -30000;
        selection_rect->end_z = -30000;
    }
    // NOTE: There's an unidentified global coord after selection_rect that is reset to -30000 here.
    // This coord goes into game->main_interface.keyboard_last_track_s if the x value is not -30000. Probably okay to ignore?

    if (cursor)
        cursor->x = cursor->y = cursor->z = -30000;

    if (pause && df::global::pause_state)
        *df::global::pause_state = true;
}

bool Gui::revealInDwarfmodeMap(int32_t x, int32_t y, int32_t z, bool center, bool highlight)
{   // Reverse-engineered from DF announcement and scrolling code
    using df::global::window_x;
    using df::global::window_y;
    using df::global::window_z;

    if (!window_x || !window_y || !window_z || !world || !game)
        return false;

    unfollow();

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

    *window_x = new_win_x;
    *window_y = new_win_y;
    *window_z = clip_range(new_win_z, 0, (world->map.z_count - 1));
    game->minimap.update = true;
    game->minimap.mustmake = true;

    if (highlight) {
        game->main_interface.recenter_indicator_m.x = x;
        game->main_interface.recenter_indicator_m.y = y;
        game->main_interface.recenter_indicator_m.z = z;
    }

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
    return (x >= 0) ? false : true;
}

bool Gui::setDesignationCoords (const int32_t x, const int32_t y, const int32_t z)
{
    selection_rect->start_x = x;
    selection_rect->start_y = y;
    selection_rect->start_z = z;
    return true;
}

// returns the map coordinates that the mouse cursor is over
df::coord Gui::getMousePos(bool allow_out_of_bounds)
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
    if (!allow_out_of_bounds && !Maps::isValidTilePos(pos.x, pos.y, pos.z))
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
