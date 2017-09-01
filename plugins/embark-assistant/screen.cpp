#include "screen.h"

namespace embark_assist {
    namespace screen {
    }
}

bool embark_assist::screen::paintString(const DFHack::Screen::Pen &pen, int x, int y, const std::string &text, bool map) {
    auto screen_size = DFHack::Screen::getWindowSize();

    if (y < 1 || y + 1 >= screen_size.y || x < 1)
    {
        return false;  //  Won't paint outside of the screen or on the frame
    }

    if (x + text.length() - 1 < screen_size.x - 2) {
        DFHack::Screen::paintString(pen, x, y, text, map);
    }
    else if (x < screen_size.x - 2) {
        DFHack::Screen::paintString(pen, x, y, text.substr(0, screen_size.x - 2 - x + 1), map);
    }
    else {
        return false;
    }

    return true;
}
