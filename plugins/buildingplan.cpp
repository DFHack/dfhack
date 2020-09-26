#include "buildingplan-lib.h"
    
#include "df/entity_position.h"
#include "df/interface_key.h"
#include "df/ui_build_selector.h"
#include "df/viewscreen_dwarfmodest.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "LuaTools.h"
#include "PluginManager.h"

#include "uicommon.h"
#include "listcolumn.h"

DFHACK_PLUGIN("buildingplan");
#define PLUGIN_VERSION 0.15
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(world);

#define MAX_MASK 10
#define MAX_MATERIAL 21

using namespace DFHack;
using namespace df::enums;

bool show_help = false;

class ViewscreenChooseMaterial : public dfhack_viewscreen
{
public:
    ViewscreenChooseMaterial(ItemFilter *filter);

    void feed(set<df::interface_key> *input);

    void render();

    std::string getFocusString() { return "buildingplan_choosemat"; }

private:
    ListColumn<df::dfhack_material_category> masks_column;
    ListColumn<MaterialInfo> materials_column;
    int selected_column;
    ItemFilter *filter;

    df::building_type btype;

    void addMaskEntry(df::dfhack_material_category &mask, const std::string &text)
    {
        auto entry = ListEntry<df::dfhack_material_category>(pad_string(text, MAX_MASK, false), mask);
        if (filter->matches(mask))
            entry.selected = true;

        masks_column.add(entry);
    }

    void populateMasks()
    {
        masks_column.clear();
        df::dfhack_material_category mask;

        mask.whole = 0;
        mask.bits.stone = true;
        addMaskEntry(mask, "Stone");

        mask.whole = 0;
        mask.bits.wood = true;
        addMaskEntry(mask, "Wood");

        mask.whole = 0;
        mask.bits.metal = true;
        addMaskEntry(mask, "Metal");

        mask.whole = 0;
        mask.bits.soap = true;
        addMaskEntry(mask, "Soap");

        masks_column.filterDisplay();
    }

    void populateMaterials()
    {
        materials_column.clear();
        df::dfhack_material_category selected_category;
        std::vector<df::dfhack_material_category> selected_masks = masks_column.getSelectedElems();
        if (selected_masks.size() == 1)
            selected_category = selected_masks[0];
        else if (selected_masks.size() > 1)
            return;

        df::world_raws &raws = world->raws;
        for (int i = 1; i < DFHack::MaterialInfo::NUM_BUILTIN; i++)
        {
            auto obj = raws.mat_table.builtin[i];
            if (obj)
            {
                MaterialInfo material;
                material.decode(i, -1);
                addMaterialEntry(selected_category, material, material.toString());
            }
        }

        for (size_t i = 0; i < raws.inorganics.size(); i++)
        {
            df::inorganic_raw *p = raws.inorganics[i];
            MaterialInfo material;
            material.decode(0, i);
            addMaterialEntry(selected_category, material, material.toString());
        }

        decltype(selected_category) wood_flag;
        wood_flag.bits.wood = true;
        if (!selected_category.whole || selected_category.bits.wood)
        {
            for (size_t i = 0; i < raws.plants.all.size(); i++)
            {
                df::plant_raw *p = raws.plants.all[i];
                for (size_t j = 0; p->material.size() > 1 && j < p->material.size(); j++)
                {
                    auto t = p->material[j];
                    if (p->material[j]->id != "WOOD")
                        continue;

                    MaterialInfo material;
                    material.decode(DFHack::MaterialInfo::PLANT_BASE+j, i);
                    auto name = material.toString();
                    ListEntry<MaterialInfo> entry(pad_string(name, MAX_MATERIAL, false), material);
                    if (filter->matches(material))
                        entry.selected = true;

                    materials_column.add(entry);
                }
            }
        }
        materials_column.sort();
    }

    void addMaterialEntry(df::dfhack_material_category &selected_category,
                          MaterialInfo &material, std::string name)
    {
        if (!selected_category.whole || material.matches(selected_category))
        {
            ListEntry<MaterialInfo> entry(pad_string(name, MAX_MATERIAL, false), material);
            if (filter->matches(material))
                entry.selected = true;

            materials_column.add(entry);
        }
    }

    void validateColumn()
    {
        set_to_limit(selected_column, 1);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        masks_column.resize();
        materials_column.resize();
    }
};

DFHack::MaterialInfo &material_info_identity_fn(DFHack::MaterialInfo &m) { return m; }

ViewscreenChooseMaterial::ViewscreenChooseMaterial(ItemFilter *filter)
{
    selected_column = 0;
    masks_column.setTitle("Type");
    masks_column.multiselect = true;
    masks_column.allow_search = false;
    masks_column.left_margin = 2;
    materials_column.left_margin = MAX_MASK + 3;
    materials_column.setTitle("Material");
    materials_column.multiselect = true;
    this->filter = filter;

    masks_column.changeHighlight(0);

    populateMasks();
    populateMaterials();

    masks_column.selectDefaultEntry();
    materials_column.selectDefaultEntry();
    materials_column.changeHighlight(0);
}

