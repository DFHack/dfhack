#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H
#include <dfhack/Console.h>
#include "luamain.h"

namespace lua
{

void RegisterConsole(lua::state &st, DFHack::Console *c);

}

#endif