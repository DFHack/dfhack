#include "df/meeting_event.h"
#include "df/viewscreen_entityst.h"

using namespace std;
using namespace df::enums;
using namespace DFHack::Gui;
using namespace DFHack::Screen;

#define DLOG DFHack::Core::getInstance().getConsole().printerr

struct civ_agreement_view_hook : df::viewscreen_entityst {
    typedef df::viewscreen_entityst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (page == 2)
        {
            fillRect(Pen(0, 0, ' '), 2, 22, 22, 22);
            int x, y;
            getWindowSize(x, y);
            x = 2;
            y -= 3;
            OutputString(COLOR_LIGHTGREEN, x, y, getKeyDisplay(interface_key::CHANGETAB));
            OutputString(COLOR_WHITE, x, y, " to change modes.");
            x = 2;
            y++;
            OutputString(COLOR_LIGHTGREEN, x, y, getKeyDisplay(interface_key::SELECT));
            OutputString(COLOR_WHITE, x, y, ": View agreement");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(civ_agreement_view_hook, render);

