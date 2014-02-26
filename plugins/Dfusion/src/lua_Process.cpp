#include "lua_Process.h"

static DFHack::Process* GetProcessPtr(lua::state &st)
{
	return DFHack::Core::getInstance().p;
}

static int lua_Process_readDWord(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	uint32_t ret=c->readDWord( (void *) st.as<uint32_t>(1));
	st.push(ret);
	return 1;
}
static int lua_Process_writeDWord(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c->writeDWord((void *) st.as<uint32_t>(1),st.as<uint32_t>(2));
	return 0;
}
static int lua_Process_readFloat(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    float ret=c->readFloat((void *) st.as<uint32_t>(1));
	st.push(ret);
	return 1;
}

static int lua_Process_readWord(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    uint16_t ret=c->readWord((void *) st.as<uint32_t>(1));
	st.push(ret);
	return 1;
}

static int lua_Process_writeWord(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c->writeWord((void *) st.as<uint32_t>(1),st.as<uint16_t>(2));
	return 0;
}
static int lua_Process_readByte(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    uint8_t ret=c->readByte((void *) st.as<uint32_t>(1));
	st.push(ret);
	return 1;
}

static int lua_Process_writeByte(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c->writeByte((void *) st.as<uint32_t>(1),st.as<uint8_t>(2));
	return 0;
}
static int lua_Process_read(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	size_t len=st.as<uint32_t>(2);
	uint8_t* buf;
	
	if(!st.is<lua::nil>(3))
		buf=(uint8_t*)lua_touserdata(st,3);
	else
		buf=new uint8_t[len];
    c->read((void *) st.as<uint32_t>(1),len,buf);
	st.pushlightuserdata(buf);
	return 1;
}
static int lua_Process_write(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c-> write((void *) st.as<uint32_t>(1),st.as<uint32_t>(2),static_cast<uint8_t*>(lua_touserdata(st,3)));
	return 0;
}
static int lua_Process_readSTLString (lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    std::string r=c->readSTLString((void *) st.as<uint32_t>(1));
	st.push(r);
	return 1;
}

static int lua_Process_writeSTLString(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c->writeSTLString((void *) st.as<uint32_t>(1),st.as<std::string>(2));
	return 0;
}
static int lua_Process_copySTLString(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    c->copySTLString((void *) st.as<uint32_t>(1),st.as<uint32_t>(2));
	return 0;
}
static int lua_Process_doReadClassName(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	std::string r=c->doReadClassName((void*)st.as<size_t>(1));
	st.push(r);
	return 1;
}
static int lua_Process_readClassName(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	std::string r=c->readClassName((void*)st.as<size_t>(1));
	st.push(r);
	return 1;
}
static int lua_Process_readCString (lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
    std::string r=c->readCString((void *) st.as<uint32_t>(1));
	st.push(r);
	return 1;
}
static int lua_Process_isSuspended(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	st.push(c->isSuspended());
	return 1;
}
static int lua_Process_isIdentified(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	st.push(c->isIdentified());
	return 1;
}
static int lua_Process_getMemRanges(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	std::vector<DFHack::t_memrange> ranges;
	c->getMemRanges(ranges);
	st.newtable();
	for(size_t i=0;i<ranges.size();i++)
	{
		st.push(i);
		st.newtable();
		st.push((uint32_t)ranges[i].start); // WARNING!! lua has only 32bit numbers, possible loss of data!!
		st.setfield("start");
        st.push((uint32_t)ranges[i].end);
		st.setfield("end");
		st.push(std::string(ranges[i].name));
		st.setfield("name");
		st.push(ranges[i].read);
		st.setfield("read");
		st.push(ranges[i].write);
		st.setfield("write");
		st.push(ranges[i].execute);
		st.setfield("execute");
		st.push(ranges[i].shared);
		st.setfield("shared");
		st.push(ranges[i].valid);
		st.setfield("valid");
		st.settable();
	}
	return 1;
}
static int lua_Process_getBase(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	uint32_t base=c->getBase();
	st.push(base);
	return 1;
}
/*static int lua_Process_getPID(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	int ret=c->getPID();
	st.push(ret);
	return 1;
}*/
static int lua_Process_getPath(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	std::string ret=c->getPath();
	st.push(ret);
	return 1;
}
static int lua_Process_setPermisions(lua_State *S)
{
	lua::state st(S);
	DFHack::Process* c=GetProcessPtr(st);
	DFHack::t_memrange range,trange;

	st.getfield("start",1);
	range.start= (void *)st.as<uint64_t>();
	st.pop();
	st.getfield("end",1);
	range.end= (void *)st.as<uint64_t>();
	st.pop();

	st.getfield("read",2);
	trange.read=st.as<bool>();
	st.pop();
	st.getfield("write",2);
	trange.write=st.as<bool>();
	st.pop();
	st.getfield("execute",2);
	trange.execute=st.as<bool>();
	st.pop();

	c->setPermisions(range,trange);

	return 0;
}
#define PROC_FUNC(name) {#name,lua_Process_ ## name}
const luaL_Reg lua_process_func[]=
{
	PROC_FUNC(readDWord),
	PROC_FUNC(writeDWord),
	PROC_FUNC(readFloat),
	PROC_FUNC(readWord),
	PROC_FUNC(writeWord),
	PROC_FUNC(readByte),
	PROC_FUNC(writeByte),
	PROC_FUNC(read),
	PROC_FUNC(write),
	PROC_FUNC(readSTLString),
	PROC_FUNC(writeSTLString),
	PROC_FUNC(copySTLString),
	PROC_FUNC(doReadClassName),
	PROC_FUNC(readClassName),
	PROC_FUNC(readCString ),
	PROC_FUNC(isSuspended),
	PROC_FUNC(isIdentified),
	PROC_FUNC(getMemRanges),
	PROC_FUNC(getBase),
	//PROC_FUNC(getPID), //not implemented
	PROC_FUNC(getPath),
	PROC_FUNC(setPermisions),
	{NULL,NULL}
};
#undef PROC_FUNC
void lua::RegisterProcess(lua::state &st)
{
	st.getglobal("Process");
	if(st.is<lua::nil>())
	{
		st.pop();
		st.newtable();
	}

	lua::RegFunctionsLocal(st, lua_process_func);

	st.setglobal("Process");
}
