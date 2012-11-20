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
#include "df/viewscreen_dwarfmodest.h"
#include "df/ui_build_selector.h"
#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/item.h"

#include "df/ui.h"
#include "modules/Gui.h"
#include "modules/Screen.h"

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

typedef int16_t construction_type;

static map<construction_type, MaterialDescriptor> last_used_material;
static map<construction_type, MaterialDescriptor> last_moved_material;
static map< construction_type, vector<MaterialDescriptor> > preferred_materials;
static map< construction_type, df::interface_key > hotkeys;
static bool last_used_moved = false;
static bool auto_choose_materials = true;
static bool auto_choose_attempted = true;
static bool revert_to_last_used_type = false;

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


static inline bool in_material_choice_stage()
{
    return Gui::build_selector_hotkey(Core::getTopViewscreen()) && 
        ui->main.mode == ui_sidebar_mode::Build && 
        ui_build_selector->stage == 2;
}

static inline bool in_type_choice_stage()
{
    return Gui::dwarfmode_hotkey(Core::getTopViewscreen()) && 
        ui->main.mode == ui_sidebar_mode::Build &&
        ui_build_selector &&
        ui_build_selector->building_type >= 0 &&
        ui_build_selector->stage == 1;
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

static void set_last_used_material(MaterialDescriptor &matetial)
{
    last_used_material[ui_build_selector->building_subtype] = matetial;
}

static MaterialDescriptor &get_last_moved_material()
{
    if (last_moved_material.find(ui_build_selector->building_subtype) == last_moved_material.end())
        last_moved_material[ui_build_selector->building_subtype] = MaterialDescriptor();

    return last_moved_material[ui_build_selector->building_subtype];
}

static void set_last_moved_material(MaterialDescriptor &matetial)
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
        result.item_type = gen->item_type;
        result.item_subtype = gen->item_subtype;
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

static bool choose_materials()
{
    size_t size = ui_build_selector->choices.size();
    for (size_t i = 0; i < size; i++)
    {
        MaterialDescriptor material = get_material_in_list(i);
        size_t j;
        if (is_material_in_autoselect(j, material))
        {
            ui_build_selector->sel_index = i;
            std::set< df::interface_key > keys;
            keys.insert(df::interface_key::SELECT_ALL);
            Core::getTopViewscreen()->feed(&keys);
            if (!in_material_choice_stage())
                return true;
        }
    }

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

struct jobutils_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (in_material_choice_stage())
        {
            MaterialDescriptor material = get_material_in_list(ui_build_selector->sel_index);
            if (material.valid)
            {
                if (input->count(interface_key::SELECT) || input->count(interface_key::SEC_SELECT))
                {
                    if (get_last_moved_material().matches(material))
                        last_used_moved = false;

                    set_last_used_material(material);
                }
                else if (input->count(interface_key::CUSTOM_A))
                {
                    check_autoselect(material, true);
                    input->clear();
                }
            }
        }
        else if (in_type_choice_stage())
        {
            if (input->count(interface_key::CUSTOM_A))
            {
                auto_choose_materials = !auto_choose_materials;
            }
            else if (input->count(interface_key::CUSTOM_T))
            {
                revert_to_last_used_type = !revert_to_last_used_type;
            }
        }

        construction_type last_used_constr_subtype = (in_material_choice_stage()) ?  ui_build_selector->building_subtype : -1;
        INTERPOSE_NEXT(feed)(input);

        if (revert_to_last_used_type && last_used_constr_subtype >= 0 && !in_material_choice_stage())
        {
            input->clear();
            std::set< df::interface_key > keys;
            keys.insert(hotkeys[last_used_constr_subtype]);
            Core::getTopViewscreen()->feed(&keys);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        if (in_material_choice_stage())
        {
            if (!last_used_moved)
            {
                if (auto_choose_materials && get_curr_constr_prefs().size() > 0)
                {
                    last_used_moved = true;
                    if (choose_materials())
                    {
                        return;
                    }
                }
                else if (ui_build_selector->is_grouped)
                {
                    last_used_moved = true;
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

        if (in_material_choice_stage())
        {
            MaterialDescriptor material = get_material_in_list(ui_build_selector->sel_index);
            if (material.valid)
            {
                int left_margin = gps->dimx - 30;
                int x = left_margin;
                int y = 25;

                string toggle_string = "Enable";
                string title = "Disabled";
                if (check_autoselect(material, false))
                {
                    toggle_string = "Disable";
                    title = "Enabled";
                }

                OutputString(COLOR_BROWN, x, y, "DFHack Autoselect: " + title, true, left_margin);
                OutputHotkeyString(x, y, toggle_string.c_str(), "a", true, left_margin);
            }
        }
        else if (in_type_choice_stage() && ui_build_selector->building_subtype != 7)
        {
            int left_margin = gps->dimx - 30;
            int x = left_margin;
            int y = 25;

            string autoselect_toggle_string = (auto_choose_materials) ? "Disable Auto Mat-select" : "Enable Auto Mat-select";
            string revert_toggle_string = (revert_to_last_used_type) ? "Disable Auto Type-select" : "Enable Auto Type-select";

            OutputString(COLOR_BROWN, x, y, "DFHack Options", true, left_margin);
            OutputHotkeyString(x, y, autoselect_toggle_string.c_str(), "a", true, left_margin);
            OutputHotkeyString(x, y, revert_toggle_string.c_str(), "t", true, left_margin);
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

    hotkeys[1] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_WALL;
    hotkeys[2] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_FLOOR;
    hotkeys[6] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_RAMP;
    hotkeys[3] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_UP;
    hotkeys[4] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_DOWN;
    hotkeys[5] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN;
    hotkeys[0] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_FORTIFICATION;
    hotkeys[7] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_TRACK;
    hotkeys[5] = df::interface_key::HOTKEY_BUILDING_CONSTRUCTION_TRACK_STOP;
        
    commands.push_back(PluginCommand(
        "automaterial", "Makes construction easier by auto selecting materials",
        automaterial_cmd, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  Makes construction easier by auto selecting materials.\n"
        ));
    return CR_OK;
}


/*
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        // initialize from the world just loaded
        break;
    case SC_GAME_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}
*/

/*
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    return CR_OK;
}
*/