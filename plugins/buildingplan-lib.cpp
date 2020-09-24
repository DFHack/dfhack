#include "buildingplan-lib.h"

#include "Core.h"

using namespace DFHack;

bool show_debugging = false;

void debug(const std::string &msg)
{
    if (!show_debugging)
        return;

    color_ostream_proxy out(Core::getInstance().getConsole());
    out << "DEBUG: " << msg << endl;
}
