#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H
#include <Console.h>
#include "luamain.h"

namespace lua
{

void RegisterConsole(lua::state &st);
void SetConsole(lua::state &st,DFHack::color_ostream& stream);
}

#endif