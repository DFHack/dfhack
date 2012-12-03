// Auto Material Select

#include <map>
#include <string>
#include <vector>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>


// DF data structure definition headers
#include "DataDefs.h"
#include "MiscUtils.h"
#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/construction_type.h"
#include "df/item.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/world.h"
#include "df/building_constructionst.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Items.h"
#include "modules/Constructions.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"

#include "TileTypes.h"
#include "df/job_item.h"

using std::map;
using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;
using df::global::gps;
using df::global::ui;
using df::global::ui_build_selector;

DFHACK_PLUGIN("automaterial");

struct MaterialDescriptor
{
    df::item_type item_type;
    int16_t item_subtype;
    int16_t type;
    int32_t index;
    bool valid;

    bool matches(const MaterialDescriptor &a) const
    {
        return a.valid && valid && 
            a.type == type && 
            a.index == index &&
            a.item_type == item_type &&
            a.item_subtype == item_subtype;
    }
};

static map<int16_t, MaterialDescriptor> last_used_material;
static map<int16_t, MaterialDescriptor> last_moved_material;
static map< int16_t, vector<MaterialDescriptor> > preferred_materials;
static map< int16_t, df::interface_key > hotkeys;
static bool last_used_moved = false;
static bool auto_choose_materials = true;
static bool auto_choose_attempted = true;
static bool revert_to_last_used_type = false;

struct point
{
    int32_t x, y, z;
};

static enum t_box_select_mode {SELECT_FIRST, SELECT_SECOND, SELECT_MATERIALS, AUTOSELECT_MATERIALS} box_select_mode = SELECT_FIRST;
static point box_first, box_second;
static bool box_select_enabled = false;
static bool show_box_selection = true;
static bool hollow_selection = false;
#define SELECTION_IGNORE_TICKS 10
static int ignore_selection = SELECTION_IGNORE_TICKS;
static deque<df::item*> box_select_materials;
static vector<df::coord> building_sites;

static command_result automaterial_cmd(color_ostream &out, vector <string> & parameters)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


void OutputString(int8_t color, int &x, int &y, const std::string &text, bool newline = false, int left_margin = 0)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    if (newline)
    {
        ++y;
        x = left_margin;
    }
    else
        x += text.length();
}

void OutputHotkeyString(int &x, int &y, const char *text, const char *hotkey, bool newline = false, int left_margin = 0, int8_t color = COLOR_WHITE)
{
    OutputString(10, x, y, hotkey);
    string display(": ");
    display.append(text);
    OutputString(color, x, y, display, newline, left_margin);
}

static string int_to_string(int i)
{
    return static_cast<ostringstream*>( &(ostringstream() << i))->str();
}

static inline bool in_material_choice_stage()
{
    return Gui::build_selector_hotkey(Core::getTopViewscreen()) && 
        ui_build_selector->building_type == df::building_type::Construction &&
        ui->main.mode == ui_sidebar_mode::Build && 
        ui_build_selector->stage == 2;
}

static inline bool in_placement_stage()
{
    return Gui::dwarfmode_hotkey(Core::getTopViewscreen()) && 
        ui->main.mode == ui_sidebar_mode::Build &&
        ui_build_selector &&
        ui_build_selector->building_type == df::building_type::Construction &&
        ui_build_selector->stage == 1;
}

static inline bool in_type_choice_stage()
{
    return Gui::dwarfmode_hotkey(Core::getTopViewscreen()) && 
        ui->main.mode == ui_sidebar_mode::Build &&
        ui_build_selector &&
        ui_build_selector->building_type < 0;
}

static inline vector<MaterialDescriptor> &get_curr_constr_prefs()
{
    if (preferred_materials.find(ui_build_selector->building_subtype) == preferred_materials.end())
        preferred_materials[ui_build_selector->building_subtype] = vector<MaterialDescriptor>();

    return preferred_materials[ui_build_selector->building_subtype];
}

static inline MaterialDescriptor &get_last_used_material()
{
    if (last_used_material.find(ui_build_selector->building_subtype) == last_used_material.end())
        last_used_material[ui_build_selector->building_subtype] = MaterialDescriptor();

    return last_used_material[ui_build_selector->building_subtype];
}

static void set_last_used_material(const MaterialDescriptor &matetial)
{
    last_used_material[ui_build_selector->building_subtype] = matetial;
}

