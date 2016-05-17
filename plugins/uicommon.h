#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <map>
#include <string>
#include <set>

#include "Core.h"
#include "MiscUtils.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>

#include "modules/Items.h"
#include "modules/Screen.h"
#include "modules/World.h"

#include "df/building_stockpilest.h"
#include "df/caravan_state.h"
#include "df/dfhack_material_category.h"
#include "df/enabler.h"
#include "df/item_quality.h"
#include "df/ui.h"
#include "df/world.h"

using namespace std;
using std::string;
using std::vector;
using std::map;
using std::ostringstream;
using std::set;

using namespace DFHack;
using namespace df::enums;

#ifndef HAVE_NULLPTR
#define nullptr 0L
#endif

#define COLOR_TITLE COLOR_BROWN
#define COLOR_UNSELECTED COLOR_GREY
#define COLOR_SELECTED COLOR_WHITE
#define COLOR_HIGHLIGHTED COLOR_GREEN

struct coord32_t
{
    int32_t x, y, z;

    coord32_t()
    {
        x = -30000;
        y = -30000;
        z = -30000;
    }

    coord32_t(df::coord& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    df::coord get_coord16() const
    {
        return df::coord(x, y, z);
    }
};

template <class T, typename Fn>
static void for_each_(vector<T> &v, Fn func)
{
    for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn>
static void for_each_(map<T, V> &v, Fn func)
{
    for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn>
static void transform_(vector<T> &src, vector<V> &dst, Fn func)
{
    transform(src.begin(), src.end(), back_inserter(dst), func);
}

typedef int8_t UIColor;

static void OutputString(UIColor color, int &x, int &y, const std::string &text,
    bool newline = false, int left_margin = 0, const UIColor bg_color = 0, bool map = false)
{
    Screen::paintString(Screen::Pen(' ', color, bg_color), x, y, text, map);
    if (newline)
    {
        ++y;
        x = left_margin;
    }
    else
        x += text.length();
}

static void OutputHotkeyString(int &x, int &y, const char *text, const char *hotkey, bool newline = false,
    int left_margin = 0, int8_t text_color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputString(hotkey_color, x, y, hotkey, false, 0, 0, map);
    string display(": ");
    display.append(text);
    OutputString(text_color, x, y, display, newline, left_margin, 0, map);
}

static void OutputHotkeyString(int &x, int &y, const char *text, df::interface_key hotkey,
    bool newline = false, int left_margin = 0, int8_t text_color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN,
    bool map = false)
{
    OutputHotkeyString(x, y, text, DFHack::Screen::getKeyDisplay(hotkey).c_str(), newline, left_margin, text_color, hotkey_color, map);
}

static void OutputLabelString(int &x, int &y, const char *text, const char *hotkey, const string &label, bool newline = false,
    int left_margin = 0, int8_t text_color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputString(hotkey_color, x, y, hotkey, false, 0, 0, map);
    string display(": ");
    display.append(text);
    display.append(": ");
    OutputString(text_color, x, y, display, false, 0, 0, map);
    OutputString(hotkey_color, x, y, label, newline, left_margin, 0, map);
}

static void OutputLabelString(int &x, int &y, const char *text, df::interface_key hotkey, const string &label, bool newline = false,
    int left_margin = 0, int8_t text_color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputLabelString(x, y, text, DFHack::Screen::getKeyDisplay(hotkey).c_str(), label, newline,
        left_margin, text_color, hotkey_color, map);
}

static void OutputFilterString(int &x, int &y, const char *text, const char *hotkey, bool state, bool newline = false,
    int left_margin = 0, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputString(hotkey_color, x, y, hotkey, false, 0, 0, map);
    OutputString(COLOR_WHITE, x, y, ": ", false, 0, 0, map);
    OutputString((state) ? COLOR_WHITE : COLOR_GREY, x, y, text, newline, left_margin, 0, map);
}

static void OutputToggleString(int &x, int &y, const char *text, const char *hotkey, bool state, bool newline = true,
    int left_margin = 0, int8_t color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputHotkeyString(x, y, text, hotkey, false, 0, color, hotkey_color, map);
    OutputString(color, x, y, ": ", false, 0, 0, map);
    if (state)
        OutputString(COLOR_GREEN, x, y, "On", newline, left_margin, 0, map);
    else
        OutputString(COLOR_GREY, x, y, "Off", newline, left_margin, 0, map);
}

static void OutputToggleString(int &x, int &y, const char *text, df::interface_key hotkey, bool state, bool newline = true,
    int left_margin = 0, int8_t color = COLOR_WHITE, int8_t hotkey_color = COLOR_LIGHTGREEN, bool map = false)
{
    OutputToggleString(x, y, text, DFHack::Screen::getKeyDisplay(hotkey).c_str(), state, newline, left_margin, color, hotkey_color, map);
}

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

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
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

static df::interface_key get_string_key(const std::set<df::interface_key> *input)
{
    for (auto it = input->begin(); it != input->end(); ++it)
    {
        if (DFHack::Screen::keyToChar(*it) >= 0)
            return *it;
    }
    return df::interface_key::NONE;
}

static char get_string_input(const std::set<df::interface_key> *input)
{
    return DFHack::Screen::keyToChar(get_string_key(input));
}

/*
 * Utility Functions
 */

static df::building_stockpilest *get_selected_stockpile()
{
    if (!Gui::dwarfmode_hotkey(Core::getTopViewscreen()) ||
        df::global::ui->main.mode != ui_sidebar_mode::QueryBuilding)
    {
        return nullptr;
    }

    return virtual_cast<df::building_stockpilest>(df::global::world->selected_building);
}

static bool can_trade()
{
    if (df::global::ui->caravans.size() == 0)
        return false;

    for (auto it = df::global::ui->caravans.begin(); it != df::global::ui->caravans.end(); it++)
    {
        typedef df::caravan_state::T_trade_state state;
        auto caravan = *it;
        auto trade_state = caravan->trade_state;
        auto time_remaining = caravan->time_remaining;
        if ((trade_state == state::Approaching || trade_state == state::AtDepot) && time_remaining != 0)
            return true;
    }

    return false;
}

static bool is_metal_item(df::item *item)
{
    MaterialInfo mat(item);
    return (mat.getCraftClass() == craft_material_class::Metal);
}

static bool is_set_to_melt(df::item* item)
{
    return item->flags.bits.melt;
}

// Copied from Kelly Martin's code
static bool can_melt(df::item* item)
{

    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect); F(in_job);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact); F(melt);
#undef F

    if (item->flags.whole & bad_flags.whole)
        return false;

    df::item_type t = item->getType();

    if (t == df::enums::item_type::BOX || t == df::enums::item_type::BAR)
        return false;

    if (!is_metal_item(item)) return false;

    for (auto g = item->general_refs.begin(); g != item->general_refs.end(); g++)
    {
        switch ((*g)->getType())
        {
        case general_ref_type::CONTAINS_ITEM:
        case general_ref_type::UNIT_HOLDER:
        case general_ref_type::CONTAINS_UNIT:
            return false;
        case general_ref_type::CONTAINED_IN_ITEM:
            {
                df::item* c = (*g)->getItem();
                for (auto gg = c->general_refs.begin(); gg != c->general_refs.end(); gg++)
                {
                    if ((*gg)->getType() == general_ref_type::UNIT_HOLDER)
                        return false;
                }
            }
            break;
        }
    }

    if (item->getQuality() >= item_quality::Masterful)
        return false;

    return true;
}


/*
 * Stockpile Access
 */

class StockpileInfo {
public:
    StockpileInfo() : id(0), sp(nullptr)
    {
    }

