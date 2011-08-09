#ifndef LUA_VERSIONINFO_H
#define LUA_VERSIONINFO_H
#include <dfhack/Core.h>
#include <dfhack/VersionInfo.h>
#include "luamain.h"
namespace lua
{
class OffsetGroup
{
    int tblid;
    DFHack::OffsetGroup *p;
public:
    int GetTableId(){return tblid;};

    //OffsetGroup(lua_State *L,DFHack::OffsetGroup *p); //constructor from c++
    OffsetGroup(lua_State *L,int id); //constructor from lua

    int getOffset(lua_State *L);
    int getAddress(lua_State *L);
    int getHexValue(lua_State *L);
    int getString(lua_State *L);
    int getGroup(lua_State *L);

    int getSafeOffset(lua_State *L);
    int getSafeAddress(lua_State *L);

    int PrintOffsets(lua_State *L);
    int getName(lua_State *L);
    int getFullName(lua_State *L);
    int getParent(lua_State *L);

    DEF_LUNE(OffsetGroup);
};
void RegisterVersionInfo(lua::state &st);

}
#endif
