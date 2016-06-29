#pragma once
#ifndef PERSISTENT_H_INCLUDED
#define PERSISTENT_H_INCLUDED

#include "ColorText.h"
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "jsoncpp.h"
#include "jsoncpp-ex.h"

#include <string>

namespace DFHack {
    namespace Persistent {
        DFHACK_EXPORT Json::Value& getBase();
        void writeToFile(const std::string& filename);
        void readFromFile(const std::string& filename);
        void clear();
        DFHACK_EXPORT Json::Value& get(const std::string& name);
        DFHACK_EXPORT void erase(const std::string& name);
        DFHACK_EXPORT int pushJson(lua_State* L, Json::Value* item);
        DFHACK_EXPORT bool readJson(lua_State* L, Json::Value& item);
        DFHACK_EXPORT int getPersistTable(lua_State* L);
    }
}

#endif
