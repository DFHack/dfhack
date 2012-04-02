#include "luamain.h"
#include <vector>


lua::glua* lua::glua::ptr=0;
lua::glua::glua()
{
    RegBasics(mystate);
}
lua::state &lua::glua::Get()
{
    if(!glua::ptr)
        glua::ptr=new glua();
    return glua::ptr->mystate;
}


int lua_Ver_Lua(lua_State *L)
{
    lua::state st(L);
    st.push(LUA_RELEASE);
    return 1;
}


static const struct luaL_Reg lua_basic_lib [] =
{
    {"getluaver", lua_Ver_Lua},
    {NULL, NULL}  /* sentinel */
};
void lua::RegBasics(lua::state &L)
{
    luaL_openlibs(L);
    RegFunctions(L,lua_basic_lib);
}

void lua::RegFunctions(lua::state &L,luaL_Reg const*arr)
{
    luaL_Reg const *cur=arr;
    while(cur->name!=NULL)
    {
        lua_pushcfunction(L, cur->func);
        lua_setglobal(L, cur->name);

        cur++;
    }
}
void lua::RegFunctionsLocal(lua::state &L,luaL_Reg const*arr)
{
    luaL_Reg const *cur=arr;
    while(cur->name!=NULL)
    {
        lua_pushcfunction(L, cur->func);
        //lua_setglobal(L, cur->name);
        L.setfield(cur->name);

        cur++;
    }
}
string lua::DebugDump(lua::state &L)
{
    L.getglobal("debug");
    L.getfield("traceback");
    L.call(0,1);
    string ret=L.as<string>();
    //cout<<"StackTrace:"<<ret<<endl;
    L.settop(0);
    return ret;
}
