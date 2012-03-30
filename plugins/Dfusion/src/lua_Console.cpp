#include "lua_Console.h"

#include <sstream>

//TODO error management. Using lua error? or something other?
static DFHack::color_ostream* GetConsolePtr(lua::state &st)
{
	int t=st.gettop();
	st.getglobal("Console");
	st.getfield("__pointer");
	DFHack::color_ostream* c=static_cast<DFHack::color_ostream*>(lua_touserdata(st,-1));
	st.settop(t);
	return c;
}

static std::string lua_print_fmt(lua_State *L)
{
    /* Copied from lua source to fully replicate builtin print */
    int n = lua_gettop(L);  /* number of arguments */
    lua_getglobal(L, "tostring");

    std::stringstream ss;

    for (int i=1; i<=n; i++) {
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        const char *s = lua_tostring(L, -1);  /* get result */
        if (s == NULL)
            luaL_error(L, "tostring must return a string to print");
        if (i>1)
            ss << '\t';
        ss << s;
        lua_pop(L, 1);  /* pop result */
    }

    return ss.str();
}

static int lua_Console_print(lua_State *S)
{
    lua::state st(S);
    DFHack::color_ostream* c=GetConsolePtr(st);
    c->print("%s", lua_print_fmt(S).c_str());
    return 0;
}

static int lua_Console_println(lua_State *S)
{
    lua::state st(S);
    DFHack::color_ostream* c=GetConsolePtr(st);
    c->print("%s\n", lua_print_fmt(S).c_str());
    return 0;
}

static int lua_Console_printerr(lua_State *S)
{
    lua::state st(S);
    DFHack::color_ostream* c=GetConsolePtr(st);
    c->printerr("%s", lua_print_fmt(S).c_str());
    return 0;
}

static int lua_Console_clear(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	c->clear();
	return 0;
}
static int lua_Console_gotoxy(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		con->gotoxy(st.as<int>(1,1),st.as<int>(1,2));
	}
	return 0;
}
static int lua_Console_color(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	c->color( static_cast<DFHack::Console::color_value>(st.as<int>(-1,1)) );
	return 0;
}
static int lua_Console_reset_color(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	c->reset_color();
	return 0;
}
static int lua_Console_cursor(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		con->cursor(st.as<bool>(1));
	}
	return 0;
}
static int lua_Console_msleep(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		con->msleep(st.as<unsigned>(1));
	}
	return 0;
}
static int lua_Console_get_columns(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		st.push(con->get_columns());
	}
	return 1;
}
static int lua_Console_get_rows(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		st.push(con->get_rows());
	}
	return 1;
}
static int lua_Console_lineedit(lua_State *S)
{
	lua::state st(S);
	DFHack::color_ostream* c=GetConsolePtr(st);
	if(c->is_console())
	{
		DFHack::Console* con=static_cast<DFHack::Console*>(c);
		string ret;
		DFHack::CommandHistory hist;
		int i=con->lineedit(st.as<string>(1),ret,hist);
		st.push(ret);
		st.push(i);
		return 2;// dunno if len is needed...
	}
	else
		return 0;
}
const luaL_Reg lua_console_func[]=
{
	{"print",lua_Console_print},
    {"println",lua_Console_println},
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
	{NULL,NULL}
};
void lua::RegisterConsole(lua::state &st)
{
	st.getglobal("Console");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}

	lua::RegFunctionsLocal(st, lua_console_func);
	//TODO add color consts
	st.setglobal("Console");
}
void lua::SetConsole(lua::state &st,DFHack::color_ostream& stream)
{
	int top=st.gettop();
	st.getglobal("Console");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}

	st.pushlightuserdata(&stream);
	st.setfield("__pointer");
	st.settop(top);
}