#include "buildingplan-lib.h"

#include <cstdarg>
#include "Core.h"

using namespace DFHack;

bool show_debugging = false;

void debug(const char *fmt, ...)
{
    if (!show_debugging)
        return;

    color_ostream_proxy out(Core::getInstance().getConsole());
    out.print("DEBUG(buildingplan): ");
    va_list args;
    va_start(args, fmt);
    out.vprint(fmt, args);
    va_end(args);
    out.print("\n");
}
