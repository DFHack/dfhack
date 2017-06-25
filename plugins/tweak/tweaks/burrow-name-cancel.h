#include "df/burrow.h"

using df::global::ui;

struct burrow_name_cancel_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    static std::string old_name;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        if (ui->main.mode == df::ui_sidebar_mode::Burrows)
        {
            bool was_naming = ui->burrows.in_edit_name_mode;
            INTERPOSE_NEXT(feed)(input);
            df::burrow *burrow = vector_get(ui->burrows.list, ui->burrows.sel_index);
            if (!burrow)
                return;

            if (ui->burrows.in_edit_name_mode)
            {
                if (!was_naming)
                {
                    // Just started renaming - make a copy of the old name
                    old_name = burrow->name;
                }
                if (input->count(df::interface_key::LEAVESCREEN))
                {
                    // Cancel and restore the old name
                    ui->burrows.in_edit_name_mode = false;
                    burrow->name = old_name;
                }
            }
        }
        else
        {
            INTERPOSE_NEXT(feed)(input);
        }
    }
};

std::string burrow_name_cancel_hook::old_name;

IMPLEMENT_VMETHOD_INTERPOSE(burrow_name_cancel_hook, feed);