    StockpileInfo(df::building_stockpilest *sp_) : sp(sp_)
    {
        readBuilding();
    }

    bool inStockpile(df::item *i)
    {
        df::item *container = Items::getContainer(i);
        if (container)
            return inStockpile(container);

        if (i->pos.z != z) return false;
        if (i->pos.x < x1 || i->pos.x >= x2 ||
            i->pos.y < y1 || i->pos.y >= y2) return false;
        int e = (i->pos.x - x1) + (i->pos.y - y1) * sp->room.width;
        return sp->room.extents[e] == 1;
    }

    bool isValid()
    {
        if (!id)
            return false;

        auto found = df::building::find(id);
        return found && found == sp && found->getType() == building_type::Stockpile;
    }

    int32_t getId()
    {
        return id;
    }

    bool matches(df::building_stockpilest* sp)
    {
        return this->sp == sp;
    }

    df::building_stockpilest* getStockpile()
    {
        return sp;
    }

protected:
    int32_t id;
    df::building_stockpilest* sp;

    void readBuilding()
    {
        if (!sp)
            return;

        id = sp->id;
        z = sp->z;
        x1 = sp->room.x;
        x2 = sp->room.x + sp->room.width;
        y1 = sp->room.y;
        y2 = sp->room.y + sp->room.height;
    }

protected:
    int x1, x2, y1, y2, z;
};

class PersistentStockpileInfo : public StockpileInfo {
public:
    PersistentStockpileInfo(df::building_stockpilest *sp, string persistence_key) :
      StockpileInfo(sp), persistence_key(persistence_key)
    {
    }

    PersistentStockpileInfo(PersistentDataItem &config, string persistence_key) :
        config(config), persistence_key(persistence_key)
    {
        id = config.ival(1);
    }

    bool load()
    {
        auto found = df::building::find(id);
        if (!found || found->getType() != building_type::Stockpile)
            return false;

        sp = virtual_cast<df::building_stockpilest>(found);
        if (!sp)
            return false;

        readBuilding();

        return true;
    }

    void save()
    {
        config = DFHack::World::AddPersistentData(persistence_key);
        config.ival(1) = id;
    }

    void remove()
    {
        DFHack::World::DeletePersistentData(config);
    }

protected:
    PersistentDataItem config;
    string persistence_key;
};

