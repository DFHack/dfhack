#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>


// DF data structure definition headers
#include "DataDefs.h"
#include "MiscUtils.h"
#include "Types.h"
#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/item.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/world.h"
#include "df/building_constructionst.h"
#include "df/enabler.h"
#include "df/building_design.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Items.h"

#include "TileTypes.h"
#include "df/job_item.h"
#include "df/dfhack_material_category.h"
#include "df/general_ref_building_holderst.h"
#include "modules/Job.h"
#include "df/building_design.h"
#include "df/buildings_other_id.h"
#include "modules/World.h"
#include <functional>
#include "df/building.h"

using std::map;
using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::ui;
using df::global::ui_build_selector;
using df::global::world;
using df::global::enabler;

DFHACK_PLUGIN("buildingplan");
#define PLUGIN_VERSION 0.3

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

struct coord32_t
{
    int32_t x, y, z;

    df::coord get_coord16() const
    {
        return df::coord(x, y, z);
    }
};

typedef int8_t UIColor;

const int ascii_to_enum_offset = interface_key::STRING_A048 - '0';

inline string int_to_string(const int n)
{
    return static_cast<ostringstream*>( &(ostringstream() << n) )->str();
}

static void set_to_limit(int &value, const int maximum, const int min = 0)
{
    if (value < min)
        value = min;
    else if (value > maximum)
        value = maximum;
}

inline void paint_text(const UIColor color, const int &x, const int &y, const std::string &text, const UIColor background = 0)
{
    Screen::paintString(Screen::Pen(' ', color, background), x, y, text);
}

static string pad_string(string text, const int size, const bool front = true, const bool trim = false)
{
    if (text.length() > size)
    {
        if (trim && size > 10)
        {
            text = text.substr(0, size-3);
            text.append("...");
        }
        return text;
    }

    string aligned(size - text.length(), ' ');
    if (front)
    {
        aligned.append(text);
        return aligned;
    }
    else
    {
        text.append(aligned);
        return text;
    }
}

