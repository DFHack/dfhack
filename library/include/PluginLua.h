/*
 * Additional macros for calling Lua from a plugin
 */

#pragma once

#include "DataFuncs.h"

#define DFHACK_PLUGIN_LUA_COMMANDS \
    DFhackCExport const DFHack::CommandReg plugin_lua_commands[] =
#define DFHACK_PLUGIN_LUA_FUNCTIONS \
    DFhackCExport const DFHack::FunctionReg plugin_lua_functions[] =
#define DFHACK_PLUGIN_LUA_EVENTS \
    DFhackCExport const DFHack::EventReg plugin_lua_events[] =

#define DFHACK_LUA_COMMAND(name) { #name, name }
#define DFHACK_LUA_FUNCTION(name) { #name, df::wrap_function(name,true) }
#define DFHACK_LUA_EVENT(name) { #name, &name##_event }
#define DFHACK_LUA_END { NULL, NULL }
