#include "lua_FunctionCall.h"
using namespace lua;
int lua_FunctionCall(lua_State *L)
{
	lua::state st(L);
	FunctionCaller cl(0);
	size_t ptr=st.as<size_t>(1);
	int callconv=st.as<size_t>(2);
	vector<int> arguments;
	for(int i=3;i<st.gettop();i++)
		arguments.push_back(st.as<int>(i));
	int ret=cl.CallFunction(ptr,(FunctionCaller::callconv)callconv,arguments);
	st.push(ret);
	return 1;// dunno if len is needed...
}

const luaL_Reg lua_functioncall_func[]=
{
	{"call",lua_FunctionCall},
	{NULL,NULL}
};

#define __ADDCONST(name) st.push(::FunctionCaller::  name); st.setglobal(#name)
void lua::RegisterFunctionCall(lua::state &st)
{
	st.getglobal("FunctionCall");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}
	__ADDCONST(STD_CALL);
	__ADDCONST(FAST_CALL);
	__ADDCONST(THIS_CALL);
	__ADDCONST(CDECL_CALL);
	lua::RegFunctionsLocal(st, lua_functioncall_func);
	st.setglobal("FunctionCall");
}
#undef __ADDCONST