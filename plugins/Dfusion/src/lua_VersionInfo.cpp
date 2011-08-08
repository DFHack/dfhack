#include "lua_VersionInfo.h"
static int __lua_getMD5(lua_State *S)
{
    lua::state st(S);
    std::string output;
    bool ret=DFHack::Core::getInstance().vinfo->getMD5(output);
    st.push(ret);
    st.push(output);
    return 2;
}
static int __lua_getPE(lua_State *S)
{
    lua::state st(S);
    uint32_t output;
    bool ret=DFHack::Core::getInstance().vinfo->getPE(output);
    st.push(ret);
    st.push(output);
    return 2;
}
static int __lua_PrintOffsets(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->PrintOffsets();
    st.push(output);
    return 1;
}
static int __lua_getMood(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getMood(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
/*static int __lua_getString(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getString(st.as<std::string>(1));
    st.push(output);
    return 1;
}*/
static int __lua_getProfession(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getProfession(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
static int __lua_getJob(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getJob(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
static int __lua_getSkill(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getSkill(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
static int __lua_getTrait(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getTrait(st.as<uint32_t>(1),st.as<uint32_t>(2));
    st.push(output);
    return 1;
}
static int __lua_getTraitName(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getTraitName(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
static int __lua_getLabor(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getLabor(st.as<uint32_t>(1));
    st.push(output);
    return 1;
}
static int __lua_getAllTraits(lua_State *S)
{
    lua::state st(S);
    std::vector< std::vector<std::string> >  output=DFHack::Core::getInstance().vinfo->getAllTraits();
    st.newtable();
    for(size_t i=0;i<output.size();i++)
    {
        st.push(i+1);
        st.newtable();
        for(size_t j=0;j<output[i].size();j++)
        {
            st.push(j+1);
            st.push(output[i][j]);
            st.settable();
        }
        st.settable();
    }
    return 1;
}
static int __lua_getAllLabours(lua_State *S)
{
    lua::state st(S);
    std::map<uint32_t, std::string>  output=DFHack::Core::getInstance().vinfo->getAllLabours();
    st.newtable();
    for(std::map<uint32_t, std::string>::iterator it=output.begin();it!=output.end();it++)
    {
        st.push(it->first);
        st.push(it->second);
        st.settable();
    }
    return 1;
}
static int __lua_getLevelInfo(lua_State *S)
{
    lua::state st(S);
    DFHack::t_level  output=DFHack::Core::getInstance().vinfo->getLevelInfo(st.as<uint32_t>(1));
    st.newtable();
    st.push(output.level);
    st.setfield("level");
    st.push(output.name);
    st.setfield("name");
    st.push(output.xpNxtLvl);
    st.setfield("xpNxtLvl");
    return 1;
}
static int __lua_getVersion(lua_State *S)
{
    lua::state st(S);
    std::string output=DFHack::Core::getInstance().vinfo->getVersion();
    st.push(output);
    return 1;
}
static int __lua_getOS(lua_State *S)
{
    lua::state st(S);
    DFHack::OSType output=DFHack::Core::getInstance().vinfo->getOS();
    st.push(output);
    return 1;
}
static int __lua_resolveObjectToClassID(lua_State *S)
{
    lua::state st(S);
    int32_t ret;
    bool output=DFHack::Core::getInstance().vinfo->resolveObjectToClassID(st.as<uint32_t>(1),ret);
    st.push(output);
    st.push(ret);
    return 2;
}
static int __lua_resolveClassnameToClassID(lua_State *S)
{
    lua::state st(S);
    int32_t ret;
    bool output=DFHack::Core::getInstance().vinfo->resolveClassnameToClassID(st.as<std::string>(1),ret);
    st.push(output);
    st.push(ret);
    return 2;
}
static int __lua_resolveClassnameToVPtr(lua_State *S)
{
    lua::state st(S);
    void * ret;
    bool output=DFHack::Core::getInstance().vinfo->resolveClassnameToVPtr(st.as<std::string>(1),ret);
    st.push(output);
    st.pushlightuserdata(ret);
    return 2;
}
static int __lua_resolveClassIDToClassname(lua_State *S)
{
    lua::state st(S);
    std::string ret;
    bool output=DFHack::Core::getInstance().vinfo->resolveClassIDToClassname(st.as<int32_t>(1),ret);
    st.push(output);
    st.push(ret);
    return 2;
}
#define ___m(x) {#x,__lua_##x}
const luaL_Reg lua_vinfo_func[]=
{
    ___m(getMD5),
    ___m(getPE),
    ___m(PrintOffsets),
    ___m(getMood),
//    ___m(getString),
    ___m(getProfession),
    ___m(getJob),
    ___m(getSkill),
    ___m(getTrait),
    ___m(getTraitName),
    ___m(getLabor),
    ___m(getAllTraits),
    ___m(getAllLabours),
    ___m(getLevelInfo),
    ___m(getVersion),
    ___m(getOS),
    ___m(resolveObjectToClassID),
    ___m(resolveClassnameToClassID),
    ___m(resolveClassnameToVPtr),
    ___m(resolveClassIDToClassname),
    {NULL,NULL}
};
#undef ___m
void lua::RegisterVersionInfo(lua::state &st)
{
    st.getglobal("VersionInfo");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}

	lua::RegFunctionsLocal(st, lua_vinfo_func);
	st.setglobal("VersionInfo");
}
