// Auto Material Select

#include "Core.h"
#include <Console.h>
#include "DataDefs.h"
#include <Export.h>
#include "MiscUtils.h"
#include <PluginManager.h>
#include <VTableInterpose.h>

#include "modules/Buildings.h"
#include "modules/Constructions.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Screen.h"

// DF data structure definition headers
#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/building_constructionst.h"
#include "df/construction_type.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "TileTypes.h"

#include <map>
#include <string>
#include <vector>

using namespace std;
using std::map;
using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("automaterial");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);

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

void OutputToggleString(int &x, int &y, const char *text, const char *hotkey, bool state, bool newline = true, int left_margin = 0, int8_t color = COLOR_WHITE)
{
    OutputHotkeyString(x, y, text, hotkey);
    OutputString(COLOR_WHITE, x, y, ": ");
    if (state)
        OutputString(COLOR_GREEN, x, y, "Enabled", newline, left_margin);
    else
        OutputString(COLOR_GREY, x, y, "Disabled", newline, left_margin);
}

static string int_to_string(int i)
{
    return static_cast<ostringstream*>( &(ostringstream() << i))->str();
}

//START UI Functions
struct coord32_t
{
    int32_t x, y, z;
};

static enum t_box_select_mode {SELECT_FIRST, SELECT_SECOND, SELECT_MATERIALS, AUTOSELECT_MATERIALS} box_select_mode = SELECT_FIRST;
static coord32_t box_first, box_second;
static bool box_select_enabled = false;
static bool show_box_selection = true;
static bool hollow_selection = false;
static deque<df::item*> box_select_materials;

#define SELECTION_IGNORE_TICKS 10
static int ignore_selection = SELECTION_IGNORE_TICKS;

static map<int16_t, MaterialDescriptor> last_used_material;
static map<int16_t, MaterialDescriptor> last_moved_material;
static map< int16_t, vector<MaterialDescriptor> > preferred_materials;
static map< int16_t, df::interface_key > hotkeys;
static bool last_used_moved = false;
static bool auto_choose_materials = true;
static bool auto_choose_attempted = true;
static bool revert_to_last_used_type = false;
static bool allow_future_placement = false;

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
//END UI Functions


//START Building and Verification
struct building_site
{
    df::coord pos;
    bool in_open_air;

    building_site(df::coord pos, bool in_open_air)
    {
        this->pos = pos;
        this->in_open_air = in_open_air;
    }

    building_site() {}
};

static deque<building_site> valid_building_sites;
static deque<building_site> open_air_sites;
static building_site anchor;

static bool is_orthogonal_to_pending_construction(building_site &site)
{
    for (deque<building_site>::iterator it = valid_building_sites.begin(); it != valid_building_sites.end(); it++)
    {
        if ((it->pos.x == site.pos.x && abs(it->pos.y - site.pos.y) == 1) || (it->pos.y == site.pos.y && abs(it->pos.x - site.pos.x) == 1))
        {
            site.in_open_air = true;
            return true;
        }
    }

    return false;
}

static df::building_constructionst *get_construction_on_tile(const df::coord &pos)
{
    auto current = Buildings::findAtTile(pos);
    if (current)
        return strict_virtual_cast<df::building_constructionst>(current);

    return NULL;
}

static df::tiletype *read_tile_shapes(const df::coord &pos, df::tiletype_shape &shape, df::tiletype_shape_basic &shape_basic)
{
    if (!Maps::isValidTilePos(pos))
        return NULL;

    auto ttype = Maps::getTileType(pos);

    if (!ttype)
        return NULL;

    shape = tileShape(*ttype);
    shape_basic = tileShapeBasic(shape);

    return ttype;
}

