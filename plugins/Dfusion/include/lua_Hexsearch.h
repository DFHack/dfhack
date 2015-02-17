#ifndef LUA_HEXSEARCH_H
#define LUA_HEXSEARCH_H
#include "hexsearch.h"

#include "luamain.h"



namespace lua
{

class Hexsearch
{
    int tblid;
    ::Hexsearch *p;
public:
    Hexsearch(lua_State *L,int id);
    ~Hexsearch();

    int GetTableId(){return tblid;};

    int find(lua_State *L);
    int findall(lua_State *L);
    int reset(lua_State *L);

    DEF_LUNE(Hexsearch);
};
void RegisterHexsearch(lua::state &st);

}


#endif