#include "df/interface_key.h"
#include "df/layer_object_listst.h"
#include "df/viewscreen_layer_stone_restrictionst.h"

using namespace DFHack;

struct stone_status_all_hook : df::viewscreen_layer_stone_restrictionst {
    typedef df::viewscreen_layer_stone_restrictionst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        if (input->count(interface_key::SELECT_ALL))
        {
            if (VIRTUAL_CAST_VAR(list, df::layer_object_listst, layer_objects[0]))
            {
                bool new_state = !*stone_economic[type_tab][list->cursor];
                for (bool *economic : stone_economic[type_tab])
                    *economic = new_state;
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        int x = 2, y = 23;
        OutputHotkeyString(x, y, "All", interface_key::SELECT_ALL,
            false, 0, COLOR_WHITE, COLOR_LIGHTRED);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stone_status_all_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(stone_status_all_hook, feed);
