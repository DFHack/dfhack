#ifndef LUASTUFF_H
#define LUASTUFF_H
#include <string>
using std::string;

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

//#include "lune.h"
#include "luaxx.hpp"

namespace lua
{
	//global lua state singleton
    class glua
    {
    public:
        static state &Get();
    private:
        glua();
        static glua *ptr;
        state mystate;
    };
	//registers basic lua commands
    void RegBasics(lua::state &L);
	//dumps lua function trace, useless unless called from lua.
    string DebugDump(lua::state &L);
	//register functions, first registers into global scope, second into current table
	void RegFunctions(lua::state &L,luaL_reg const *arr);
	void RegFunctionsLocal(lua::state &L,luaL_reg const *arr);
}



#endif // LUASTUFF_H
