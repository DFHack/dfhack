#include "lua_Console.h"
//TODO error management. Using lua error? or something other?
static DFHack::Console* GetConsolePtr(lua::state &st)
{
	int t=st.gettop();
	st.getglobal("Console");
	st.getfield("__pointer");
	DFHack::Console* c=static_cast<DFHack::Console*>(lua_touserdata(st,-1));
	st.settop(t);
	return c;
}
static int lua_Console_print(lua_State *S)
{
	lua::state st(S);
	int t=st.gettop();
	DFHack::Console* c=GetConsolePtr(st);
	c->print("%s",st.as<string>(t).c_str());
	return 0;
}

static int lua_Console_printerr(lua_State *S)
{
	lua::state st(S);
	int t=st.gettop();
	DFHack::Console* c=GetConsolePtr(st);
	c->printerr("%s",st.as<string>(t).c_str());
	return 0;
}

static int lua_Console_clear(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->clear();
	return 0;
}
static int lua_Console_gotoxy(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->gotoxy(st.as<int>(1,1),st.as<int>(1,2));
	return 0;
}
static int lua_Console_color(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->color( static_cast<DFHack::Console::color_value>(st.as<int>(-1,1)) );
	return 0;
}
static int lua_Console_reset_color(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->reset_color();
	return 0;
}
static int lua_Console_cursor(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->cursor(st.as<bool>(1));
	return 0;
}
static int lua_Console_msleep(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->msleep(st.as<unsigned>(1));
	return 0;
}
static int lua_Console_get_columns(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	st.push(c->get_columns());
	return 1;
}
static int lua_Console_get_rows(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	st.push(c->get_rows());
	return 1;
}
static int lua_Console_lineedit(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	string ret;
	int i=c->lineedit(st.as<string>(1),ret);
	st.push(ret);
	st.push(i);
	return 2;// dunno if len is needed...
}
static int lua_Console_history_add(lua_State *S)
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->history_add(st.as<string>(1));
	return 0;
}
/*static int lua_Console_history_clear(lua_State *S) //TODO someday add this
{
	lua::state st(S);
	DFHack::Console* c=GetConsolePtr(st);
	c->history_clear();
	return 0;
}*/
const luaL_Reg lua_console_func[]=
{
	{"print",lua_Console_print},
	{"printerr",lua_Console_printerr},
	{"clear",lua_Console_clear},
	{"gotoxy",lua_Console_gotoxy},
	{"color",lua_Console_color},
	{"reset_color",lua_Console_reset_color},
	{"cursor",lua_Console_cursor},
	{"msleep",lua_Console_msleep},
	{"get_columns",lua_Console_get_columns},
	{"get_rows",lua_Console_get_rows},
	{"lineedit",lua_Console_lineedit},
	{"history_add",lua_Console_history_add},
	//{"history_clear",lua_Console_history_clear},
	{NULL,NULL}
};
void lua::RegisterConsole(lua::state &st, DFHack::Console *c)
{
	st.getglobal("Console");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}

	st.pushlightuserdata(c);
	st.setfield("__pointer");
	
	lua::RegFunctionsLocal(st, lua_console_func);
	//TODO add color consts
	st.setglobal("Console");
}
