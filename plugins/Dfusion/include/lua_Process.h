#ifndef LUA_PROCESS_H
#define LUA_PROCESS_H

#include "Core.h"
#include <MemAccess.h>

#include "luamain.h"

namespace lua
{
void RegisterProcess(lua::state &st);
}
#endif