void ViewscreenChooseMaterial::feed(set<df::interface_key> *input)
{
    bool key_processed = false;
    switch (selected_column)
    {
    case 0:
        key_processed = masks_column.feed(input);
        if (input->count(interface_key::SELECT))
            populateMaterials(); // Redo materials lists based on category selection
        break;
    case 1:
        key_processed = materials_column.feed(input);
        break;
    }

    if (key_processed)
        return;

    if (input->count(interface_key::LEAVESCREEN))
    {
        input->clear();
        Screen::dismiss(this);
        return;
    }
    if (input->count(interface_key::CUSTOM_SHIFT_C))
    {
        filter->clear();
        masks_column.clearSelection();
        materials_column.clearSelection();
        populateMaterials();
    }
    else if (input->count(interface_key::SEC_SELECT))
    {
        // Convert list selections to material filters
        filter->mat_mask.whole = 0;
        filter->materials.clear();

        // Category masks
        auto masks = masks_column.getSelectedElems();
        for (auto it = masks.begin(); it != masks.end(); ++it)
            filter->mat_mask.whole |= it->whole;

        // Specific materials
        auto materials = materials_column.getSelectedElems();
        transform_(materials, filter->materials, material_info_identity_fn);

        Screen::dismiss(this);
    }
    else if (input->count(interface_key::CURSOR_LEFT))
    {
        --selected_column;
        validateColumn();
    }
    else if (input->count(interface_key::CURSOR_RIGHT))
    {
        selected_column++;
        validateColumn();
    }
    else if (enabler->tracking_on && enabler->mouse_lbut)
    {
        if (masks_column.setHighlightByMouse())
            selected_column = 0;
        else if (materials_column.setHighlightByMouse())
            selected_column = 1;

        enabler->mouse_lbut = enabler->mouse_rbut = 0;
    }
}

void ViewscreenChooseMaterial::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    Screen::clear();
    Screen::drawBorder("  Building Material  ");

    masks_column.display(selected_column == 0);
    materials_column.display(selected_column == 1);

    int32_t y = gps->dimy - 3;
    int32_t x = 2;
    OutputHotkeyString(x, y, "Toggle", interface_key::SELECT);
    x += 3;
    OutputHotkeyString(x, y, "Save", interface_key::SEC_SELECT);
    x += 3;
    OutputHotkeyString(x, y, "Clear", interface_key::CUSTOM_SHIFT_C);
    x += 3;
    OutputHotkeyString(x, y, "Cancel", interface_key::LEAVESCREEN);
}

//START Viewscreen Hook
static bool is_planmode_enabled(df::building_type type)
{
    if (planmode_enabled.find(type) == planmode_enabled.end())
    {
        return false;
    }

    return planmode_enabled[type];
}

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
            df::global::ui_build_selector &&
            df::global::ui_build_selector->stage < 2 &&
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
                    Gui::refreshSidebar();
                    planner.in_dummmy_screen = false;
                }
                return true;
            }
            else if (input->count(interface_key::CUSTOM_P) ||
                     input->count(interface_key::CUSTOM_F) ||
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
                        Gui::refreshSidebar();
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
                    Screen::show(dts::make_unique<ViewscreenChooseMaterial>(planner.getDefaultItemFilterForType(type)), plugin_self);
                }
                else if (input->count(interface_key::CUSTOM_Q))
                {
                    planner.adjustMinQuality(type, -1);
                }
                else if (input->count(interface_key::CUSTOM_W))
                {
                    planner.adjustMinQuality(type, 1);
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_Q))
                {
                    planner.adjustMaxQuality(type, -1);
                }
                else if (input->count(interface_key::CUSTOM_SHIFT_W))
                {
                    planner.adjustMaxQuality(type, 1);
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
            if (Gui::inRenameBuilding())
                return false;
            auto np = getNoblePositionOfSelectedBuildingOwner();
            df::interface_key last_token = get_string_key(input);
            if (last_token >= interface_key::STRING_A048 && last_token <= interface_key::STRING_A058)
            {
                size_t selection = last_token - interface_key::STRING_A048;
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

                    OutputHotkeyString(x, y, "Min Quality: ", "qw");
                    OutputString(COLOR_BROWN, x, y, filter->getMinQuality(), true, left_margin);

                    OutputHotkeyString(x, y, "Max Quality: ", "QW");
                    OutputString(COLOR_BROWN, x, y, filter->getMaxQuality(), true, left_margin);

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
            OutputString(COLOR_BROWN, x, y, "Min Quality: ", false, left_margin);
            OutputString(COLOR_BLUE, x, y, filter->getMinQuality(), true, left_margin);
            OutputString(COLOR_BROWN, x, y, "Max Quality: ", false, left_margin);
            OutputString(COLOR_BLUE, x, y, filter->getMaxQuality(), true, left_margin);

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
            for (size_t i = 0; i < np.size() && i < 9; i++)
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

#define DAY_TICKS 1200
DFhackCExport command_result plugin_onupdate(color_ostream &)
{
    if (Maps::IsValid() && !World::ReadPauseState() && world->frame_counter % (DAY_TICKS/2) == 0)
    {
        planner.doCycle();
        roomMonitor.doCycle();
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &)
{
    return CR_OK;
}

// Lua API section

static bool isPlannableBuilding(df::building_type type) {
    return planner.isPlanableBuilding(type);
}

static void addPlannedBuilding(df::building *bld) {
    planner.addPlannedBuilding(bld);
}

static void doCycle() {
    planner.doCycle();
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(isPlannableBuilding),
    DFHACK_LUA_FUNCTION(addPlannedBuilding),
    DFHACK_LUA_FUNCTION(doCycle),
    DFHACK_LUA_END
};
