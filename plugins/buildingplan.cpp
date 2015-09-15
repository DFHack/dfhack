#include "buildingplan-lib.h"

DFHACK_PLUGIN("buildingplan");
#define PLUGIN_VERSION 0.14
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(world);

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


static bool is_planmode_enabled(df::building_type type)
{
    if (planmode_enabled.find(type) == planmode_enabled.end())
    {
        return false;
    }

    return planmode_enabled[type];
}

#define DAY_TICKS 1200
DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    static decltype(world->frame_counter) last_frame_count = 0;
    if ((world->frame_counter - last_frame_count) >= DAY_TICKS/2)
    {
        last_frame_count = world->frame_counter;
        planner.doCycle();
        roomMonitor.doCycle();
    }

    return CR_OK;
}

//START Viewscreen Hook
struct buildingplan_hook : public df::viewscreen_dwarfmodest
{
    //START UI Methods
    typedef df::viewscreen_dwarfmodest interpose_base;

    void send_key(const df::interface_key &key)
    {
        set< df::interface_key > keys;
        keys.insert(key);
        this->feed(&keys);
    }

    bool isInPlannedBuildingQueryMode()
    {
        return (ui->main.mode == df::ui_sidebar_mode::QueryBuilding ||
            ui->main.mode == df::ui_sidebar_mode::BuildingItems) &&
            planner.getSelectedPlannedBuilding();
    }

    bool isInPlannedBuildingPlacementMode()
    {
        return ui->main.mode == ui_sidebar_mode::Build &&
            ui_build_selector &&
            ui_build_selector->stage < 2 &&
            planner.isPlanableBuilding(ui_build_selector->building_type);
    }

    std::vector<Units::NoblePosition> getNoblePositionOfSelectedBuildingOwner()
    {
        std::vector<Units::NoblePosition> np;
        if (ui->main.mode != df::ui_sidebar_mode::QueryBuilding ||
            !world->selected_building ||
            !world->selected_building->owner)
        {
            return np;
        }

        switch (world->selected_building->getType())
        {
        case building_type::Bed:
        case building_type::Chair:
        case building_type::Table:
            break;
        default:
            return np;
        }

        return getUniqueNoblePositions(world->selected_building->owner);
    }

    bool isInNobleRoomQueryMode()
    {
        if (getNoblePositionOfSelectedBuildingOwner().size() > 0)
            return canReserveRoom(world->selected_building);
        else
            return false;
    }

