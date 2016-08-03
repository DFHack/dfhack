using namespace DFHack;
using df::global::gps;

struct kitchen_prefs_empty_hook : df::viewscreen_kitchenprefst {
    typedef df::viewscreen_kitchenprefst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        static const char *state_names[] = {
            "Vegetables/fruit/leaves",
            "Seeds",
            "Drinks",
            "Meat/fish/other"
        };
        static int tab_x[] = {2, 30, 45, 60};
        static Screen::Pen pen(' ', COLOR_WHITE, COLOR_BLACK);

        INTERPOSE_NEXT(render)();
        for (int x = 1; x < gps->dimx - 2; x++)
            Screen::paintTile(pen, x, 2);
        for (int i = 0; i < 4; i++)
        {
            pen.bold = (page == i);
            Screen::paintString(pen, tab_x[i], 2, state_names[i]);
        }
        if (!item_type[page].size())
        {
            pen.bold = true;
            Screen::paintString(pen, 2, 4, "You have no appropriate ingredients.");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(kitchen_prefs_empty_hook, render);
