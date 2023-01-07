#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Units.h"
#include "df/ui_unit_view_mode.h"
#include "df/unit_labor.h"
#include "df/viewscreen_dwarfmodest.h"

using namespace DFHack;
using df::global::plotinfo;
using df::global::ui_look_cursor;
using df::global::ui_unit_view_mode;

struct block_labors_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    inline bool valid_mode()
    {
        return plotinfo->main.mode == df::ui_sidebar_mode::ViewUnits &&
            ui_unit_view_mode->value == df::ui_unit_view_mode::T_value::PrefLabor &&
            Gui::getAnyUnit(this);
    }

    inline bool forbidden_labor (df::unit *unit, df::unit_labor labor)
    {
        return is_valid_enum_item(labor) && unit && !Units::isValidLabor(unit, labor);
    }

    inline bool all_labors_enabled (df::unit *unit, df::unit_labor_category cat)
    {
        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (ENUM_ATTR(unit_labor, category, labor) == cat &&
                    !unit->status.labors[labor] &&
                    !forbidden_labor(unit, labor))
                return false;
        }
        return true;
    }

    inline void recolor_line (int x1, int x2, int y, UIColor color)
    {
        for (int x = x1; x <= x2; x++)
        {
            auto tile = Screen::readTile(x, y);
            tile.fg = color;
            tile.bold = false;
            Screen::paintTile(tile, x, y);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        auto dims = Gui::getDwarfmodeViewDims();
        if (valid_mode())
        {
            df::unit *unit = Gui::getAnyUnit(this);

            for (int y = 5, i = (*ui_look_cursor/13)*13;
                y <= 17 && size_t(i) < unit_labors_sidemenu.size();
                ++y, ++i)
            {
                df::unit_labor labor = unit_labors_sidemenu[i];
                df::unit_labor_category cat = df::unit_labor_category(labor);

                if (is_valid_enum_item(cat) && all_labors_enabled(unit, cat))
                    recolor_line(dims.menu_x1, dims.menu_x2, y, COLOR_WHITE);

                if (forbidden_labor(unit, labor))
                    recolor_line(dims.menu_x1, dims.menu_x2, y, COLOR_RED +
                        (unit->status.labors[labor] ? 8 : 0));
            }
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        using namespace df::enums::interface_key;
        df::unit *unit = Gui::getAnyUnit(this);
        df::unit_labor labor = vector_get(unit_labors_sidemenu, *ui_look_cursor, df::unit_labor::NONE);
        df::unit_labor_category cat = df::unit_labor_category(labor);

        if (valid_mode() && labor != df::unit_labor::NONE)
        {
            if ((input->count(SELECT) || input->count(SELECT_ALL)) && forbidden_labor(unit, labor))
            {
                unit->status.labors[labor] = false;
                return;
            }
            else if (input->count(SELECT_ALL) && is_valid_enum_item(cat))
            {
                bool new_state = !all_labors_enabled(unit, cat);
                FOR_ENUM_ITEMS(unit_labor, labor)
                {
                    if (ENUM_ATTR(unit_labor, category, labor) == cat)
                        unit->status.labors[labor] = (new_state && !forbidden_labor(unit, labor));
                }
                return;
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(block_labors_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(block_labors_hook, render);