static MaterialDescriptor &get_last_moved_material()
{
    if (last_moved_material.find(ui_build_selector->building_subtype) == last_moved_material.end())
        last_moved_material[ui_build_selector->building_subtype] = MaterialDescriptor();

    return last_moved_material[ui_build_selector->building_subtype];
}

static void set_last_moved_material(const MaterialDescriptor &matetial)
{
    last_moved_material[ui_build_selector->building_subtype] = matetial;
}

static MaterialDescriptor get_material_in_list(size_t i)
{
    MaterialDescriptor result;
    result.valid = false;

    if (VIRTUAL_CAST_VAR(gen, df::build_req_choice_genst, ui_build_selector->choices[i]))
    {
        result.item_type = gen->item_type;
        result.item_subtype = gen->item_subtype;
        result.type = gen->mat_type;
        result.index = gen->mat_index;
        result.valid = true;
    }
    else if (VIRTUAL_CAST_VAR(spec, df::build_req_choice_specst, ui_build_selector->choices[i]))
    {
        result.item_type = spec->candidate->getType();
        result.item_subtype = spec->candidate->getSubtype();
        result.type = spec->candidate->getActualMaterial();
        result.index = spec->candidate->getActualMaterialIndex();
        result.valid = true;
    }

    return result;
}

static bool is_material_in_autoselect(size_t &i, MaterialDescriptor &material)
{
    for (i = 0; i < get_curr_constr_prefs().size(); i++)
    {
        if (get_curr_constr_prefs()[i].matches(material))
            return true;
    }

    return false;
}

static bool is_material_in_list(size_t &i, MaterialDescriptor &material)
{
    const size_t size = ui_build_selector->choices.size(); //Just because material list could be very big
    for (i = 0; i < size; i++)
    {
        if (get_material_in_list(i).matches(material))
            return true;
    }

    return false;
}

static bool move_material_to_top(MaterialDescriptor &material)
{
    size_t i;
    if (is_material_in_list(i, material))
    {
        auto sel_item = ui_build_selector->choices[i];
        ui_build_selector->choices.erase(ui_build_selector->choices.begin() + i);
        ui_build_selector->choices.insert(ui_build_selector->choices.begin(), sel_item);

        ui_build_selector->sel_index = 0;
        set_last_moved_material(material);
        return true;
    }

    set_last_moved_material(MaterialDescriptor());
    return false;
}

static bool check_autoselect(MaterialDescriptor &material, bool toggle)
{
    size_t idx;
    if (is_material_in_autoselect(idx, material))
    {
        if (toggle) 
            vector_erase_at(get_curr_constr_prefs(), idx);

        return true;
    }
    else
    {
        if (toggle)
            get_curr_constr_prefs().push_back(material);

        return false;
    }
}

static void cancel_box_selection()
{
    if (box_select_mode == SELECT_FIRST)
        return;

    box_select_mode = SELECT_FIRST;
    box_select_materials.clear();
    if (!show_box_selection)
        Gui::setDesignationCoords(-1, -1, -1);
}

static bool is_valid_building_site(df::coord &pos)
{
    auto ttype = Maps::getTileType(pos);
    if (!ttype || 
        *ttype == tiletype::Tree ||
        tileMaterial(*ttype) == tiletype_material::CONSTRUCTION ||
        tileMaterial(*ttype) == tiletype_material::AIR ||
        tileMaterial(*ttype) == tiletype_material::CAMPFIRE ||
        tileMaterial(*ttype) == tiletype_material::FIRE ||
        tileMaterial(*ttype) == tiletype_material::MAGMA ||
        tileMaterial(*ttype) == tiletype_material::DRIFTWOOD ||
        tileMaterial(*ttype) == tiletype_material::POOL ||
        tileMaterial(*ttype) == tiletype_material::BROOK ||
        tileMaterial(*ttype) == tiletype_material::RIVER
        )
        return false;

    auto current = Buildings::findAtTile(pos);
    if (current)
        return false;

    df::coord2d size(1,1);
    return Buildings::checkFreeTiles(pos, size, NULL, false, false);
}

