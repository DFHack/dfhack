#include "modules/Gui.h"
#include "df/viewscreen_dwarfmodest.h"

using namespace DFHack;
using df::global::gps;
using df::global::ui_sidebar_menus;

struct hide_priority_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;
    inline bool valid_mode ()
    {
        switch (ui->main.mode)
        {
        case df::ui_sidebar_mode::DesignateMine:
        case df::ui_sidebar_mode::DesignateRemoveRamps:
        case df::ui_sidebar_mode::DesignateUpStair:
        case df::ui_sidebar_mode::DesignateDownStair:
        case df::ui_sidebar_mode::DesignateUpDownStair:
        case df::ui_sidebar_mode::DesignateUpRamp:
        case df::ui_sidebar_mode::DesignateChannel:
        case df::ui_sidebar_mode::DesignateGatherPlants:
        case df::ui_sidebar_mode::DesignateRemoveDesignation:
        case df::ui_sidebar_mode::DesignateSmooth:
        case df::ui_sidebar_mode::DesignateCarveTrack:
        case df::ui_sidebar_mode::DesignateEngrave:
        case df::ui_sidebar_mode::DesignateCarveFortification:
        case df::ui_sidebar_mode::DesignateChopTrees:
        case df::ui_sidebar_mode::DesignateToggleEngravings:
        case df::ui_sidebar_mode::DesignateToggleMarker:
        case df::ui_sidebar_mode::DesignateRemoveConstruction:
            return true;
        default:
            return false;
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (valid_mode())
        {
            auto dims = Gui::getDwarfmodeViewDims();
            if (dims.menu_on)
            {
                int x = dims.menu_x1 + 1, y = gps->dimy - (gps->dimy > 26 ? 8 : 7);
                OutputToggleString(x, y, "Show priorities", df::interface_key::CUSTOM_ALT_P,
                    ui_sidebar_menus->designation.priority_set, true, 0,
                    COLOR_WHITE, COLOR_LIGHTRED);
            }
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        if (valid_mode() && input->count(df::interface_key::CUSTOM_ALT_P))
            ui_sidebar_menus->designation.priority_set = !ui_sidebar_menus->designation.priority_set;
        else
            INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(hide_priority_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(hide_priority_hook, render);
