#include "df/viewscreen_dwarfmodest.h"

using namespace df::enums;
using df::global::ui;

struct shift_8_scroll_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key>* input))
    {
        if (ui->main.mode != ui_sidebar_mode::Default &&
            input->count(interface_key::CURSOR_UP_FAST) &&
            input->count(interface_key::SECONDSCROLL_PAGEDOWN)
            )
        {
            input->erase(interface_key::CURSOR_UP_FAST);
        }
        INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(shift_8_scroll_hook, feed);
