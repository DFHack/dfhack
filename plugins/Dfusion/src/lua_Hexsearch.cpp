#include "lua_Hexsearch.h"
int lua::Hexsearch::find(lua_State *L)
{
    lua::state st(L);
	uint64_t pos=p->FindNext();
	st.push(pos);
	return 1;
}
int lua::Hexsearch::findall(lua_State *L)
{
    lua::state st(L);
	std::vector<uint64_t> pos=p->FindAll();
	st.newtable();
	for(int i=0;i<pos.size();i++)
	{
		st.push(i+1);
		st.push(pos[i]);
		st.settable();
	}
	return 1;
}
lua::Hexsearch::Hexsearch(lua_State *L,int id):tblid(id)
{
    lua::state st(L);
	uint64_t start,end;
	::Hexsearch::SearchArgType args;
	start=st.as<uint32_t>(1);
	end=st.as<uint32_t>(2);
	for(int i=3;i<=st.gettop();i++)
	{
		args.push_back(st.as<int>(i));
	}
	p=new ::Hexsearch(args,start,end);
}
lua::Hexsearch::~Hexsearch()
{
	delete p;
}
int lua::Hexsearch::reset(lua_State *L)
{
    lua::state st(L);
	p->Reset();
	return 0;
}

IMP_LUNE(lua::Hexsearch,hexsearch);
LUNE_METHODS_START(lua::Hexsearch)
	method(lua::Hexsearch,find),
	method(lua::Hexsearch,findall),
	method(lua::Hexsearch,reset),
LUNE_METHODS_END();
void lua::RegisterHexsearch(lua::state &st)
{

	Lune<lua::Hexsearch>::Register(st);

}