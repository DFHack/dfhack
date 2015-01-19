#include "df/building_nest_boxst.h"
#include "df/item_eggst.h"
#include "df/viewscreen_dwarfmodest.h"

using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

struct egg_fertile_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    df::building_nest_boxst* getNestBox()
    {
        if (ui->main.mode != ui_sidebar_mode::QueryBuilding &&
            ui->main.mode != ui_sidebar_mode::BuildingItems)
            return NULL;
        return virtual_cast<df::building_nest_boxst>(world->selected_building);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        df::building_nest_boxst* nest_box = getNestBox();
        if (nest_box)
        {
            auto dims = Gui::getDwarfmodeViewDims();
            bool has_eggs = false;
            bool fertile = false;
            int idx = 0;
            for (auto iter = nest_box->contained_items.begin();
                 iter != nest_box->contained_items.end(); ++iter)
            {
                df::item_eggst* egg = virtual_cast<df::item_eggst>((*iter)->item);
                if (egg)
                {
                    has_eggs = true;
                    if (egg->egg_flags.bits.fertile)
                        fertile = true;
                    if (ui->main.mode == ui_sidebar_mode::BuildingItems)
                    {
                        Screen::paintString(
                            Screen::Pen(' ', fertile ? COLOR_LIGHTGREEN : COLOR_LIGHTRED),
                            dims.menu_x2 - (fertile ? 4 : 6),
                            dims.y1 + idx + 3,
                            fertile ? "Fert" : "N.Fert"
                        );
                    }
                }
                ++idx;
            }
            if (has_eggs && ui->main.mode == ui_sidebar_mode::QueryBuilding)
            {
                Screen::paintString(
                    Screen::Pen(' ', fertile ? COLOR_LIGHTGREEN : COLOR_LIGHTRED),
                    dims.menu_x1 + 1,
                    dims.y1 + 5,
                    fertile ? "Eggs Fertile" : "Eggs infertile"
                );
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(egg_fertile_hook, render);
