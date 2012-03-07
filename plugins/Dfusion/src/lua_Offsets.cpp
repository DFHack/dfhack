#include "lua_Offsets.h"
#include <string.h>
#include <stdint.h>
//TODO make a seperate module with peeks/pokes and page permisions (linux/windows spec)
//TODO maybe remove alltogether- use DFHack::Process instead?
template <typename T>
T engine_peek(size_t offset)
{
	return *(reinterpret_cast<T*>(offset));
}
template <typename T>
void engine_poke(size_t offset,T val)
{
	*(reinterpret_cast<T*>(offset))=val;
}

void peekarb(size_t offset, void *mem,size_t size)
{
	memcpy(mem,(void*)offset,size);
}
void peekstr(size_t offset, char* buf, size_t maxsize)
{
	strncpy(buf,(char*)offset,maxsize);
}
void pokearb(size_t offset, void *mem,size_t size)
{
	memcpy((void*)offset,mem,size);
}
void pokestr(size_t offset, char* buf, size_t maxsize)
{
	strncpy((char*)offset,buf,maxsize);
}

//// lua stuff here
static int lua_peekb(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<uint8_t>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekw(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<uint16_t>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekd(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<uint32_t>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekq(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<uint64_t>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekfloat(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<float>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekdouble(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<double>(st.as<size_t>(1)));
    return 1;
}
static int lua_peekarb(lua_State *L)
{
    lua::state st(L);
    size_t size=st.as<size_t>(2);
    void *p=st.newuserdata(size);
    peekarb(st.as<size_t>(1),p,size);
    return 1;
}
static int lua_peekstr(lua_State *L)
{
    lua::state st(L);
    char *buf;
    buf=new char[256];
    peekstr(st.as<size_t>(1),buf,256);
    std::string tstr(buf);
    st.push(tstr);
    delete [] buf;
    return 1;
}
static int lua_peekstr2(lua_State *L)
{
    lua::state st(L);
    st.push(engine_peek<std::string>(st.as<size_t>(1)));
    return 1;
}
static int lua_pokeb(lua_State *L)
{
    lua::state st(L);
    engine_poke<uint8_t>(st.as<size_t>(1),st.as<uint8_t>(2));
    return 0;
}
static int lua_pokew(lua_State *L)
{
    lua::state st(L);
    engine_poke<uint16_t>(st.as<size_t>(1),st.as<uint16_t>(2));
    return 0;
}
static int lua_poked(lua_State *L)
{
    lua::state st(L);
    engine_poke<uint32_t>(st.as<size_t>(1),st.as<uint32_t>(2));
    return 0;
}
static int lua_pokeq(lua_State *L)
{
    lua::state st(L);
    engine_poke<uint64_t>(st.as<size_t>(1),st.as<uint64_t>(2));
    return 0;
}
static int lua_pokefloat(lua_State *L)
{
    lua::state st(L);
    engine_poke<float>(st.as<size_t>(1),st.as<float>(2));
    return 0;
}
static int lua_pokedouble(lua_State *L)
{
    lua::state st(L);
    engine_poke<double>(st.as<size_t>(1),st.as<double>(2));
    return 0;
}
static int lua_pokearb(lua_State *L)
{
    lua::state st(L);
    void *p=(void *)lua_touserdata(L, 2);//st.as<lua::userdata>(2);
    size_t size=st.as<size_t>(3);
    pokearb(st.as<size_t>(1),p,size);
    return 0;
}
static int lua_pokestr(lua_State *L)
{
    lua::state st(L);
    std::string trg=st.as<std::string>(2);
    pokestr(st.as<size_t>(1),(char*)trg.c_str(),trg.size());
    return 0;
}
static int lua_pokestr2(lua_State *L)
{
    lua::state st(L);
    std::string trg=st.as<std::string>(2);
    engine_poke<std::string>(st.as<size_t>(1),trg);
    return 0;
}
const luaL_Reg lua_engine_func[]=
{
	{"peekb",lua_peekb},
	{"peekw",lua_peekw},
	{"peekd",lua_peekd},
	{"peekq",lua_peekq},
	{"peekfloat",lua_peekfloat},
	{"peekdouble",lua_peekdouble},

	{"peekarb",lua_peekarb},
	{"peekstr",lua_peekstr},
	{"peekstr2",lua_peekstr2},

	{"pokeb",lua_pokeb},
	{"pokew",lua_pokew},
	{"poked",lua_poked},
	{"pokeq",lua_pokeq},
	{"pokefloat",lua_pokefloat},
	{"pokedouble",lua_pokedouble},

	{"pokearb",lua_pokearb},
	{"pokestr",lua_pokestr},
	{"pokestr2",lua_pokestr2},
	{NULL,NULL}
};

void lua::RegisterEngine(lua::state &st)
{
	st.getglobal("engine");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}
	lua::RegFunctionsLocal(st,lua_engine_func);
	st.setglobal("engine");
}
