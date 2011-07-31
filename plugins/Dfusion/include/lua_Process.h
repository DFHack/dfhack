#ifndef LUA_PROCESS_H
#define LUA_PROCESS_H

#include <dfhack/Core.h>
#include <dfhack/Process.h>

#include "luamain.h"

namespace lua
{
void RegisterProcess(lua::state &st,DFHack::Process *p);
}
#endif