static bool designate_new_construction(df::coord &pos, df::construction_type &type, df::item *item)
{
    auto newinst = Buildings::allocInstance(pos, building_type::Construction, type);
    if (!newinst)
        return false;

    vector<df::item*> items;
    items.push_back(item);

    if (!Buildings::constructWithItems(newinst, items))
    {
        delete newinst;
        return false;
    }

    return true;
}


struct jobutils_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    void send_key(const df::interface_key &key)
    {
        set< df::interface_key > keys;
        keys.insert(key);
        this->feed(&keys);
    }

    bool select_material_at_index(size_t i)
    {
        ui_build_selector->sel_index = i;
        std::set< df::interface_key > keys;
        keys.insert(df::interface_key::SELECT_ALL);
        this->feed(&keys);
        return !in_material_choice_stage();
    }

    bool choose_materials()
    {
        if (!auto_choose_materials || get_curr_constr_prefs().size() == 0)
            return false;

        size_t size = ui_build_selector->choices.size();
        for (size_t i = 0; i < size; i++)
        {
            MaterialDescriptor material = get_material_in_list(i);
            size_t j;
            if (is_material_in_autoselect(j, material))
            {
                return select_material_at_index(i);
            }
        }

        return false;
    }

    bool populate_box_materials(vector<MaterialDescriptor> &gen_materials)
    {
        bool result = false;

        if (gen_materials.size() == 0)
            return result;

        if (ui_build_selector->is_grouped)
            send_key(interface_key::BUILDING_EXPAND_CONTRACT);

        size_t size = ui_build_selector->choices.size();
        vector<MaterialDescriptor>::iterator gen_material;
        for (size_t i = 0; i < size; i++)
        {
            if (VIRTUAL_CAST_VAR(spec, df::build_req_choice_specst, ui_build_selector->choices[i]))
            {
                for (gen_material = gen_materials.begin(); gen_material != gen_materials.end(); gen_material++)
                {
                    if (gen_material->item_type == spec->candidate->getType() &&
                        gen_material->item_subtype == spec->candidate->getSubtype() &&
                        gen_material->type == spec->candidate->getActualMaterial() &&
                        gen_material->index == spec->candidate->getActualMaterialIndex())
                    {
                        box_select_materials.push_back(spec->candidate);
                        result = true;
                        break;
                    }
                }
            }
        }
        send_key(interface_key::BUILDING_EXPAND_CONTRACT);

        return result;
    }

    void draw_box_selection()
    {
        if (!box_select_enabled)
            return;

        df::coord vport = Gui::getViewportPos();

        //Even if selection drawing is disabled, paint a green cursor as we can place box selection anywhere

        if (box_select_mode == SELECT_FIRST || (!show_box_selection && box_select_mode == SELECT_SECOND))
        {
            int32_t x, y, z;

            if (!Gui::getCursorCoords(x, y, z))
                return;

            x = x - vport.x + 1;
            y = y - vport.y + 1;
            OutputString(COLOR_GREEN, x, y, "X");
        }
        else if (show_box_selection && box_select_mode == SELECT_SECOND)
        {
            if (!Gui::getCursorCoords(box_second.x, box_second.y, box_second.z))
                return;

            int32_t xD = (box_second.x > box_first.x) ? 1 : -1;
            int32_t yD = (box_second.y > box_first.y) ? 1 : -1;
            for (int32_t xB = box_first.x; (xD > 0) ? (xB <= box_second.x) : (xB >= box_second.x); xB += xD)
            {
                for (int32_t yB = box_first.y; (yD > 0) ? (yB <= box_second.y) : (yB >= box_second.y); yB += yD)
                {
                    if (hollow_selection && !(xB == box_first.x || xB == box_second.x || yB == box_first.y || yB == box_second.y))
                        continue;

                    int8_t color = (xB == box_second.x && yB == box_second.y) ? COLOR_GREEN : COLOR_BROWN;

                    int32_t x = xB - vport.x + 1;
                    int32_t y = yB - vport.y + 1;
                    OutputString(color, x, y, "X");
                }
            }
        }
        else if (show_box_selection && box_select_mode == SELECT_MATERIALS)
        {
            for (vector<df::coord>::iterator it = building_sites.begin(); it != building_sites.end(); it++)
            {
                int32_t x = it->x - vport.x + 1;
                int32_t y = it->y - vport.y + 1;
                OutputString(COLOR_GREEN, x, y, "X");
            }
        }
    }

    static void find_valid_building_sites()
    {
        building_sites.clear();

        int xD = (box_second.x > box_first.x) ? 1 : -1;
        int yD = (box_second.y > box_first.y) ? 1 : -1;
        for (int32_t xB = box_first.x; (xD > 0) ? (xB <= box_second.x) : (xB >= box_second.x); xB += xD)
        {
            for (int32_t yB = box_first.y; (yD > 0) ? (yB <= box_second.y) : (yB >= box_second.y); yB += yD)
            {
                if (hollow_selection && !(xB == box_first.x || xB == box_second.x || yB == box_first.y || yB == box_second.y))
                    continue;

                df::coord pos(xB, yB, box_second.z);
                if (is_valid_building_site(pos))
                    building_sites.push_back(pos);
            }
        }
    }

    void apply_box_selection(bool new_start)
    {
        static bool saved_revert_setting = false;
        static bool auto_select_applied = false;

        box_select_mode = SELECT_MATERIALS;
        if (new_start)
        {
            find_valid_building_sites();
            saved_revert_setting = revert_to_last_used_type;
            revert_to_last_used_type = true;
            auto_select_applied = false;
            box_select_materials.clear();
        }

        while (building_sites.size() > 0)
        {
            df::coord pos = building_sites.back();
            building_sites.pop_back();
            if (box_select_materials.size() > 0)
            {
                df::construction_type type = (df::construction_type) ui_build_selector->building_subtype;
                df::item *item = NULL;
                while (box_select_materials.size() > 0)
                {
                    item = box_select_materials.front();
                    if (!item->flags.bits.in_job)
                        break;
                    box_select_materials.pop_front();
                    item = NULL;
                }

                if (item != NULL)
                {
                    if (designate_new_construction(pos, type, item))
                    {
                        box_select_materials.pop_front();
                        box_select_mode = AUTOSELECT_MATERIALS;
                        send_key(interface_key::LEAVESCREEN); //Must do this to register items in use
                        send_key(hotkeys[type]);
                        box_select_mode = SELECT_MATERIALS;
                    }
                    continue;
                }
            }

            Gui::setCursorCoords(pos.x, pos.y, box_second.z);
            send_key(interface_key::CURSOR_DOWN_Z);
            send_key(interface_key::CURSOR_UP_Z);
            send_key(df::interface_key::SELECT);

            if (in_material_choice_stage())
            {
                if (!auto_select_applied)
                {
                    auto_select_applied = true;
                    if (populate_box_materials(preferred_materials[ui_build_selector->building_subtype]))
                    {
                        building_sites.push_back(pos); //Retry current tile with auto select
                        continue;
                    }
                }

                last_used_moved = false;
                return;
            }
        }

        Gui::setCursorCoords(box_second.x, box_second.y, box_second.z);
        send_key(interface_key::CURSOR_DOWN_Z);
        send_key(interface_key::CURSOR_UP_Z);

        revert_to_last_used_type = saved_revert_setting;
        if (!revert_to_last_used_type)
        {
            send_key(df::interface_key::LEAVESCREEN);
        }

        cancel_box_selection();
        hollow_selection = false;
        ignore_selection = 0;
    }

    void reset_existing_selection()
    {
        for (int i = 0; i < 10; i++)
        {
            send_key(df::interface_key::BUILDING_DIM_Y_DOWN);
            send_key(df::interface_key::BUILDING_DIM_X_DOWN);
        }
    }

    void handle_input(set<df::interface_key> *input)
    {
        if (in_material_choice_stage())
        {
            if (input->count(interface_key::LEAVESCREEN))
            {
                box_select_mode = SELECT_FIRST;
            }

            MaterialDescriptor material = get_material_in_list(ui_build_selector->sel_index);
            if (material.valid)
            {
                if (input->count(interface_key::SELECT) || input->count(interface_key::SEC_SELECT))
                {
                    if (get_last_moved_material().matches(material))
                        last_used_moved = false; //Keep selected material on top

                    set_last_used_material(material);

                    if (box_select_enabled && input->count(interface_key::SEC_SELECT) && ui_build_selector->is_grouped)
                    {
                        auto curr_index = ui_build_selector->sel_index;
                        vector<MaterialDescriptor> gen_material;
                        gen_material.push_back(get_material_in_list(curr_index));
                        box_select_materials.clear();
                        populate_box_materials(gen_material);
                        ui_build_selector->sel_index = curr_index;
                    }
                }
                else if (input->count(interface_key::CUSTOM_A))
                {
                    check_autoselect(material, true);
                    input->clear();
                }
            }
        }
        else if (in_placement_stage())
        {
            if (input->count(interface_key::CUSTOM_A))
            {
                auto_choose_materials = !auto_choose_materials;
            }
            else if (input->count(interface_key::CUSTOM_T))
            {
                revert_to_last_used_type = !revert_to_last_used_type;
            }
            else if (input->count(interface_key::CUSTOM_B))
            {
                box_select_enabled = !box_select_enabled;
                if (!box_select_enabled)
                    cancel_box_selection();
                return;
            }
            else if (input->count(interface_key::LEAVESCREEN))
            {
                switch (box_select_mode)
                {
                case SELECT_FIRST:
                case SELECT_SECOND:
                    cancel_box_selection();

                default:
                    break;
                }
            }
            else if (box_select_enabled && 
                (ui_build_selector->building_subtype == construction_type::Wall || 
                 ui_build_selector->building_subtype == construction_type::Fortification || 
                 ui_build_selector->building_subtype == construction_type::Floor))
            {
                if (input->count(interface_key::SELECT))
                {
                    switch (box_select_mode)
                    {
                    case SELECT_FIRST:
                        if (!Gui::getCursorCoords(box_first.x, box_first.y, box_first.z))
                        {
                            cancel_box_selection();
                            return;
                        }
                        box_select_mode = SELECT_SECOND;
                        if (!show_box_selection)
                            Gui::setDesignationCoords(box_first.x, box_first.y, box_first.z);
                        input->clear();
                        return;

                    case SELECT_SECOND:
                        if (!Gui::getCursorCoords(box_second.x, box_second.y, box_second.z))
                        {
                            cancel_box_selection();
                            return;
                        }
                        cancel_box_selection();
                        input->clear();
                        apply_box_selection(true);
                        return;

                    default:
                        break;
                    }
                }
                else if (input->count(interface_key::CUSTOM_X))
                {
                    show_box_selection = !show_box_selection;
                    if (box_select_mode == SELECT_SECOND)
                    {
                        if (show_box_selection)
                        {
                            Gui::setDesignationCoords(-1, -1, -1);
                        }
                        else
                        {
                            Gui::setDesignationCoords(box_first.x, box_first.y, box_first.z);
                        }
                    }
                }
                else if (input->count(interface_key::CUSTOM_H))
                {
                    hollow_selection = !hollow_selection;
                }
                else if (input->count(interface_key::BUILDING_DIM_Y_UP) ||
                         input->count(interface_key::BUILDING_DIM_Y_DOWN) ||
                         input->count(interface_key::BUILDING_DIM_X_UP) ||
                         input->count(interface_key::BUILDING_DIM_X_DOWN))
                {
                    input->clear();
                    return;
                }
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (ignore_selection < SELECTION_IGNORE_TICKS)
        {
            //FIXME: Sometimes there's an extra ENTER key left over after box selection
            ignore_selection = SELECTION_IGNORE_TICKS;
            return;
        }

        if (box_select_mode != AUTOSELECT_MATERIALS)
            handle_input(input);

        int16_t last_used_constr_subtype = (in_material_choice_stage()) ?  ui_build_selector->building_subtype : -1;
        INTERPOSE_NEXT(feed)(input);

        if (revert_to_last_used_type && 
            last_used_constr_subtype >= 0 && 
            in_type_choice_stage() &&
            hotkeys.find(last_used_constr_subtype) != hotkeys.end())
        {
            input->clear();
            input->insert(hotkeys[last_used_constr_subtype]);
            INTERPOSE_NEXT(feed)(input);

            if (box_select_mode == SELECT_MATERIALS)
            {
                apply_box_selection(false);
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        if (ignore_selection < SELECTION_IGNORE_TICKS)
        {
            ++ignore_selection;
        }

        if (in_material_choice_stage())
        {
            if (!last_used_moved && ui_build_selector->is_grouped)
            {
                last_used_moved = true;
                if (choose_materials())
                {
                    return;
                }
                else
                {
                    move_material_to_top(get_last_used_material());
                }
            }
            else if (!ui_build_selector->is_grouped)
            {
                last_used_moved = false;
            }
        }
        else
        {
            last_used_moved = false;
        }

        INTERPOSE_NEXT(render)();

        draw_box_selection();

        if (in_type_choice_stage())
        {
            cancel_box_selection();
            return;
        }

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = 25;
        if (in_material_choice_stage())
        {
            MaterialDescriptor material = get_material_in_list(ui_build_selector->sel_index);
            if (material.valid)
            {
                string toggle_string = "Enable";
                string title = "Disabled";
                if (check_autoselect(material, false))
                {
                    toggle_string = "Disable";
                    title = "Enabled";
                }

                OutputString(COLOR_BROWN, x, y, "DFHack Autoselect: " + title, true, left_margin);
                OutputHotkeyString(x, y, toggle_string.c_str(), "a", true, left_margin);

                if (box_select_mode == SELECT_MATERIALS)
                {
                    ++y;
                    OutputString(COLOR_BROWN, x, y, "Construction:", true, left_margin);
                    OutputString(COLOR_WHITE, x, y, int_to_string(building_sites.size() + 1) + " tiles to fill", true, left_margin);
                }
            }
        }
        else if (in_placement_stage() && ui_build_selector->building_subtype < 7)
        {
            string autoselect_toggle_string = (auto_choose_materials) ? "Disable Auto Mat-select" : "Enable Auto Mat-select";
            string revert_toggle_string = (revert_to_last_used_type) ? "Disable Auto Type-select" : "Enable Auto Type-select";

            OutputString(COLOR_BROWN, x, y, "DFHack Options", true, left_margin);
            OutputHotkeyString(x, y, autoselect_toggle_string.c_str(), "a", true, left_margin);
            OutputHotkeyString(x, y, revert_toggle_string.c_str(), "t", true, left_margin);

            if (ui_build_selector->building_subtype == construction_type::Wall || 
                ui_build_selector->building_subtype == construction_type::Fortification || 
                ui_build_selector->building_subtype == construction_type::Floor)
            {
                ++y;
                OutputHotkeyString(x, y, (box_select_enabled) ? "Disable Box Select" : "Enable Box Select", "b", true, left_margin);
                if (box_select_enabled)
                {
                    OutputHotkeyString(x, y, (show_box_selection) ? "Disable Box Marking" : "Enable Box Marking", "x", true, left_margin);
                    OutputHotkeyString(x, y, (hollow_selection) ? "Make Solid" : "Make Hollow", "h", true, left_margin);
                }
                ++y;
                if (box_select_enabled)
                {
                    Screen::Pen pen(' ',COLOR_BLACK);
                    y = dims.y1 + 2;
                    Screen::fillRect(pen, x, y, dims.menu_x2, y + 17);

                    y += 2;
                    switch (box_select_mode)
                    {
                    case SELECT_FIRST:
                        OutputString(COLOR_BROWN, x, y, "Choose first corner", true, left_margin);
                        break;

                    case SELECT_SECOND:
                        OutputString(COLOR_GREEN, x, y, "Choose second corner", true, left_margin);
                        int cx = box_first.x;
                        int cy = box_first.y;
                        OutputString(COLOR_BROWN, cx, cy, "X");
                    }

                    OutputString(COLOR_BROWN, x, ++y, "Ignore Building Restrictions", true, left_margin);
                }
            }
        }
    }
};

color_ostream_proxy console_out(Core::getInstance().getConsole());


IMPLEMENT_VMETHOD_INTERPOSE(jobutils_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(jobutils_hook, render);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(jobutils_hook, feed).apply() || !INTERPOSE_HOOK(jobutils_hook, render).apply())
        out.printerr("Could not insert jobutils hooks!\n");

    hotkeys[construction_type::Wall] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_WALL;
    hotkeys[construction_type::Floor] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_FLOOR;
    hotkeys[construction_type::Ramp] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_RAMP;
    hotkeys[construction_type::UpStair] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_UP;
    hotkeys[construction_type::DownStair] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_DOWN;
    hotkeys[construction_type::UpDownStair] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN;
    hotkeys[construction_type::Fortification] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_FORTIFICATION;
    //Ignore tracks, DF already returns to track menu

    return CR_OK;
}
