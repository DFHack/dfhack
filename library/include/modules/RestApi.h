#pragma once

#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "VersionInfo.h"
#include "Core.h"

//
// lua functions:
//  send_as_json(address, object, complete_callback)
// 

namespace DFHack {;
namespace RestApi {;

int send_as_json(lua_State* state);
int to_json_string(lua_State* state);
int get_iso8601_timestamp(lua_State* state);


}
}
