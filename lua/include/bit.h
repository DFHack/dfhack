#ifndef BIT_H
#define BIT_H

#define LUA_BITOP_VERSION	"1.0.1"

#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#ifdef __cplusplus
 extern "C" {
#endif
LUALIB_API int luaopen_bit(lua_State *L);
#ifdef __cplusplus
 }
#endif
#endif // BIT_H

