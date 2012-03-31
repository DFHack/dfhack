#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H
#include <Console.h>
#include "luamain.h"

namespace lua
{

void RegisterConsole(lua::state &st);

}

#endif