#include "lua_VersionInfo.h"
namespace lua
{
OffsetGroup::OffsetGroup(lua_State *L,int id):tblid(id)
{
	p=static_cast<DFHack::OffsetGroup*>(lua_touserdata(L,1));
}

int OffsetGroup::getOffset(lua_State *L)
{
    lua::state st(L);
    int32_t ret=p->getOffset(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
int OffsetGroup::getAddress(lua_State *L)
{
    lua::state st(L);
    uint32_t ret=p->getAddress(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
int OffsetGroup::getHexValue(lua_State *L)
{
    lua::state st(L);
    uint32_t ret=p->getHexValue(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
int OffsetGroup::getString(lua_State *L)
{
    lua::state st(L);
    std::string ret=p->getString(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
int OffsetGroup::getGroup(lua_State *L)
{
    lua::state st(L);
	DFHack::OffsetGroup* t= p->getGroup(st.as<std::string>(1));
	st.getglobal("OffsetGroup");	
	st.getfield("new");
	st.getglobal("OffsetGroup");
	st.pushlightuserdata(t);
	st.pcall(2,1);
	return 1;
}
int OffsetGroup::getSafeOffset(lua_State *L)
{
    lua::state st(L);
    int32_t out;
    bool ret=p->getSafeOffset(st.as<std::string>(1),out);
    st.push(ret);
    st.push(out);
    return 2;
}
int OffsetGroup::getSafeAddress(lua_State *L)
{
    lua::state st(L);
    uint32_t out;
    bool ret=p->getSafeAddress(st.as<std::string>(1),out);
    st.push(ret);
    st.push(out);
    return 2;
}
int OffsetGroup::PrintOffsets(lua_State *L)
{
	lua::state st(L);
    std::string output;
    output=p->PrintOffsets(st.as<int>(1)); 
    st.push(output);
    return 1;
}
int OffsetGroup::getName(lua_State *L)
{
    lua::state st(L);
    std::string ret=p->getName();
    st.push(ret);
    return 1;
}
int OffsetGroup::getFullName(lua_State *L)
{
    lua::state st(L);
    std::string ret=p->getFullName();
    st.push(ret);
    return 1;
}
int OffsetGroup::getParent(lua_State *L)
{
	lua::state st(L);
	DFHack::OffsetGroup* t= p->getParent();
	st.getglobal("OffsetGroup");	
	st.getfield("new");
	st.getglobal("OffsetGroup");
	st.pushlightuserdata(t);
	st.pcall(2,1);
	return 1;
}
int OffsetGroup::getKeys(lua_State *L)
{
	const char* invalids[]={"notset","invalid","valid"};
	const char* keytypes[]={"offset","address","hexval","string","group"};
	lua::state st(L);
	std::vector<DFHack::OffsetKey> t= p->getKeys();
	st.newtable();
	for(size_t i=0;i<t.size();i++)
	{
		st.push(i);
		st.newtable();
		st.push(invalids[t[i].inval]);
		st.setfield("invalid");
		st.push(t[i].key);
		st.setfield("key");
		st.push(keytypes[t[i].keytype]);//maybe use int...
		st.setfield("keytype"); 

		st.settable();
	}
	return 1;
}

}
IMP_LUNE(lua::OffsetGroup,OffsetGroup);
LUNE_METHODS_START(lua::OffsetGroup)
	method(lua::OffsetGroup,getOffset),
	method(lua::OffsetGroup,getAddress),
	method(lua::OffsetGroup,getHexValue),
	method(lua::OffsetGroup,getString),
	method(lua::OffsetGroup,getGroup),

	method(lua::OffsetGroup,getSafeOffset),
	method(lua::OffsetGroup,getSafeAddress),

	method(lua::OffsetGroup,PrintOffsets),
	method(lua::OffsetGroup,getName),
	method(lua::OffsetGroup,getFullName),
	method(lua::OffsetGroup,getParent),
	method(lua::OffsetGroup,getKeys),
LUNE_METHODS_END();
//	VersionInfo Stuff
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
    std::string output;
    //if(st.is<lua::nil>(1))
        output=DFHack::Core::getInstance().vinfo->PrintOffsets();
    //else
    //    output=DFHack::Core::getInstance().vinfo->PrintOffsets(st.as<int>(1)); //TODO add when virtual methods are ok
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
//      OFFSET BASE STUFF (for version info)
static int __lua_getOffset(lua_State *S)
{
    lua::state st(S);
    int32_t ret=DFHack::Core::getInstance().vinfo->getOffset(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
static int __lua_getAddress(lua_State *S)
{
    lua::state st(S);
    uint32_t ret=DFHack::Core::getInstance().vinfo->getAddress(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
static int __lua_getHexValue(lua_State *S)
{
    lua::state st(S);
    uint32_t ret=DFHack::Core::getInstance().vinfo->getHexValue(st.as<std::string>(1));
    st.push(ret);
    return 1;
}
/*static int __lua_getString(lua_State *S) //from offsetbase
{
    lua::state st(S);
    std::string ret=DFHack::Core::getInstance().vinfo->getString(st.as<std::string>(1));
    st.push(ret);
    return 1;
}*/
static int __lua_getGroup(lua_State *S)
{
	lua::state st(S);
	if(st.as<std::string>(1)=="") //if no argument, return version info as a groupoffset (dynamic cast)
	{
		st.getglobal("OffsetGroup");	
		st.getfield("new");
		st.getglobal("OffsetGroup");
		st.pushlightuserdata(dynamic_cast<DFHack::OffsetGroup*>(DFHack::Core::getInstance().vinfo));
		st.pcall(2,1);
	}
	else
	{
		DFHack::OffsetGroup* t= DFHack::Core::getInstance().vinfo->getGroup(st.as<std::string>(1));
		st.getglobal("OffsetGroup");	
		st.getfield("new");
		st.getglobal("OffsetGroup");
		st.pushlightuserdata(t);
		st.pcall(2,1);
	}
	return 1;
}
static int __lua_getParent(lua_State *S)
{
	lua::state st(S);
	DFHack::OffsetGroup* t= DFHack::Core::getInstance().vinfo->getParent();
	st.getglobal("OffsetGroup");	
	st.getfield("new");
	st.getglobal("OffsetGroup");
	st.pushlightuserdata(t);
	st.pcall(2,1);
	return 1;
}
static int __lua_getSafeOffset(lua_State *S)
{
    lua::state st(S);
    int32_t out;
    bool ret=DFHack::Core::getInstance().vinfo->getSafeOffset(st.as<std::string>(1),out);
    st.push(ret);
    st.push(out);
    return 2;
}
static int __lua_getSafeAddress(lua_State *S)
{
    lua::state st(S);
    uint32_t out;
    bool ret=DFHack::Core::getInstance().vinfo->getSafeAddress(st.as<std::string>(1),out);
    st.push(ret);
    st.push(out);
    return 2;
}
//std::string PrintOffsets(int indentation); //overload up there^^^
static int __lua_getName(lua_State *S)
{
    lua::state st(S);
    std::string ret=DFHack::Core::getInstance().vinfo->getName();
    st.push(ret);
    return 1;
}
static int __lua_getFullName(lua_State *S)
{
    lua::state st(S);
    std::string ret=DFHack::Core::getInstance().vinfo->getFullName();
    st.push(ret);
    return 1;
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

	___m(getGroup),
	___m(getParent),

    ___m(getOffset),
    ___m(getAddress),
    ___m(getHexValue),
    //___m(getString),
    ___m(getSafeOffset),
    ___m(getSafeAddress),
    ___m(getName),
    ___m(getFullName),

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
	Lune<lua::OffsetGroup>::Register(st);
}
