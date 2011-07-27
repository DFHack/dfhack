#include "lua_Offsets.h"
//TODO make a seperate module with peeks/pokes and page permisions (linux/windows spec)
unsigned char peekb(size_t offset) 
{
	return *((unsigned char*)(offset));
}
unsigned short peekw(size_t offset) 
{
	return *((unsigned short*)(offset));
}
unsigned peekd(size_t offset) 
{
	return *((unsigned*)(offset));
}
void lua::RegisterEngine(lua::state &st)
{

}