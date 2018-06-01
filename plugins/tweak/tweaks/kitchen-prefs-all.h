#include "modules/Kitchen.h"

#include "df/interface_key.h"
#include "df/layer_object_listst.h"
#include "df/viewscreen_kitchenprefst.h"

using namespace DFHack;

struct kitchen_prefs_all_hook : df::viewscreen_kitchenprefst {
    typedef df::viewscreen_kitchenprefst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        df::kitchen_pref_flag flag;
        df::kitchen_exc_type exc_type;
        if (input->count(interface_key::CUSTOM_SHIFT_C))
        {
            flag.bits.Cook = true;
            exc_type = df::kitchen_exc_type::Cook;
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_B))
        {
            flag.bits.Brew = true;
            exc_type = df::kitchen_exc_type::Brew;
        }

        if (flag.whole && size_t(cursor) < forbidden[page].size())
        {
            bool was_forbidden = forbidden[page][cursor].whole & flag.whole;
            for (size_t i = 0; i < forbidden[page].size(); i++)
            {
                if (possible[page][i].whole & flag.whole)
                {
                    if (was_forbidden)
                    {
                        // unset flag
                        forbidden[page][i].whole &= ~flag.whole;
                        Kitchen::removeExclusion(exc_type,
                            item_type[page][i], item_subtype[page][i],
                            mat_type[page][i], mat_index[page][i]);
                    }
                    else
                    {
                        // set flag
                        forbidden[page][i].whole |= flag.whole;
                        Kitchen::addExclusion(exc_type,
                            item_type[page][i], item_subtype[page][i],
                            mat_type[page][i], mat_index[page][i]);
                    }
                }
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        int x = 2, y = gps->dimy - 2;
        OutputHotkeyString(x, y, "Cook all", interface_key::CUSTOM_SHIFT_C,
            false, 0, COLOR_WHITE, COLOR_LIGHTRED);
        x = 20;
        OutputHotkeyString(x, y, "Brew all", interface_key::CUSTOM_SHIFT_B,
            false, 0, COLOR_WHITE, COLOR_LIGHTRED);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(kitchen_prefs_all_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(kitchen_prefs_all_hook, feed);