static bool is_valid_building_site(building_site &site, bool orthogonal_check, bool check_placed_constructions, bool in_future_placement_mode)
{
    df::tiletype_shape shape;
    df::tiletype_shape_basic shape_basic;

    auto ttype = read_tile_shapes(site.pos, shape, shape_basic);
    if (!ttype)
        return false;

    if (shape_basic == tiletype_shape_basic::Open)
    {
        if (orthogonal_check)
        {
            // Check if this is a valid tile to have a construction placed orthogonally to it
            if (!in_future_placement_mode)
                return false;

            df::building_constructionst *cons = get_construction_on_tile(site.pos);
            if (cons && cons->type == construction_type::Floor)
            {
                site.in_open_air = true;
                return true;
            }

            return false;
        }

        // Stairs can be placed in open space, if they can connect to other stairs
        df::tiletype_shape shape_s;
        df::tiletype_shape_basic shape_basic_s;

        if (ui_build_selector->building_subtype == construction_type::DownStair ||
            ui_build_selector->building_subtype == construction_type::UpDownStair)
        {
            df::coord below(site.pos.x, site.pos.y, site.pos.z - 1);
            auto ttype_s = read_tile_shapes(below, shape_s, shape_basic_s);
            if (ttype_s)
            {
                if (shape_s == tiletype_shape::STAIR_UP || shape_s == tiletype_shape::STAIR_UPDOWN)
                    return true;
            }
        }

        if (ui_build_selector->building_subtype == construction_type::UpStair ||
            ui_build_selector->building_subtype == construction_type::UpDownStair)
        {
            df::coord above(site.pos.x, site.pos.y, site.pos.z + 1);
            auto ttype_s = read_tile_shapes(above, shape_s, shape_basic_s);
            if (ttype_s)
            {
                if (shape_s == tiletype_shape::STAIR_DOWN || shape_s == tiletype_shape::STAIR_UPDOWN)
                    return true;
            }
        }

        // Check if there is a valid tile orthogonally adjacent
        bool valid_orthogonal_tile_found = false;
        df::coord orthagonal_pos;
        orthagonal_pos.z = site.pos.z;
        for (orthagonal_pos.x = site.pos.x-1; orthagonal_pos.x <= site.pos.x+1 && !valid_orthogonal_tile_found; orthagonal_pos.x++)
        {
            for (orthagonal_pos.y = site.pos.y-1; orthagonal_pos.y <= site.pos.y+1; orthagonal_pos.y++)
            {
                if ((site.pos.x == orthagonal_pos.x) == (site.pos.y == orthagonal_pos.y))
                    continue;

                building_site orthogonal_site(orthagonal_pos, false);
                if (is_valid_building_site(orthogonal_site, true, check_placed_constructions, in_future_placement_mode))
                {
                    valid_orthogonal_tile_found = true;
                    if (orthogonal_site.in_open_air)
                        site.in_open_air = true;
                    break;
                }

            }
        }

        if (!(valid_orthogonal_tile_found || (check_placed_constructions && is_orthogonal_to_pending_construction(site))))
        {
            site.in_open_air = true;
            return false;
        }
    }
    else if (orthogonal_check)
    {
        if (shape != tiletype_shape::RAMP &&
            shape_basic != tiletype_shape_basic::Floor &&
            shape_basic != tiletype_shape_basic::Stair)
            return false;
    }
    else
    {
        auto material = tileMaterial(*ttype);
        if (shape == tiletype_shape::RAMP)
        {
            if (material == tiletype_material::CONSTRUCTION)
                return false;
        }
        else
        {
            if (shape != tiletype_shape::STAIR_DOWN && shape_basic != tiletype_shape_basic::Floor)
                return false;

            // Can build on top of a wall, but not on other construction
            auto construction = Constructions::findAtTile(site.pos);
            if (construction)
            {
                if (construction->flags.bits.top_of_wall==0)
                    return false;
            }

            if (material == tiletype_material::FIRE ||
                material == tiletype_material::POOL ||
                material == tiletype_material::BROOK ||
                material == tiletype_material::RIVER ||
                material == tiletype_material::MAGMA ||
                material == tiletype_material::DRIFTWOOD ||
                material == tiletype_material::CAMPFIRE
                )

                return false;
        }
    }

    if (orthogonal_check)
        return true;

    auto designation = Maps::getTileDesignation(site.pos);
    if (designation->bits.flow_size > 2)
        return false;

    auto current = Buildings::findAtTile(site.pos);
    if (current)
        return false;

    df::coord2d size(1,1);
    return Buildings::checkFreeTiles(site.pos, size, NULL, false, false);
}


static bool find_anchor_in_spiral(const df::coord &start)
{
    bool found = false;

    for (anchor.pos.z = start.z; anchor.pos.z > start.z - 4; anchor.pos.z--)
    {
        int x, y, dx, dy;
        x = y = dx = 0;
        dy = -1;
        const int side = 11;
        const int maxI = side*side;
        for (int i = 0; i < maxI; i++)
        {
            if (-side/2 < x && x <= side/2 && -side/2 < y && y <= side/2)
            {
                anchor.pos.x = start.x + x;
                anchor.pos.y = start.y + y;
                if (is_valid_building_site(anchor, false, false, false))
                {
                    found = true;
                    break;
                }
            }

            if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1-y)))
            {
                int tmp = dx;
                dx = -dy;
                dy = tmp;
            }

            x += dx;
            y += dy;
        }

        if (found)
            break;
    }

    return found;
}

