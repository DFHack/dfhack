#include "modules/Gui.h"
#include "modules/Screen.h"
#include "df/building_cagest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/ui_sidebar_mode.h"
#include "df/unit.h"

using namespace DFHack;
using df::global::ui;
using df::global::ui_building_in_assign;
using df::global::ui_building_in_resize;
using df::global::ui_building_item_cursor;

struct cage_butcher_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    inline df::building_cagest *get_cage()
    {
        if (*ui_building_in_assign || *ui_building_in_resize)
            return nullptr;

        if (ui->main.mode != df::ui_sidebar_mode::QueryBuilding)
            return nullptr;

        auto cage = virtual_cast<df::building_cagest>(Gui::getAnyBuilding(this));
        if (!cage)
            return nullptr;
        if (cage->getBuildStage() < cage->getMaxBuildStage())
            return nullptr;
        if (cage->flags.bits.justice)
            return nullptr;
        if (Buildings::markedForRemoval(cage))
            return nullptr;

        return cage;
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        using namespace df::enums::interface_key;
        INTERPOSE_NEXT(render)();

        auto cage = get_cage();
        if (!cage)
            return;

        std::vector<df::unit*> units;
        if (!Buildings::getCageOccupants(cage, units))
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        for (int y = 4, i = (*ui_building_item_cursor/11)*11;
            y <= 14 && i < units.size();
            ++y, ++i)
        {
            df::unit *unit = vector_get(units, i);
            if (unit && unit->flags2.bits.slaughter)
            {
                int x = dims.menu_x2 - 2;
                OutputString(COLOR_LIGHTMAGENTA, x, y, "Bu");
            }
        }

        int x = dims.menu_x1 + 1, y = dims.y2;
        OutputHotkeyString(x, y, "Butcher ", CUSTOM_B, false, 0, COLOR_WHITE, COLOR_LIGHTRED);
        OutputHotkeyString(x, y, "all", CUSTOM_SHIFT_B, false, 0, COLOR_WHITE, COLOR_LIGHTRED);
    }
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        using namespace df::enums::interface_key;
        auto cage = get_cage();
        if (cage)
        {
            std::vector<df::unit*> units;
            if (Buildings::getCageOccupants(cage, units))
            {
                df::unit *unit = vector_get(units, *ui_building_item_cursor);
                if (unit)
                {
                    if (input->count(CUSTOM_B))
                    {
                        unit->flags2.bits.slaughter = !unit->flags2.bits.slaughter;
                    }
                }
                if (input->count(CUSTOM_SHIFT_B))
                {
                    bool state = unit ? !unit->flags2.bits.slaughter : true;
                    for (auto u : units)
                        u->flags2.bits.slaughter = state;
                }
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(cage_butcher_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(cage_butcher_hook, render);
