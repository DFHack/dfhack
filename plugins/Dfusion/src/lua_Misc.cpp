#include "lua_Misc.h"
uint32_t lua::PlugManager::AddNewPlug(std::string name,uint32_t size,uint32_t loc)
{
	void *p;
	if(size!=0)
		p=new unsigned char[size];
	else
		p=(void*)loc;
	plugs[name]=p;
	return (uint32_t)p;
}
uint32_t lua::PlugManager::FindPlugin(std::string name)
{
	mapPlugs::iterator it=plugs.find(name);
	if(it!=plugs.end())
		return (uint32_t)it->second;
	else
		return 0;
}


static int LoadMod(lua_State *L)
{
    lua::state st(L);
    std::string modfile=st.as<std::string>(1);
    std::string modname=st.as<std::string>(2);
	uint32_t size_add=st.as<uint32_t>(0,3);
    OutFile::File f(modfile);
    uint32_t size=f.GetTextSize();
	uint32_t pos=lua::PlugManager::GetInst().AddNewPlug(modname,size+size_add);
    char *buf;
    buf=new char[size];
    f.GetText(buf);
    //std::cout<<"poking @:"<<std::hex<<pos<<"size :"<<size<<std::endl;
    DFHack::Core::getInstance().p->write((void *) pos,size,(uint8_t*)buf);
    delete [] buf;
    st.push(pos);
    st.push(size);
    return 2;
}
static int LoadObj(lua_State *L)
{
    lua::state st(L);
    std::string modfile=st.as<std::string>(1);
    OutFile::File f(modfile);
    size_t s=f.GetTextSize();
    void *p=st.newuserdata(s); //TODO does it leak memory??
    f.GetText((char*)p);
    st.push(s);
    return 2;
}
static int FindMarker(lua_State *L) // marker, void ptr, size, start
{
    lua::state st(L);
    union
    {
        unsigned char bytes[4];
        size_t mark;
    }M;
    M.mark=st.as<size_t>(1);
    unsigned char *p=(unsigned char *)lua_touserdata(L, 2);//st.as<lua::userdata>(2);
    size_t size=st.as<size_t>(3);
    size_t start=st.as<size_t>(4);
    for(size_t i=start;i<size;i++)
    {
        bool ok;
        ok=true;
        if(p[i]==M.bytes[0])
        {
            for(size_t j=0;j<4;j++)
            {
                if(p[i+j]!=M.bytes[j])
                {
                    ok=false;
                    break;
                }
            }
            if(ok)
            {
                st.push(i);
                return 1;
            }
        }
    }

    return 0;
}
static int LoadObjSymbols(lua_State *L)
{
    lua::state st(L);
    std::string modfile=st.as<std::string>(1);
    OutFile::File f(modfile);
    OutFile::vSymbol vec=f.GetSymbols();
    OutFile::Symbol S;

    st.newtable();
    for(size_t i=0;i<vec.size();i++)
    {
        st.push(i);
        S=vec[i];
        st.newtable();
        st.push(S.name);
        st.setfield("name");
        st.push(S.pos);
        st.setfield("pos");
        st.settable();
    }

    return 1;
}
static int NewMod(lua_State *L)
{
    lua::state st(L);
    std::string modname=st.as<std::string>(1);
    size_t size=st.as<size_t>(2);
    size_t loc=st.as<size_t>(3,0);
    uint32_t pos=lua::PlugManager::GetInst().AddNewPlug(modname,size,loc);
    st.push(pos);
    return 1;
}
static int GetMod(lua_State *L)
{
    lua::state st(L);
    std::string modname=st.as<std::string>(1);
    uint32_t pos=lua::PlugManager::GetInst().FindPlugin(modname);
    if(pos==0)
        st.push();
    else
        st.push(pos);
    return 1;
}
static int lua_malloc(lua_State *L)
{
	lua::state st(L);
    size_t size=st.as<size_t>(1);
    size_t pos=reinterpret_cast<size_t>(malloc(size));
	st.push(pos);
    return 1;
}
static int lua_malloc_free(lua_State *L)
{
	lua::state st(L);
    size_t ptr=st.as<size_t>(1);
    free(reinterpret_cast<void*>(ptr));
    return 0;
}
#ifdef LINUX_BUILD
static size_t __attribute__((stdcall))  PushValue(size_t ret,uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t edi,uint32_t esi,uint32_t esp,uint32_t ebp)
#else
static size_t __stdcall PushValue(size_t ret,uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t edi,uint32_t esi,uint32_t esp,uint32_t ebp)
#endif
{
	lua::state st=lua::glua::Get();
	st.getglobal("OnFunction");
	if(st.is<lua::nil>())
		return 0;
	st.newtable();
	st.push(eax);
	st.setfield("eax");
	st.push(ebx);
	st.setfield("ebx");
	st.push(ecx);
	st.setfield("ecx");
	st.push(edx);
	st.setfield("edx");
	st.push(edi);
	st.setfield("edi");
	st.push(esi);
	st.setfield("esi");
	st.push(esp+12);
	st.setfield("esp");
	st.push(ebp);
	st.setfield("ebp");
	st.push(ret);
	st.setfield("ret");
	DFHack::Lua::SafeCall(DFHack::Core::getInstance().getConsole(),st,1,1);
	return st.as<uint32_t>();
}
static int Get_PushValue(lua_State *L)
{
    lua::state st(L);
    st.push((uint32_t)&PushValue);
    return 1;
}
static int Call_Df(lua_State *L)
{
    lua::state st(L);
    FunctionCaller f(0);
	std::vector<int> args;
	size_t ptr;
	FunctionCaller::callconv conv;
	ptr=st.as<size_t>(1);
	conv=(FunctionCaller::callconv)st.as<size_t>(2);
	for(size_t j=3;j<=st.gettop();j++)
		args.push_back(st.as<int>(j));
	st.push(f.CallFunction(ptr,conv,args));
    return 1;
}
static int Suspend_Df(lua_State *L)
{
	lua::state st(L);
	DFHack::Core::getInstance().Suspend();
	return 0;
}
static int Resume_Df(lua_State *L)
{
	lua::state st(L);
	DFHack::Core::getInstance().Resume();
	return 0;
}
static int Cast(lua_State *L)
{
	lua::state st(L);
	if(DFHack::Lua::IsDFObject(st,1)!=DFHack::Lua::OBJ_TYPE)
		st.error("First argument must be df type!");
	if(!st.is<lua::number>(2)) //todo maybe lightuserdata?
		st.error("Second argument must be pointer as a number!");
	st.getfield("_identity",1);
	DFHack::Lua::PushDFObject(st,(DFHack::type_identity*)lua_touserdata(st,-1),(void*)st.as<int>(2));
	return 1;
}
const luaL_Reg lua_misc_func[]=
{
	{"alloc",lua_malloc},
	{"free",lua_malloc_free},
	{"loadmod",LoadMod},
	{"getmod",GetMod},
	{"loadobj",LoadObj},
	{"loadobjsymbols",LoadObjSymbols},
	{"findmarker",FindMarker},
	{"newmod",NewMod},
	{"getpushvalue",Get_PushValue},
	{"calldf",Call_Df},
	{"suspend",Suspend_Df},
	{"resume",Resume_Df},
	{"cast",Cast},
	{NULL,NULL}
};
void lua::RegisterMisc(lua::state &st)
{
	st.getglobal("engine");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}
	lua::RegFunctionsLocal(st, lua_misc_func);
	st.setglobal("engine");
}