static bool find_valid_building_sites(bool in_future_placement_mode)
{
    valid_building_sites.clear();
    open_air_sites.clear();

    int xD = (box_second.x > box_first.x) ? 1 : -1;
    int yD = (box_second.y > box_first.y) ? 1 : -1;
    for (int32_t xB = box_first.x; (xD > 0) ? (xB <= box_second.x) : (xB >= box_second.x); xB += xD)
    {
        for (int32_t yB = box_first.y; (yD > 0) ? (yB <= box_second.y) : (yB >= box_second.y); yB += yD)
        {
            if (hollow_selection && !(xB == box_first.x || xB == box_second.x || yB == box_first.y || yB == box_second.y))
                continue;

            building_site site(df::coord(xB, yB, box_second.z), false);
            if (is_valid_building_site(site, false, true, in_future_placement_mode))
                valid_building_sites.push_back(site);
            else if (site.in_open_air)
            {
                if (in_future_placement_mode)
                    valid_building_sites.push_back(site);
                else
                    open_air_sites.push_back(site);
            }
        }
    }

    size_t last_open_air_count = 0;
    while (valid_building_sites.size() > 0 && open_air_sites.size() != last_open_air_count)
    {
        last_open_air_count = open_air_sites.size();
        deque<building_site> current_open_air_list = open_air_sites;
        open_air_sites.clear();
        for (deque<building_site>::iterator it = current_open_air_list.begin(); it != current_open_air_list.end(); it++)
        {
            if (is_orthogonal_to_pending_construction(*it))
                valid_building_sites.push_back(*it);
            else
                open_air_sites.push_back(*it);
        }

    }

    return valid_building_sites.size() > 0;
}

static bool designate_new_construction(df::coord &pos, df::construction_type &type, df::item *item)
{
    auto newinst = Buildings::allocInstance(pos, building_type::Construction, type);
    if (!newinst)
        return false;

    vector<df::item*> items;
    items.push_back(item);
    Maps::ensureTileBlock(pos);

    if (!Buildings::constructWithItems(newinst, items))
    {
        delete newinst;
        return false;
    }

    return true;
}
//END Building and Verification


//START Viewscreen Hook
struct jobutils_hook : public df::viewscreen_dwarfmodest
{
    //START UI Methods
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

    void draw_box_selection()
    {
        if (!box_select_enabled)
            return;

        if (ui->main.mode != df::ui_sidebar_mode::Build ||
            ui_build_selector->building_type != df::building_type::Construction)
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
            for (deque<building_site>::iterator it = valid_building_sites.begin(); it != valid_building_sites.end(); it++)
            {
                int32_t x = it->pos.x - vport.x + 1;
                int32_t y = it->pos.y - vport.y + 1;
                OutputString(COLOR_GREEN, x, y, "X");
            }
        }
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
        if (ui_build_selector->building_subtype >= 7)
            return;

        if (in_material_choice_stage())
        {
            if (input->count(interface_key::LEAVESCREEN))
            {
                box_select_mode = SELECT_FIRST;
            }

            MaterialDescriptor material = get_material_in_list(ui_build_selector->sel_index);
            if (material.valid)
            {
                if (input->count(interface_key::SELECT) || input->count(interface_key::SELECT_ALL))
                {
                    if (get_last_moved_material().matches(material))
                        last_used_moved = false; //Keep selected material on top

                    set_last_used_material(material);

                    if (box_select_enabled)
                    {
                        auto curr_index = ui_build_selector->sel_index;
                        vector<MaterialDescriptor> gen_material;
                        gen_material.push_back(get_material_in_list(curr_index));
                        box_select_materials.clear();
                        // Populate material list with selected material
                        populate_box_materials(gen_material, ((input->count(interface_key::SELECT_ALL) && ui_build_selector->is_grouped) ? -1 : 1));

                        input->clear(); // Let the apply_box_selection routine allocate the construction
                        input->insert(interface_key::LEAVESCREEN);
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
                reset_existing_selection();
                box_select_enabled = !box_select_enabled;
                if (!box_select_enabled)
                    cancel_box_selection();

                return;
            }
            else if (input->count(interface_key::CUSTOM_O))
            {
                allow_future_placement = !allow_future_placement;
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
            else if (box_select_enabled)
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
    //END UI Methods

    //START Building Application
    bool populate_box_materials(vector<MaterialDescriptor> &gen_materials, int32_t count = -1)
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
                        if (count > -1)
                            return true; // Right now we only support 1 or all materials

                        result = true;
                        break;
                    }
                }
            }
        }
        send_key(interface_key::BUILDING_EXPAND_CONTRACT);

