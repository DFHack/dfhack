#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include <vector>
#include <string>

#include "luamain.h"
#include "lua_Process.h"
#include "lua_Hexsearch.h"
#include "lua_Misc.h"

#include "DataDefs.h"
#include "LuaTools.h"

using std::vector;
using std::string;
using namespace DFHack;



DFHACK_PLUGIN("dfusion")

static int loadObjectFile(lua_State* L)
{
    std::string path;

    path=luaL_checkstring(L,1);

    OutFile::File f(path);
    lua_newtable(L);
    int table_pos=lua_gettop(L);
    size_t size=f.GetTextSize();
    Lua::Push(L,size);
    lua_setfield(L,table_pos,"data_size");
    char* buf=new char[size];
    f.GetText(buf);

    //Lua::PushDFObject(L,DFHack::,buf);
    //Lua::Push(L,buf);
    lua_pushlightuserdata(L,buf);
    lua_setfield(L,table_pos,"data");
    const OutFile::vSymbol &symbols=f.GetSymbols();
    lua_newtable(L);
    for(size_t i=0;i<symbols.size();i++)
    {
        Lua::Push(L,i);
        lua_newtable(L);
        Lua::Push(L,symbols[i].name);
        lua_setfield(L,-2,"name");
        Lua::Push(L,symbols[i].pos);
        lua_setfield(L,-2,"pos");


        lua_settable(L,-3);
    }
    lua_setfield(L,table_pos,"symbols");
    return 1;
}
static int markAsExecutable(lua_State* L)
{
    unsigned addr=luaL_checkunsigned(L,1);
    std::vector<DFHack::t_memrange> ranges;
    DFHack::Core::getInstance().p->getMemRanges(ranges);
    for(size_t i=0;i<ranges.size();i++)
    {
        if(ranges[i].isInRange((void*)addr))
        {
            DFHack::t_memrange newperm=ranges[i];
            newperm.execute=true;
            DFHack::Core::getInstance().p->setPermisions(ranges[i],newperm);
            return 0;
        }
    }
    lua_pushlstring(L,"Memory range not found",23);
    lua_error(L);
    return 0;
}
DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(loadObjectFile),
    DFHACK_LUA_COMMAND(markAsExecutable),
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    return CR_OK;
}