    bool handleInput(set<df::interface_key> *input)
    {
        if (isInPlannedBuildingPlacementMode())
        {
            auto type = ui_build_selector->building_type;
            if (input->count(interface_key::CUSTOM_SHIFT_P))
            {
                planmode_enabled[type] = !planmode_enabled[type];
                if (!planmode_enabled[type])
                {
                    send_key(interface_key::CURSOR_DOWN_Z);
                    send_key(interface_key::CURSOR_UP_Z);
                    planner.in_dummmy_screen = false;
                }
                return true;
            }
            else if (input->count(interface_key::CUSTOM_P) ||
                     input->count(interface_key::CUSTOM_F) ||
                     input->count(interface_key::CUSTOM_Q) ||
                     input->count(interface_key::CUSTOM_D) ||
                     input->count(interface_key::CUSTOM_N))
            {
                show_help = true;
            }

            if (is_planmode_enabled(type))
            {
                if (planner.inQuickFortMode() && planner.in_dummmy_screen)
                {
                    if (input->count(interface_key::SELECT) || input->count(interface_key::SEC_SELECT)
                         || input->count(interface_key::LEAVESCREEN))
                    {
                        planner.in_dummmy_screen = false;
                        send_key(interface_key::LEAVESCREEN);
                    }

                    return true;
                }

                if (input->count(interface_key::SELECT))
                {
                    if (ui_build_selector->errors.size() == 0 && planner.allocatePlannedBuilding(type))
                    {
                        send_key(interface_key::CURSOR_DOWN_Z);
                        send_key(interface_key::CURSOR_UP_Z);
                        if (planner.inQuickFortMode())
                        {
                            planner.in_dummmy_screen = true;
                        }
                    }

                    return true;
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_F))
                {
                    if (!planner.inQuickFortMode())
                    {
                        planner.enableQuickfortMode();
                    }
                    else
                    {
                        planner.disableQuickfortMode();
                    }
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_M))
                {
                    Screen::show(new ViewscreenChooseMaterial(planner.getDefaultItemFilterForType(type)));
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_Q))
                {
                    planner.cycleDefaultQuality(type);
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_D))
                {
                    planner.getDefaultItemFilterForType(type)->decorated_only =
                        !planner.getDefaultItemFilterForType(type)->decorated_only;
                }
            }
        }
        else if (isInPlannedBuildingQueryMode())
        {
            if (input->count(interface_key::SUSPENDBUILDING))
            {
                return true; // Don't unsuspend planned buildings
            }
            else if (input->count(interface_key::DESTROYBUILDING))
            {
                planner.removeSelectedPlannedBuilding(); // Remove persistent data
            }

        }
        else if (isInNobleRoomQueryMode())
        {
            auto np = getNoblePositionOfSelectedBuildingOwner();
            df::interface_key last_token = get_string_key(input);
            if (last_token >= interface_key::STRING_A048 && last_token <= interface_key::STRING_A058)
            {
                int selection = last_token - interface_key::STRING_A048;
                if (np.size() < selection)
                    return false;
                roomMonitor.toggleRoomForPosition(world->selected_building->id, np.at(selection-1).position->code);
                return true;
            }
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handleInput(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        bool plannable = isInPlannedBuildingPlacementMode();
        if (plannable && is_planmode_enabled(ui_build_selector->building_type))
        {
            if (ui_build_selector->stage < 1)
            {
                // No materials but turn on cursor
                ui_build_selector->stage = 1;
            }

            for (auto iter = ui_build_selector->errors.begin(); iter != ui_build_selector->errors.end();)
            {
                //FIXME Hide bags
                if (((*iter)->find("Needs") != string::npos && **iter != "Needs adjacent wall")  ||
                    (*iter)->find("No access") != string::npos)
                {
                    iter = ui_build_selector->errors.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        INTERPOSE_NEXT(render)();

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        auto type = ui_build_selector->building_type;
        if (plannable)
        {
            if (planner.inQuickFortMode() && planner.in_dummmy_screen)
            {
                Screen::Pen pen(' ',COLOR_BLACK);
                int y = dims.y1 + 1;
                Screen::fillRect(pen, x, y, dims.menu_x2, y + 20);

                ++y;

                OutputString(COLOR_BROWN, x, y, "Quickfort Placeholder", true, left_margin);
                OutputString(COLOR_WHITE, x, y, "Enter, Shift-Enter or Esc", true, left_margin);
            }
            else
            {
                int y = 23;

                if (show_help)
                {
                    OutputString(COLOR_BROWN, x, y, "Note: ");
                    OutputString(COLOR_WHITE, x, y, "Use Shift-Keys here", true, left_margin);
                }
                OutputToggleString(x, y, "Planning Mode", "P", is_planmode_enabled(type), true, left_margin);

                if (is_planmode_enabled(type))
                {
                    OutputToggleString(x, y, "Quickfort Mode", "F", planner.inQuickFortMode(), true, left_margin);

                    auto filter = planner.getDefaultItemFilterForType(type);

                    OutputHotkeyString(x, y, "Min Quality: ", "Q");
                    OutputString(COLOR_BROWN, x, y, filter->getMinQuality(), true, left_margin);

                    OutputToggleString(x, y, "Decorated Only: ", "D", filter->decorated_only, true, left_margin);

                    OutputHotkeyString(x, y, "Material Filter:", "M", true, left_margin);
                    auto filter_descriptions = filter->getMaterialFilterAsVector();
                    for (auto it = filter_descriptions.begin(); it != filter_descriptions.end(); ++it)
                        OutputString(COLOR_BROWN, x, y, "   *" + *it, true, left_margin);
                }
                else
                {
                    planner.in_dummmy_screen = false;
                }
            }
        }
        else if (isInPlannedBuildingQueryMode())
        {
            planner.in_dummmy_screen = false;

            // Hide suspend toggle option
            int y = 20;
            Screen::Pen pen(' ', COLOR_BLACK);
            Screen::fillRect(pen, x, y, dims.menu_x2, y);

            auto filter = planner.getSelectedPlannedBuilding()->getFilter();
            y = 24;
            OutputString(COLOR_BROWN, x, y, "Planned Building Filter:", true, left_margin);
            OutputString(COLOR_BLUE, x, y, filter->getMinQuality(), true, left_margin);

            if (filter->decorated_only)
                OutputString(COLOR_BLUE, x, y, "Decorated Only", true, left_margin);

            OutputString(COLOR_BROWN, x, y, "Materials:", true, left_margin);
            auto filters = filter->getMaterialFilterAsVector();
            for (auto it = filters.begin(); it != filters.end(); ++it)
                OutputString(COLOR_BLUE, x, y, "*" + *it, true, left_margin);
        }
        else if (isInNobleRoomQueryMode())
        {
            auto np = getNoblePositionOfSelectedBuildingOwner();
            int y = 24;
            OutputString(COLOR_BROWN, x, y, "DFHack", true, left_margin);
            OutputString(COLOR_WHITE, x, y, "Auto-allocate to:", true, left_margin);
            for (int i = 0; i < np.size() && i < 9; i++)
            {
                bool enabled = (roomMonitor.getReservedNobleCode(world->selected_building->id)
                    == np[i].position->code);
                OutputToggleString(x, y, np[i].position->name[0].c_str(),
                    int_to_string(i+1).c_str(), enabled, true, left_margin);
            }
        }
        else
        {
            planner.in_dummmy_screen = false;
            show_help = false;
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(buildingplan_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(buildingplan_hook, render);


static command_result buildingplan_cmd(color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
    {
        if (parameters.size() == 1 && toLower(parameters[0])[0] == 'v')
        {
            out << "Building Plan" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if (parameters.size() == 2 && toLower(parameters[0]) == "debug")
        {
            show_debugging = (toLower(parameters[1]) == "on");
            out << "Debugging " << ((show_debugging) ? "enabled" : "disabled") << endl;
        }
    }

    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        planner.reset(out);

        if (!INTERPOSE_HOOK(buildingplan_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(buildingplan_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
        "buildingplan", "Place furniture before it's built",
        buildingplan_cmd, false, "Run 'buildingplan debug [on|off]' to toggle debugging, or 'buildingplan version' to query the plugin version."));
    planner.initialize();

    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        planner.reset(out);
        roomMonitor.reset(out);
        break;
    default:
        break;
    }

    return CR_OK;
}