        return result;
    }

    void move_cursor(df::coord &pos)
    {
        Gui::setCursorCoords(pos.x, pos.y, pos.z);
        send_key(interface_key::CURSOR_DOWN_Z);
        send_key(interface_key::CURSOR_UP_Z);
    }

    void move_cursor(coord32_t &pos)
    {
        df::coord c((int16_t) pos.x, (int16_t) pos.y, (int16_t) pos.z);
        move_cursor(c);
    }

    void apply_box_selection(bool new_start)
    {
        static bool saved_revert_setting = false;
        static bool auto_select_applied = false;

        box_select_mode = SELECT_MATERIALS;
        if (new_start)
        {
            bool ok_to_continue = false;
            bool in_future_placement_mode = false;
            if (!find_valid_building_sites(false))
            {
                if (allow_future_placement)
                {
                    in_future_placement_mode = find_valid_building_sites(true);
                }
            }
            else
            {
                ok_to_continue = true;
            }

            if (in_future_placement_mode)
            {
                ok_to_continue = find_anchor_in_spiral(valid_building_sites[0].pos);
            }
            else if (ok_to_continue)
            {
                // First valid site is guaranteed to be anchored, either on a tile or against a valid orthogonal tile
                // Use it as an anchor point to generate materials list
                anchor = valid_building_sites.front();
                valid_building_sites.pop_front();
                valid_building_sites.push_back(anchor);
            }

            if (!ok_to_continue)
            {
                cancel_box_selection();
                hollow_selection = false;
                return;
            }

            saved_revert_setting = revert_to_last_used_type;
            revert_to_last_used_type = true;
            auto_select_applied = false;
            box_select_materials.clear();

        }

        while (valid_building_sites.size() > 0)
        {
            building_site site = valid_building_sites.front();
            valid_building_sites.pop_front();
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
                    if (designate_new_construction(site.pos, type, item))
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

            // Generate material list using regular construction placement routine

            if (site.in_open_air)
            {
                // Cannot invoke material selection on an unconnected tile, use anchor instead
                move_cursor(anchor.pos);
                send_key(df::interface_key::SELECT);
            }

            move_cursor(site.pos);

            if (!site.in_open_air)
                send_key(df::interface_key::SELECT);

            if (in_material_choice_stage())
            {
                valid_building_sites.push_front(site); //Redo current tile with whatever gets selected
                if (!auto_select_applied)
                {
                    // See if any auto select materials are available
                    auto_select_applied = true;
                    if (auto_choose_materials && populate_box_materials(preferred_materials[ui_build_selector->building_subtype]))
                    {
                        continue;
                    }
                }

                last_used_moved = false;
                return; // No auto select materials left, ask user
            }
        }

        // Allocation done, reset
        move_cursor(box_second);

        revert_to_last_used_type = saved_revert_setting;
        if (!revert_to_last_used_type)
        {
            send_key(df::interface_key::LEAVESCREEN);
        }

        cancel_box_selection();
        hollow_selection = false;
        ignore_selection = 0;
    }
    //END Building Application

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
                if (!box_select_enabled && choose_materials())
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
                OutputToggleString(x, y, "Autoselect", "a", check_autoselect(material, false), true, left_margin);

                if (box_select_mode == SELECT_MATERIALS)
                {
                    ++y;
                    OutputString(COLOR_BROWN, x, y, "Construction:", true, left_margin);
                    OutputString(COLOR_WHITE, x, y, int_to_string(valid_building_sites.size()) + " tiles to fill", true, left_margin);
                }
            }
        }
        else if (in_placement_stage() && ui_build_selector->building_subtype < 7)
        {
            OutputString(COLOR_BROWN, x, y, "DFHack Options", true, left_margin);
            OutputToggleString(x, y, "Auto Mat-select", "a", auto_choose_materials, true, left_margin);
            OutputToggleString(x, y, "Reselect Type", "t", revert_to_last_used_type, true, left_margin);

            ++y;
            OutputToggleString(x, y, "Box Select", "b", box_select_enabled, true, left_margin);
            if (box_select_enabled)
            {
                OutputToggleString(x, y, "Show Box Mask", "x", show_box_selection, true, left_margin);
                OutputHotkeyString(x, y, (hollow_selection) ? "Make Solid" : "Make Hollow", "h", true, left_margin);
                OutputToggleString(x, y, "Open Placement", "o", allow_future_placement, true, left_margin);
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

                    int32_t curr_x, curr_y, curr_z;
                    Gui::getCursorCoords(curr_x, curr_y, curr_z);
                    int dX = abs(box_first.x - curr_x) + 1;
                    int dY = abs(box_first.y - curr_y) + 1;
                    stringstream label;
                    label << "Selection: " << dX << "x" << dY;
                    OutputString(COLOR_WHITE, x, ++y, label.str(), true, left_margin);

                    int cx = box_first.x;
                    int cy = box_first.y;
                    OutputString(COLOR_BROWN, cx, cy, "X");
                }

                OutputString(COLOR_BROWN, x, ++y, "Ignore Building Restrictions", true, left_margin);
            }
        }
    }
};
//END Viewscreen Hook

color_ostream_proxy console_out(Core::getInstance().getConsole());


IMPLEMENT_VMETHOD_INTERPOSE(jobutils_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(jobutils_hook, render);

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(jobutils_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(jobutils_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
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