template <class T, typename Fn>
static void for_each_(vector<T> &v, Fn func)
{
    for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn> 
static void transform_(vector<T> &src, vector<V> &dst, Fn func)
{
    transform(src.begin(), src.end(), back_inserter(dst), func);
}


#define MAX_MASK 10
#define MAX_MATERIAL 21

#define SIDEBAR_WIDTH 30
#define COLOR_TITLE COLOR_BLUE
#define COLOR_UNSELECTED COLOR_GREY
#define COLOR_SELECTED COLOR_WHITE
#define COLOR_HIGHLIGHTED COLOR_GREEN


/*
 * List classes
 */
template <typename T>
class ListEntry
{
public:
    T elem;
    string text;
    bool selected;

    ListEntry(string text, T elem)
    {
        this->text = text;
        this->elem = elem;
        selected = false;
    }
};

template <typename T>
class ListColumn
{
public:
    string title;
    int highlighted_index;
    int display_start_offset;
    int32_t bottom_margin, search_margin, left_margin;
    bool multiselect;
    bool allow_null;
    bool auto_select;
    bool force_sort;
    bool allow_search;

    ListColumn()
    {
        clear();
        left_margin = 2;
        bottom_margin = 3;
        search_margin = 63;
        highlighted_index = 0;
        multiselect = false;
        allow_null = true;
        auto_select = false;
        force_sort = false;
        allow_search = true;
    }

    void clear()
    {
        list.clear();
        display_list.clear();
        display_start_offset = 0;
        max_item_width = 0;
        resize();
    }

    void resize()
    {
        display_max_rows = gps->dimy - 4 - bottom_margin;
    }

    void add(ListEntry<T> &entry)
    {
        list.push_back(entry);
        if (entry.text.length() > max_item_width)
            max_item_width = entry.text.length();
    }

    void add(const string &text, T &elem)
    {
        list.push_back(ListEntry<T>(text, elem));
        if (text.length() > max_item_width)
            max_item_width = text.length();
    }

    virtual void display_extras(const T &elem, int32_t &x, int32_t &y) const {}

    void display(const bool is_selected_column) const
    {
        int32_t y = 2;
        paint_text(COLOR_TITLE, left_margin, y, title);

        int last_index_able_to_display = display_start_offset + display_max_rows;
        for (int i = display_start_offset; i < display_list.size() && i < last_index_able_to_display; i++)
        {
            ++y;
            UIColor fg_color = (display_list[i]->selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
            UIColor bg_color = (is_selected_column && i == highlighted_index) ? COLOR_HIGHLIGHTED : COLOR_BLACK;
            paint_text(fg_color, left_margin, y, display_list[i]->text, bg_color);
            int x = left_margin + display_list[i]->text.length() + 1;
            display_extras(display_list[i]->elem, x, y);
        }

        if (is_selected_column && allow_search)
        {
            y = gps->dimy - bottom_margin;
            int32_t x = search_margin;
            OutputHotkeyString(x, y, "Search" ,"S");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString(COLOR_WHITE, x, y, search_string);
            OutputString(COLOR_LIGHTGREEN, x, y, "_");
        }
    }

    void filterDisplay()
    {
        ListEntry<T> *prev_selected = (getDisplayListSize() > 0) ? display_list[highlighted_index] : NULL;
        display_list.clear();
        for (size_t i = 0; i < list.size(); i++)
        {
            if (search_string.empty() || list[i].text.find(search_string) != string::npos)
            {
                ListEntry<T> *entry = &list[i];
                display_list.push_back(entry);
                if (entry == prev_selected)
                    highlighted_index = display_list.size() - 1;
            }
        }
        changeHighlight(0);
    }

    void selectDefaultEntry()
    {
        for (size_t i = 0; i < display_list.size(); i++)
        {
            if (display_list[i]->selected)
            {
                highlighted_index = i;
                break;
            }
        }
    }

    void validateHighlight()
    {
        set_to_limit(highlighted_index, display_list.size() - 1);

        if (highlighted_index < display_start_offset)
            display_start_offset = highlighted_index;
        else if (highlighted_index >= display_start_offset + display_max_rows)
            display_start_offset = highlighted_index - display_max_rows + 1;

        if (auto_select || (!allow_null && list.size() == 1))
            display_list[highlighted_index]->selected = true;
    }

    void changeHighlight(const int highlight_change, const int offset_shift = 0)
    {
        if (!initHighlightChange())
            return;

        highlighted_index += highlight_change + offset_shift * display_max_rows;

        display_start_offset += offset_shift * display_max_rows;
        set_to_limit(display_start_offset, max(0, (int)(display_list.size())-display_max_rows));
        validateHighlight();
    }

    void setHighlight(const int index)
    {
        if (!initHighlightChange())
            return;

        highlighted_index = index;
        validateHighlight();
    }

    bool initHighlightChange()
    {
        if (display_list.size() == 0)
            return false;

        if (auto_select && !multiselect)
        {
            for (typename vector< ListEntry<T> >::iterator it = list.begin(); it != list.end(); it++)
            {
                it->selected = false;
            }
        }

        return true;
    }

    void toggleHighlighted()
    {
        if (auto_select)
            return;

        ListEntry<T> *entry = display_list[highlighted_index];
        if (!multiselect || !allow_null)
        {
            int selected_count = 0;
            for (size_t i = 0; i < list.size(); i++)
            {
                if (!multiselect && !entry->selected)
                    list[i].selected = false;
                if (!allow_null && list[i].selected)
                    selected_count++;
            }

            if (!allow_null && entry->selected && selected_count == 1)
                return;
        }

        entry->selected = !entry->selected;
    }

    vector<T*> getSelectedElems(bool only_one = false)
    {
        vector<T*> results;
        for (auto it = list.begin(); it != list.end(); it++)
        {
            if ((*it).selected)
            {
                results.push_back(&(it->elem));
                if (only_one)
                    break;
            }
        }

        return results;
    }

    void clear_selection()
    {
        for_each_(list, [] (ListEntry<T> &e) { e.selected = false; });
    }

    T* getFirstSelectedElem()
    {
        vector<T*> results = getSelectedElems(true);
        if (results.size() == 0)
            return NULL;
        else
            return results[0];
    }

    size_t getDisplayListSize()
    {
        return display_list.size();
    }

    size_t getBaseListSize()
    {
        return list.size();
    }

    bool feed(set<df::interface_key> *input)
    {
        if  (input->count(interface_key::CURSOR_UP))
        {
            changeHighlight(-1);
        }
        else if  (input->count(interface_key::CURSOR_DOWN))
        {
            changeHighlight(1);
        }
        else if  (input->count(interface_key::STANDARDSCROLL_PAGEUP))
        {
            changeHighlight(0, -1);
        }
        else if  (input->count(interface_key::STANDARDSCROLL_PAGEDOWN))
        {
            changeHighlight(0, 1);
        }
        else if  (input->count(interface_key::SELECT) && !auto_select)
        {
            toggleHighlighted();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_S))
        {
            search_string.clear();
            filterDisplay();
        }
        else if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1 && enabler->mouse_lbut)
        {
            return setHighlightByMouse();
        }
        else if (allow_search)
        {
            // Search query typing mode always on

            df::interface_key last_token = *input->rbegin();
            if (last_token >= interface_key::STRING_A096 && last_token <= interface_key::STRING_A123)
            {
                // Standard character
                search_string += last_token - ascii_to_enum_offset;
                filterDisplay();
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (search_string.length() > 0)
                {
                    search_string.erase(search_string.length()-1);
                    filterDisplay();
                }
            }
            else
            {
                return false;
            }

            return true;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool setHighlightByMouse()
    {
        if (gps->mouse_y >= 3 && gps->mouse_y < display_max_rows + 3 &&
            gps->mouse_x >= left_margin && gps->mouse_x < left_margin + max_item_width)
        {
            int new_index = display_start_offset + gps->mouse_y - 3;
            if (new_index < display_list.size())
                setHighlight(new_index);

            enabler->mouse_lbut = enabler->mouse_rbut = 0;

            return true;
        }

        return false;
    }

    void sort()
    {
        if (force_sort || list.size() < 100)
            std::sort(list.begin(), list.end(), 
                [] (ListEntry<T> const& a, ListEntry<T> const& b) { return a.text.compare(b.text) < 0; });

        filterDisplay();
    }


    private:
        vector<ListEntry<T>> list;
        vector<ListEntry<T>*> display_list;
        string search_string;
        int display_max_rows;
        int max_item_width;
};


/*
 * Material Choice Screen
 */

struct ItemFilter
{
    df::dfhack_material_category mat_mask;
    vector<MaterialInfo> materials;
    df::item_quality min_quality;
    bool decorated_only;

    ItemFilter() : min_quality(item_quality::Ordinary), decorated_only(false), valid(true)
    {    }

    bool matchesMask(MaterialInfo &mat)
    {
        return (mat_mask.whole) ? mat.matches(mat_mask) : true;
    }

    bool matches(const df::dfhack_material_category mask) const
    {
        return mask.whole & mat_mask.whole;
    }

    bool matches(MaterialInfo &material) const
    {
        return any_of(materials.begin(), materials.end(), 
            [&] (const MaterialInfo &m) { return material.matches(m); });
    }

    bool matches(df::item *item)
    {
        if (item->getQuality() < min_quality)
            return false;

        if (decorated_only && !item->hasImprovements())
            return false;

        auto imattype = item->getActualMaterial();
        auto imatindex = item->getActualMaterialIndex();
        auto item_mat = MaterialInfo(imattype, imatindex);

        return (materials.size() == 0) ? matchesMask(item_mat) : matches(item_mat);
    }

    vector<string> getMaterialFilterAsVector()
    {
        vector<string> descriptions;

        transform_(materials, descriptions,
            [] (MaterialInfo m) { return m.toString(); });

        if (descriptions.size() == 0)
            bitfield_to_string(&descriptions, mat_mask);

        if (descriptions.size() == 0)
            descriptions.push_back("any");

        return descriptions;
    }

    string getMaterialFilterAsSerial()
    {
        string str;

        str.append(bitfield_to_string(mat_mask, ","));
        str.append("/");
        if (materials.size() > 0)
        {
            for_each_(materials,
                [&] (MaterialInfo &m) { str.append(m.getToken() + ","); });

            if (str[str.size()-1] == ',')
                str.resize(str.size () - 1);
        }

        return str;
    }

    bool parseSerializedMaterialTokens(string str)
    {
        valid = false;
        vector<string> tokens;
        split_string(&tokens, str, "/");
               
        if (tokens.size() > 0 && !tokens[0].empty())
        {
            if (!parseJobMaterialCategory(&mat_mask, tokens[0]))
                return false;
        }

        if (tokens.size() > 1 && !tokens[1].empty())
        {
            vector<string> mat_names;
            split_string(&mat_names, tokens[1], ",");
            for (auto m = mat_names.begin(); m != mat_names.end(); m++)
            {
                MaterialInfo material;
                if (!material.find(*m) || !material.isValid())
                    return false;

                materials.push_back(material);
            }
        }

        valid = true;
        return true;
    }

    string getMinQuality()
    {
        return ENUM_KEY_STR(item_quality, min_quality);
    }

    bool isValid()
    {
        return valid;
    }

    void clear()
    {
        mat_mask.whole = 0;
        materials.clear();
    }

private:
    bool valid;
};


class ViewscreenChooseMaterial : public dfhack_viewscreen
{
public:
    ViewscreenChooseMaterial(ItemFilter *filter)
    {
        selected_column = 0;
        masks_column.title = "Type";
        masks_column.multiselect = true;
        masks_column.allow_search = false;
        masks_column.left_margin = 2;
        materials_column.left_margin = MAX_MASK + 3;
        materials_column.title = "Material";
        materials_column.multiselect = true;
        this->filter = filter;

        masks_column.changeHighlight(0);

        populateMasks();
        populateMaterials();

        masks_column.selectDefaultEntry();
        materials_column.selectDefaultEntry();
        materials_column.changeHighlight(0);
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed;
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
            masks_column.clear_selection();
            materials_column.clear_selection();
            populateMaterials();
        }
        else if  (input->count(interface_key::SEC_SELECT))
        {
            // Convert list selections to material filters


            filter->mat_mask.whole = 0;
            filter->materials.clear();

            // Category masks
            auto masks = masks_column.getSelectedElems();
            for_each_(masks,
                [&] (df::dfhack_material_category *m) { filter->mat_mask.whole |= m->whole; });

            // Specific materials
            auto materials = materials_column.getSelectedElems();
            transform_(materials, filter->materials,
                [] (MaterialInfo *m) { return *m; });

            Screen::dismiss(this);
        }
        else if  (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
            validateColumn();
        }
        else if  (input->count(interface_key::CURSOR_RIGHT))
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

    void render()
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
        OutputHotkeyString(x, y, "Toggle", "Enter");
        x += 3;
        OutputHotkeyString(x, y, "Save", "Shift-Enter");
        x += 3;
        OutputHotkeyString(x, y, "Clear", "C");
        x += 3;
        OutputHotkeyString(x, y, "Cancel", "Esc");
    }

    std::string getFocusString() { return "buildingplan_choosemat"; }

private:
    ListColumn<df::dfhack_material_category> masks_column;
    ListColumn<MaterialInfo> materials_column;
    int selected_column;
    ItemFilter *filter;

    df::building_type btype;

    void addMaskEntry(df::dfhack_material_category &mask, const string &text)
    {
        auto entry = ListEntry<df::dfhack_material_category>(pad_string(text, MAX_MASK, false), mask);
        if (filter->matches(mask))
            entry.selected = true;

        masks_column.add(entry);
        mask.whole = 0;
    }

    void populateMasks()
    {
        masks_column.clear();
        df::dfhack_material_category mask;

        mask.bits.stone = true;
        addMaskEntry(mask, "Stone");
        mask.bits.wood = true;
        addMaskEntry(mask, "Wood");
        mask.bits.metal = true;
        addMaskEntry(mask, "Metal");
        mask.bits.soap = true;
        addMaskEntry(mask, "Soap");

        masks_column.filterDisplay();
    }
    
    void populateMaterials()
    {
        materials_column.clear();
        df::dfhack_material_category selected_category;
        vector<df::dfhack_material_category *> selected_masks = masks_column.getSelectedElems();
        if (selected_masks.size() == 1)
            selected_category = *selected_masks[0];
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
                //string basename = p->name;

                /*MaterialInfo material;
                material.decode(p->material_defs.type_basic_mat, p->material_defs.idx_basic_mat);*/


                //addMaterialEntry(selected_category, material, material.toString());

                for (size_t j = 0; p->material.size() > 1 && j < p->material.size(); j++)
                {
                    auto t = p->material[j];
                    if (p->material[j]->id != "WOOD")
                        continue;

                    MaterialInfo material;
                    material.decode(DFHack::MaterialInfo::PLANT_BASE+j, i);
                    //addMaterialEntry(selected_category, material, material.toString());
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
                          MaterialInfo &material, string name)
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


// START Planning 
class PlannedBuilding
{
public:
    PlannedBuilding(df::building *building, ItemFilter *filter)
    {
        this->building = building;
        this->filter = *filter;
        pos = df::coord(building->centerx, building->centery, building->z);
        config = Core::getInstance().getWorld()->AddPersistentData("buildingplan/constraints");
        config.val() = filter->getMaterialFilterAsSerial();
        config.ival(1) = building->id;
        config.ival(2) = filter->min_quality + 1;
        config.ival(3) = static_cast<int>(filter->decorated_only) + 1;
    }

    PlannedBuilding(PersistentDataItem &config, color_ostream &out)
    {
        this->config = config;

        if (!filter.parseSerializedMaterialTokens(config.val()))
        {
            out.printerr("Buildingplan: Cannot parse filter: %s\nDiscarding.", config.val().c_str());
            return;
        }

        building = df::building::find(config.ival(1));
        pos = df::coord(building->centerx, building->centery, building->z);
        filter.min_quality = static_cast<df::item_quality>(config.ival(2) - 1);
        filter.decorated_only = config.ival(3) - 1;
    }

    df::building_type getType()
    {
        return building->getType();
    }

    bool assignClosestItem(vector<df::item *> *items_vector)
    {
        decltype(items_vector->begin()) closest_item;
        int32_t closest_distance = -1;
        for (auto item_iter = items_vector->begin(); item_iter != items_vector->end(); item_iter++)
        {
            auto item = *item_iter;
            if (!filter.matches(item))
                continue;

            auto pos = item->pos;
            auto distance = abs(pos.x - building->centerx) + 
                abs(pos.y - building->centery) + 
                abs(pos.z - building->z) * 50;

            if (closest_distance > -1 && distance >= closest_distance)
                continue;

            closest_distance = distance;
            closest_item = item_iter;
        }

        if (closest_distance > -1 && assignItem(*closest_item))
        {
            items_vector->erase(closest_item);
            remove();
            return true;
        }

        return false;
    }

    bool assignItem(df::item *item)
    {
        auto ref = df::allocate<df::general_ref_building_holderst>();
        if (!ref)
        {
            Core::printerr("Could not allocate general_ref_building_holderst\n");
            return false;
        }

        ref->building_id = building->id;

        auto job = building->jobs[0];
        delete job->job_items[0];
        job->job_items.clear();
        job->flags.bits.suspend = false;

        bool rough = false;
        Job::attachJobItem(job, item, df::job_item_ref::Hauled);
        if (item->getType() == item_type::BOULDER)
            rough = true;
        building->mat_type = item->getMaterial();
        building->mat_index = item->getMaterialIndex();

        job->mat_type = building->mat_type;
        job->mat_index = building->mat_index;

        if (building->needsDesign())
        {
            auto act = (df::building_actual *) building;
            act->design = new df::building_design();
            act->design->flags.bits.rough = rough;
        }

        return true;
    }

    bool isValid()
    {
        bool valid = filter.isValid() && 
            building && Buildings::findAtTile(pos) == building &&
            building->getBuildStage() == 0;

        if (!valid)
            remove();

        return valid;
    }

    bool isCurrentlySelectedBuilding()
    {
        return isValid() && (building == world->selected_building);
    }

    ItemFilter *getFilter()
    {
        return &filter;
    }

    void remove()
    {
        Core::getInstance().getWorld()->DeletePersistentData(config);
    }

private:
    df::building *building;
    PersistentDataItem config;
    df::coord pos;
    ItemFilter filter;
};


class Planner
{
public:
    bool isPlanableBuilding(const df::building_type type) const
    {
        return item_for_building_type.find(type) != item_for_building_type.end();
    }

    void reset(color_ostream &out)
    {
        planned_buildings.clear();
        auto pworld = Core::getInstance().getWorld();
        std::vector<PersistentDataItem> items;
        pworld->GetPersistentData(&items, "buildingplan/constraints");

        for (auto i = items.begin(); i != items.end(); i++)
        {
            PlannedBuilding pb(*i, out);
            if (pb.isValid())
                planned_buildings.push_back(pb);
        }
    }

    void initialize()
    {
        vector<string> item_names;
        typedef df::enum_traits<df::item_type> item_types;
        int size = item_types::last_item_value - item_types::first_item_value+1;
        for (size_t i = 1; i < size; i++)
        {
            is_relevant_item_type[(df::item_type) (i-1)] = false;
            string item_name = toLower(item_types::key_table[i]);
            string item_name_clean;
            for (auto c = item_name.begin(); c != item_name.end(); c++)
            {
                if (*c == '_')
                    continue;
                item_name_clean += *c;
            }
            item_names.push_back(item_name_clean);
        }
        
        typedef df::enum_traits<df::building_type> building_types;
        size = building_types::last_item_value - building_types::first_item_value+1;
        for (size_t i = 1; i < size; i++)
        {
            auto building_type = (df::building_type) (i-1);
            if (building_type == building_type::Weapon || building_type == building_type::Floodgate)
                continue;

            string building_name = toLower(building_types::key_table[i]);
            for (size_t j = 0; j < item_names.size(); j++)
            {
                if (building_name == item_names[j])
                {
                    auto btype = (df::building_type) (i-1);
                    auto itype = (df::item_type) j;

                    item_for_building_type[btype] = itype;
                    default_item_filters[btype] =  ItemFilter();
                    available_item_vectors[itype] = vector<df::item *>();
                    is_relevant_item_type[itype] = true;
                }
            }
        }
    }

    void addPlannedBuilding(df::building *bld)
    {
        PlannedBuilding pb(bld, &default_item_filters[bld->getType()]);
        planned_buildings.push_back(pb);
    }

    void doCycle()
    {
        if (planned_buildings.size() == 0)
            return;

        gather_available_items();
        for (auto building_iter = planned_buildings.begin(); building_iter != planned_buildings.end();)
        {
            if (building_iter->isValid())
            {
                auto required_item_type = item_for_building_type[building_iter->getType()];
                auto items_vector = &available_item_vectors[required_item_type];
                if (items_vector->size() == 0 || !building_iter->assignClosestItem(items_vector))
                {
                    ++building_iter;
                    continue;
                }
            }

            building_iter = planned_buildings.erase(building_iter);
        }
    }

    bool allocatePlannedBuilding(df::building_type type)
    {
        coord32_t cursor;
        if (!Gui::getCursorCoords(cursor.x, cursor.y, cursor.z))
            return false;

        auto newinst = Buildings::allocInstance(cursor.get_coord16(), type);
        if (!newinst)
            return false;

        df::job_item *filter = new df::job_item();
        filter->item_type = item_type::NONE;
        filter->mat_index = 0;
        filter->flags2.bits.building_material = true;
        std::vector<df::job_item*> filters;
        filters.push_back(filter);

        if (!Buildings::constructWithFilters(newinst, filters))
        {
            delete newinst;
            return false;
        }

        for (auto iter = newinst->jobs.begin(); iter != newinst->jobs.end(); iter++)
        {
            (*iter)->flags.bits.suspend = true;
        }

        addPlannedBuilding(newinst);

        return true;
    }

    PlannedBuilding *getSelectedPlannedBuilding()
    {
        for (auto building_iter = planned_buildings.begin(); building_iter != planned_buildings.end(); building_iter++)
        {
            if (building_iter->isCurrentlySelectedBuilding())
            {
                return &(*building_iter);
            }
        }

        return nullptr;
    }

    void removeSelectedPlannedBuilding()
    {
        getSelectedPlannedBuilding()->remove();
    }

    ItemFilter *getDefaultItemFilterForType(df::building_type type)
    {
        return &default_item_filters[type];
    }

    void cycleDefaultQuality(df::building_type type)
    {
        auto quality = &getDefaultItemFilterForType(type)->min_quality;
        *quality = static_cast<df::item_quality>(*quality + 1);
        if (*quality == item_quality::Artifact)
            (*quality) = item_quality::Ordinary;
    }


private:
    map<df::building_type, df::item_type> item_for_building_type;
    map<df::building_type, ItemFilter> default_item_filters;
    map<df::item_type, vector<df::item *>> available_item_vectors;
    map<df::item_type, bool> is_relevant_item_type; //Needed for fast check when looping over all items

    vector<PlannedBuilding> planned_buildings;

    void gather_available_items()
    {
        for (auto iter = available_item_vectors.begin(); iter != available_item_vectors.end(); iter++)
        {
            iter->second.clear();
        }

        // Precompute a bitmask with the bad flags
        df::item_flags bad_flags;
        bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact1);
#undef F

        std::vector<df::item*> &items = world->items.other[items_other_id::ANY_FREE];

        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            if (item->flags.whole & bad_flags.whole)
                continue;

            df::item_type itype = item->getType();
            if (!is_relevant_item_type[itype])
                continue;

            if (itype == item_type::BOX && item->isBag())
                continue; //Skip bags

            if (item->flags.bits.artifact1 || item->flags.bits.artifact2)
                continue;

            if (item->flags.bits.in_job ||
                item->isAssignedToStockpile() ||
                item->flags.bits.owned ||
                item->flags.bits.in_chest)
            {
                continue;
            }

            available_item_vectors[itype].push_back(item);
        }
    }
};

static Planner planner;


static map<df::building_type, bool> planmode_enabled;
static bool is_planmode_enabled(df::building_type type)
{
    if (planmode_enabled.find(type) == planmode_enabled.end())
    {
        planmode_enabled[type] = false;
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

    bool handleInput(set<df::interface_key> *input)
    {
        if (isInPlannedBuildingPlacementMode())
        {
            auto type = ui_build_selector->building_type;
            if (input->count(interface_key::CUSTOM_P))
            {
                planmode_enabled[type] = !planmode_enabled[type];
                if (!planmode_enabled[type])
                {
                    send_key(interface_key::CURSOR_DOWN_Z);
                    send_key(interface_key::CURSOR_UP_Z);
                }
                return true;
            }
            
            if (is_planmode_enabled(type))
            {
                if (input->count(interface_key::SELECT))
                {
                    if (ui_build_selector->errors.size() == 0 && planner.allocatePlannedBuilding(type))
                    {
                        send_key(interface_key::CURSOR_DOWN_Z);
                        send_key(interface_key::CURSOR_UP_Z);
                    }

                    return true;
                }
                else if (input->count(interface_key::CUSTOM_M))
                {
                    Screen::show(new ViewscreenChooseMaterial(planner.getDefaultItemFilterForType(type)));
                }
                else if (input->count(interface_key::CUSTOM_Q))
                {
                    planner.cycleDefaultQuality(type);
                }
                else if (input->count(interface_key::CUSTOM_D))
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
            int y = 23;

            OutputToggleString(x, y, "Planning Mode", "p", is_planmode_enabled(type), true, left_margin);

            if (is_planmode_enabled(type))
            {
                auto filter = planner.getDefaultItemFilterForType(type);

                OutputHotkeyString(x, y, "Min Quality: ", "q");
                OutputString(COLOR_BROWN, x, y, filter->getMinQuality(), true, left_margin);

                OutputHotkeyString(x, y, "Decorated Only: ", "d");
                OutputString(COLOR_BROWN, x, y, 
                    (filter->decorated_only) ? "Yes" : "No", true, left_margin);
                
                OutputHotkeyString(x, y, "Material Filter:", "m", true, left_margin);
                auto filter_descriptions = filter->getMaterialFilterAsVector();
                for_each_(filter_descriptions, 
                    [&](string d) { OutputString(COLOR_BROWN, x, y, "   *" + d, true, left_margin); });
            }
        }
        else if (isInPlannedBuildingQueryMode())
        {
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
            for_each_(filters,
                [&](string d) { OutputString(COLOR_BLUE, x, y, "*" + d, true, left_margin); });
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(buildingplan_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(buildingplan_hook, render);


static command_result buildingplan_cmd(color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
    {
        out << "Building Plan" << endl << "Version: " << PLUGIN_VERSION << endl;
    }

    return CR_OK;
}


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(buildingplan_hook, feed).apply() || !INTERPOSE_HOOK(buildingplan_hook, render).apply())
        out.printerr("Could not insert buildingplan hooks!\n");

    commands.push_back(
        PluginCommand(
        "buildingplan", "Place furniture before it's built",
        buildingplan_cmd, false, ""));

    planner.initialize();

    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        planner.reset(out);
        break;
    default:
        break;
    }

    return CR_OK;
}
