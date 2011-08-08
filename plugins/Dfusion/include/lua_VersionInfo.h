#ifndef LUA_VERSIONINFO_H
#define LUA_VERSIONINFO_H
#include <dfhack/Core.h>
#include <dfhack/VersionInfo.h>
#include "luamain.h"
namespace lua
{

void RegisterVersionInfo(lua::state &st);

}
#endif
