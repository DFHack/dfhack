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


const luaL_Reg lua_misc_func[]=
{
    {"loadmod",LoadMod},
    {"getmod",GetMod},
    {"loadobj",LoadObj},
    {"loadobjsymbols",LoadObjSymbols},
    {"findmarker",FindMarker},
    {"newmod",NewMod},
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
