#include "lua_VersionInfo.h"

#define VI_FUNC(name) int lua_VI_ ## name (lua_State *L){\
	lua::state s(L);\
	DFHack::VersionInfo* vif=DFHack::Core::getInstance().vinfo;
#define END_VI_FUNC }

VI_FUNC(getBase)
//int lua_VI_getBase(lua_State *L)
	s.push(vif->getBase());
	return 1;
END_VI_FUNC

VI_FUNC(setBase)
	uint32_t val=s.as<uint32_t>(1);
	vif->setBase(val);
	return 0;
END_VI_FUNC
VI_FUNC(rebaseTo)
	uint32_t val=s.as<uint32_t>(1);
	vif->rebaseTo(val);
	return 0;
END_VI_FUNC
VI_FUNC(addMD5)
	std::string val=s.as<std::string>(1);
	vif->addMD5(val);
	return 0;
END_VI_FUNC     
VI_FUNC(hasMD5)
	std::string val=s.as<std::string>(1);
	s.push(vif->hasMD5(val));
	return 1;
END_VI_FUNC   
       
VI_FUNC(addPE)
	uint32_t val=s.as<uint32_t>(1);
	vif->addPE(val);
	return 0;
END_VI_FUNC      

VI_FUNC(hasPE)
	uint32_t val=s.as<uint32_t>(1);
	s.push(vif->hasPE(val));
	return 1;
END_VI_FUNC
        
VI_FUNC(setVersion)
	std::string val=s.as<std::string>(1);
	vif->setVersion(val);
	return 0;
END_VI_FUNC

VI_FUNC(getVersion)
	s.push(vif->getVersion());
	return 1;
END_VI_FUNC        

VI_FUNC(setAddress)
	std::string key=s.as<std::string>(1);
	uint32_t val=s.as<uint32_t>(2);
	vif->setAddress(key,val);
	return 0;
END_VI_FUNC 

VI_FUNC(getAddress)
	std::string key=s.as<std::string>(1);
	
	s.push(vif->getAddress(key));
	return 1;
END_VI_FUNC         
        
VI_FUNC(setOS)
	unsigned os=s.as<unsigned>(1);
	vif->setOS((DFHack::OSType)os);
	return 0;
END_VI_FUNC 

VI_FUNC(getOS)
	s.push(vif->getOS());
	return 1;
END_VI_FUNC 
#undef VI_FUNC
#undef END_VI_FUNC
#define VI_FUNC(name) {#name,lua_VI_ ## name}
const luaL_Reg lua_vinfo_func[]=
{
	VI_FUNC(getBase),
	VI_FUNC(setBase),
	VI_FUNC(rebaseTo),
	VI_FUNC(addMD5),
	VI_FUNC(hasMD5),
	VI_FUNC(addPE),
	VI_FUNC(hasPE),
	VI_FUNC(setVersion),
	VI_FUNC(getVersion),
	VI_FUNC(setAddress),
	VI_FUNC(getAddress),
	VI_FUNC(setOS),
	VI_FUNC(getOS),
	{NULL,NULL}
};
#undef VI_FUNC
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
