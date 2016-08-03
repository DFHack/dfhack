using namespace DFHack;
using namespace df::enums;

using df::global::ui_sidebar_menus;
using df::global::ui_workshop_in_add;

static df::interface_key kitchen_bindings[] = {
    df::interface_key::HOTKEY_KITCHEN_COOK_2,
    df::interface_key::HOTKEY_KITCHEN_COOK_3,
    df::interface_key::HOTKEY_KITCHEN_COOK_4,
    // DF uses CUSTOM_R for this reaction in the raws, so this key is recognized
    // by this tweak but not displayed
    df::interface_key::HOTKEY_KITCHEN_RENDER_FAT
};

struct kitchen_keys_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    void draw_binding (int row, df::interface_key key)
    {
        std::string label = Screen::getKeyDisplay(key);
        int x = Gui::getDwarfmodeViewDims().menu_x2 - 2 - label.size();
        int y = row + 4;
        OutputString(COLOR_GREY, x, y, "(");
        OutputString(COLOR_LIGHTRED, x, y, label);
        OutputString(COLOR_GREY, x, y, ")");
    }

    bool kitchen_in_add()
    {
        if (!*ui_workshop_in_add)
            return false;
        df::building_workshopst *ws = virtual_cast<df::building_workshopst>(world->selected_building);
        if (!ws)
            return false;
        if (ws->type != workshop_type::Kitchen)
            return false;
        return true;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        if (kitchen_in_add())
        {
            for (int i = 0; i < 4; i++)
            {
                if (input->count(kitchen_bindings[i]))
                {
                    ui_sidebar_menus->workshop_job.cursor = i;
                    input->clear();
                    input->insert(df::interface_key::SELECT);
                }
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (kitchen_in_add())
            for (int i = 0; i < 3; i++)
                draw_binding(i, kitchen_bindings[i]);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(kitchen_keys_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(kitchen_keys_hook, render);
