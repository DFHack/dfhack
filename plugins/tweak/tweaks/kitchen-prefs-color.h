using namespace DFHack;
using df::global::gps;

struct kitchen_prefs_color_hook : df::viewscreen_kitchenprefst {
    typedef df::viewscreen_kitchenprefst interpose_base;

    static std::string read_string(unsigned x, unsigned y, size_t length)
    {
        std::string s;
        for ( ; length; x++, length--)
        {
            auto tile = Screen::readTile(x, y);
            if (!tile.valid())
                break;
            s += tile.ch;
        }
        return s;
    }

    static void recolor(unsigned x, unsigned y, std::string str,
        UIColor old_color, UIColor new_color)
    {
        static Screen::Pen tile;
        for (unsigned i = 0; i < str.size(); i++)
        {
            tile = Screen::readTile(x + i, y);
            if (!tile.valid() || tile.ch != str[i] || tile.fg != old_color)
                return;
        }
        for (unsigned i = 0; i < str.size(); i++)
        {
            tile = Screen::readTile(x + i, y);
            tile.fg = new_color;
            Screen::paintTile(tile, x + i, y);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        /* DF uses a slightly complicated algorithm to position the Number and
           Permissions columns (i.e. not a constant distance from either margin).
           Detecting light blue "Cook" and "Brew" strings should be sufficient,
           but reverse-engineering the algorithm could be slightly faster. */

        if (!item_type[page].size())
           return;

        int start_x = 0,
            start_y = 6;
        for (int x = 0; x < gps->dimx; x++)
        {
            std::string word = read_string(x, start_y, 4);
            if (word == "Cook" || word == "----")
            {
                start_x = x;
                break;
            }
        }
        if (!start_x)
            return;

        for (int y = start_y; y < gps->dimy; y++)
        {
            recolor(start_x, y, "Cook", COLOR_BLUE, COLOR_GREEN);
            recolor(start_x + 5, y, "Brew", COLOR_BLUE, COLOR_GREEN);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(kitchen_prefs_color_hook, render);
