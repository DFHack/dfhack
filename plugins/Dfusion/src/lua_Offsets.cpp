#include "lua_Offsets.h"
#include <string.h>
//TODO make a seperate module with peeks/pokes and page permisions (linux/windows spec)
//TODO maybe remove alltogether- use DFHack::Process instead?
unsigned char peekb(size_t offset) 
{
	return *((unsigned char*)(offset));
}
unsigned short peekw(size_t offset) 
{
	return *((unsigned short*)(offset));
}
unsigned peekd(size_t offset) 
{
	return *((unsigned*)(offset));
}
void peekarb(size_t offset, void *mem,size_t size)
{
	memcpy(mem,(void*)offset,size);
}
void peekstr(size_t offset, char* buf, size_t maxsize)
{
	strncpy(buf,(char*)offset,maxsize);
}
void pokeb(size_t offset,unsigned char val) 
{
	*((unsigned char*)(offset))=val;
}
void pokew(size_t offset,unsigned short val) 
{
	*((unsigned short*)(offset))=val;
}
void poked(size_t offset,unsigned val) 
{
	*((unsigned*)(offset))=val;
}
void pokearb(size_t offset, void *mem,size_t size)
{
	memcpy((void*)offset,mem,size);
}
void pokestr(size_t offset, char* buf, size_t maxsize)
{
	strncpy((char*)offset,buf,maxsize);
}
template <typename T>
T peek(size_t offset)		//prob lower performance
{
	T tmp;
	peekarb(offset,&tmp,sizeof(T));
	return tmp;
}

//// lua stuff here
static int lua_peekb(lua_State *L)
{
    lua::state st(L);
    st.push(peekb(st.as<size_t>(1)));
    return 1;
}
static int lua_peekd(lua_State *L)
{
    lua::state st(L);
    st.push(peekd(st.as<size_t>(1)));
    return 1;
}
static int lua_peekw(lua_State *L)
{
    lua::state st(L);
    st.push(peekw(st.as<size_t>(1)));
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
/*static int lua_peekarb(lua_State *L)
{
    lua::state st(L);
    st.push(peekarb(st.as<DWORD>(1)));
    return 1;
}*/
static int lua_pokeb(lua_State *L)
{
    lua::state st(L);
    pokeb(st.as<size_t>(1),st.as<size_t>(2));
    return 0;
}
static int lua_poked(lua_State *L)
{
    lua::state st(L);
    poked(st.as<size_t>(1),st.as<size_t>(2));
    return 0;
}
static int lua_pokew(lua_State *L)
{
    lua::state st(L);
    pokew(st.as<size_t>(1),st.as<size_t>(2));
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
const luaL_Reg lua_engine_func[]=
{
	{"peekb",lua_peekb},
	{"peekw",lua_peekw},
	{"peekd",lua_peekd},
	{"peekarb",lua_peekarb},
	{"peekstr",lua_peekstr},
	{"pokeb",lua_pokeb},
	{"pokew",lua_pokew},
	{"poked",lua_poked},
	{"pokearb",lua_pokearb},
	{"pokestr",lua_pokestr},
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
