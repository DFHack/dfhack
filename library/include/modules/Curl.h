#pragma once

#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "VersionInfo.h"
#include "Core.h"

namespace DFHack {;
namespace Curl {;

int send_as_json(lua_State* state);
int to_json_string(lua_State* state);
int get_iso8601_timestamp(lua_State* state);

}
}
