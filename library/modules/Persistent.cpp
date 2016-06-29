#include "modules/Filesystem.h"
#include "modules/Persistent.h"
#include "modules/World.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace DFHack;
using namespace Persistent;

static Json::Value root(Json::objectValue);

void Persistent::writeToFile(const std::string& filename) {
    std::ofstream out(filename);
    //TODO: assert out
    out << root;
    out.flush();
    out.close();
}

void Persistent::readFromFile(const std::string& filename) {
    if ( DFHack::Filesystem::exists(filename) ) {
        std::ifstream in(filename);
        //TODO: assert in
        in >> root;
        in.close();
    } else {
        root = Json::Value();
    }
}

Json::Value& Persistent::getBase() {
    return root;
}

Json::Value& Persistent::get(const std::string& name) {
    return root[name];
}

void Persistent::erase(const std::string& name) {
    if ( root.isMember(name) )
        root[name].clear();
}

void Persistent::clear() {
    root.clear();
}



/* Helper functions for the Lua interface */

static void convertHistfigEntries(const std::map<std::string,std::map<std::string,std::string>>& entries, const std::string& currentNode, Json::Value& result) {
    auto i = entries.find(currentNode);
    if ( i == entries.end() ) {
        return;
    }
    const std::map<std::string,std::string>& fields = i->second;
    for ( auto j = fields.begin(); j != fields.end(); ++j ) {
        const std::string& fieldName = j->first;
        const std::string& fieldVal2 = j->second;
        std::string fieldVal = fieldVal2.substr(1);
        bool is_ptr = fieldVal2[0] == 'P';
        if ( is_ptr ) {
            Json::Value& substructure = result[fieldName];
            convertHistfigEntries(entries, fieldVal, substructure);
        } else {
            result[fieldName] = fieldVal;
        }
    }
}

static void convertPersistTable(Json::Value& val) {
    /*
        Internal format: all names are prefixed with persist-table
        the nodes with key=persist-table$available are the available indicies
        persist-table$smallest_unused is the smallest unused index
        entry: persist-table<table-index>$$<field-name>, value=table-index of the guy or actual value (int_vals[6] = 1 iff pointer)
        root: persist-tablemastertable
    */
    CoreSuspender suspend;
    std::vector<PersistentDataItem> items;
    World::GetPersistentData(&items,"",true);
    std::string prefix("persist-table");
    std::string separator("$$");

    std::map<std::string,std::map<std::string,std::string>> entries;
    std::vector<int32_t> is_ptr;
    for ( size_t a = 0; a < items.size(); a++ ) {
        PersistentDataItem& item = items[a];
        const std::string& key = item.key();
        const std::string& val = item.val();
        int32_t comp = key.compare(0,prefix.size(),prefix);
        if ( comp != 0 )
            continue;
        size_t startOfSeparator = key.find(separator);
        if ( startOfSeparator == std::string::npos ) {
            World::DeletePersistentData(item);
            continue;
        }
        size_t endOfPrefix = prefix.size();
        size_t endOfSeparator = startOfSeparator+separator.size();
        std::string itemName = key.substr(endOfPrefix,startOfSeparator-prefix.size());
        std::string argName = key.substr(endOfSeparator);
        bool is_ptr = item.ival(5)==1;
        std::string val2 = is_ptr ? ("P" + val) : ("L" + val); //P for ptr, L for literal
        entries[itemName][argName] = val2;
        World::DeletePersistentData(item);
    }

    items.clear();
    std::string startNode = "mastertable";
    convertHistfigEntries(entries, startNode, val);
}

static bool null_terminated(const char* c, size_t size) {
    for ( size_t a = 0; a < size; a++ ) {
        if ( c[a] == static_cast<char>(0) )
            return false;
    }
    return c[size] == static_cast<char>(0);
}

static const char* read_str(lua_State* L, int32_t index) {
    // Read a string value from the Lua stack.
    // Converts other values to string, if necessary.
    size_t size;
    const char* result = lua_tolstring(L,index,&size);
    if ( !null_terminated(result,size) )
        luaL_error(L,"Error: Strings must not contain null bytes");
    return result;
}


/* Json Object Metatable functions */

static inline Json::Value* read_json_object(lua_State* L, int32_t index) {
    // Read a Json object value from the Lua stack.
    Json::Value** ptr = static_cast<Json::Value**>(luaL_checkudata(L,index,"persistent.object"));
    return *ptr;
}

static int32_t object_set(lua_State* L, Json::Value& base) {
    // Take key, value from the top of the stack, and set base[key] = value.
    int32_t index_type = lua_type(L, -2);
    if ( index_type != LUA_TNUMBER && index_type != LUA_TSTRING )
        luaL_error(L, "Error: Bad index type (%s)", luaL_typename(L, -2));
    const char* indexVal = read_str(L, -2);
    
    if (lua_isnil(L, -1))
        // This could add a null value, but persist-table has used it to
        // delete the value (including subtables, if any).
        base.removeMember(indexVal);
    else if (!Persistent::readJson(L, base[indexVal]))
        luaL_error(L, "Error: Bad value type (%s)", luaL_typename(L, -1));
    
    return 0;
}

static int32_t object_index(lua_State* L) {
    Json::Value& base = *read_json_object(L,1);
    
    int32_t type = lua_type(L,2);
    if ( type != LUA_TNUMBER && type != LUA_TSTRING )
        luaL_error(L, "Error: Bad index type (%s)", luaL_typename(L, 2));
    
    const char* arg = read_str(L,2);
    std::string childrenStr("_children");
    if ( childrenStr.compare(arg) == 0 ) {
        // Emulate the ._children behavior of the old persist-table.lua library.
        lua_pop(L,1);
        lua_newtable(L);
        int32_t counter = 0;
        Json::Value::Members members = base.getMemberNames();
        for ( auto i = members.begin(); i != members.end(); ++i ) {
            const std::string& str = *i;
            lua_pushstring(L,str.c_str());
            lua_rawseti(L,-2,counter++);
        }
    } else if (base.isMember(arg)) {
        Json::Value* child = &base[arg];
        lua_pop(L,1);
        Persistent::pushJson(L,child);
    } else {
        lua_pop(L,1);
        lua_pushnil(L);
    }
    
    return 1;
}

static int32_t object_newindex(lua_State* L) {
    Json::Value& base = *read_json_object(L,1);
    return object_set(L, base);
}

static int32_t object_call(lua_State* L) {
    // Return the value at the given position, inserting
    // the default value if that position is unset.
    Json::Value& base = *read_json_object(L,1);
    
    int32_t type = lua_type(L,2);
    if ( type != LUA_TNUMBER && type != LUA_TSTRING )
        luaL_error(L, "Error: Bad index type (%s)", luaL_typename(L, 2));
    const char* key = read_str(L,2);
    
    luaL_checkany(L,3);
    if (!base.isMember(key) || base[key].type() == Json::ValueType::nullValue) {
        Persistent::readJson(L, base[key]);
    }
    
    Persistent::pushJson(L, &base[key]);
    return 1;
}

static int32_t object_len(lua_State* L) {
    Json::Value* val = read_json_object(L,-1);
    int32_t size = (int32_t)val->size();
    lua_pushinteger(L,size);
    return 1;
}

static int32_t object_next(lua_State* L) {
    // Lua iterator for a Json object.
    // Lua iterator semantics: state, old_key => new_key, value
    
    // state['base'] holds the parent json object pointer.
    lua_getfield(L, 1, "base");
    Json::Value& base = *read_json_object(L, -1);
    
    // state['index'] holds the current position in the key vector.
    lua_getfield(L, 1, "index");
    size_t index = lua_tointeger(L, -1);
    
    Json::Value::Members members = base.getMemberNames();
    if (index >= members.size()) {
        // Iteration complete.
        lua_pushnil(L);
        lua_pushnil(L);
    } else {
        // Increment the index in the state.
        lua_pushinteger(L, index + 1);
        lua_setfield(L, 1, "index");
        
        // Push the new key and value.
        std::string key = members[index];
        lua_pushstring(L, key.c_str());
        Persistent::pushJson(L, &base[key]);
    }
    
    return 2;
}

static int32_t object_pairs(lua_State* L) {
    // Initialize an iterator over the members of the object.
    // Json object -> function, state, nil
    Json::Value* val = read_json_object(L, -1);
    lua_pushcfunction(L, object_next);
    
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "index");
    Persistent::pushJson(L, val);
    lua_setfield(L, -2, "base");
    
    lua_pushnil(L);
    return 3;
}

static int32_t object_tostring(lua_State* L) {
    Json::Value& val = *read_json_object(L,-1);
    stringstream out;
    out << val;
    std::string result = out.str();
    lua_pushstring(L,result.c_str());
    return 1;
}

static int32_t init_object_metatable(lua_State* L) {
    // Initialize the metatable for the json wrapper.
    if ( luaL_newmetatable(L,"persistent.object") == 0 ) {
        return 1;
    }
    lua_pushcfunction(L,object_index);
    lua_setfield(L,-2,"__index");
    lua_pushcfunction(L,object_newindex);
    lua_setfield(L,-2,"__newindex");
    lua_pushcfunction(L,object_call);
    lua_setfield(L,-2,"__call");
    lua_pushcfunction(L,object_len);
    lua_setfield(L,-2,"__len");
    lua_pushcfunction(L,object_pairs);
    lua_setfield(L,-2,"__pairs");
    lua_pushcfunction(L,object_pairs);
    lua_setfield(L,-2,"__ipairs");
    lua_pushcfunction(L,object_tostring);
    lua_setfield(L,-2,"__tostring");
    return 1;
}


/* Json Array Metatable functions */

static inline Json::Value* read_json_array(lua_State* L, int32_t index) {
    // Read a Json array value from the Lua stack.
    Json::Value** ptr = static_cast<Json::Value**>(luaL_checkudata(L,index,"persistent.array"));
    return *ptr;
}

static int read_array_key(lua_State* L, int index, int maximum) {
    int isnum;
    auto key = lua_tointegerx(L, index, &isnum);
    if ( !isnum )
        luaL_error(L, "Error: Bad index type (%s)", luaL_typename(L, index));
    if ( key < 0 || key > maximum )
        luaL_error(L, "Error: Index out of bounds (%d)", key);
    return key;
}

static int32_t array_index(lua_State* L) {
    Json::Value& base = *read_json_array(L, 1);
    auto indexVal = read_array_key(L, -1, base.size() - 1);
    lua_pop(L, 1);
    
    Json::Value* child = &base[indexVal];
    Persistent::pushJson(L,child);
    return 1;
}

static int32_t array_newindex(lua_State* L) {
    Json::Value& base = *read_json_array(L,1);
    auto indexVal = read_array_key(L, -2, base.size());
    if (!Persistent::readJson(L, base[indexVal]))
        luaL_error(L, "Error: Bad value type (%s)", luaL_typename(L, -1));
    return 0;
}

static int32_t array_call(lua_State* L) {
    // Return the value at the given position, inserting
    // the default value if that position is unset.
    Json::Value& base = *read_json_array(L,1);
    auto indexVal = read_array_key(L, 2, base.size());
    luaL_checkany(L,3);
    if (indexVal >= base.size() || base[indexVal].type() == Json::ValueType::nullValue) {
        Persistent::readJson(L, base[indexVal]);
    }
    
    Persistent::pushJson(L, &base[indexVal]);
    return 1;
}

static int32_t array_len(lua_State* L) {
    Json::Value* val = read_json_array(L,-1);
    int32_t size = (int32_t)val->size();
    lua_pushinteger(L,size);
    return 1;
}

static int32_t array_next(lua_State* L) {
    // Lua iterator for a Json array.
    // Lua iterator semantics: state, old_key => new_key, value
    
    // The state is the parent json array pointer.
    Json::Value& base = *read_json_array(L, 1);
    
    // The key is either nil or an index number.
    auto index = 0;
    if ( !lua_isnil(L, -1) ) {
        int isnum;
        index = lua_tointegerx(L, -1, &isnum) + 1;
        if (!isnum)
            luaL_error(L, "Error: Bad index type (%s)", luaL_typename(L, -1));
    }
    
    lua_pop(L, 1);
    
    if (index >= base.size()) {
        // Iteration complete.
        lua_pushnil(L);
        lua_pushnil(L);
    } else {
        // Push the new key and value.
        lua_pushinteger(L, index);
        Persistent::pushJson(L, &base[index]);
    }
    
    return 2;
}

static int32_t array_pairs(lua_State* L) {
    // Initialize an iterator over the members of the array.
    // Json array -> function, state, nil
    lua_pushcfunction(L, array_next);
    lua_insert(L, -2);
    lua_pushnil(L);
    return 3;
}

static int32_t array_tostring(lua_State* L) {
    Json::Value& val = *read_json_array(L,-1);
    stringstream out;
    out << val;
    std::string result = out.str();
    lua_pushstring(L,result.c_str());
    return 1;
}

static int32_t init_array_metatable(lua_State* L) {
    // Initialize the metatable for the json wrapper.
    if ( luaL_newmetatable(L,"persistent.array") == 0 ) {
        return 1;
    }
    lua_pushcfunction(L,array_index);
    lua_setfield(L,-2,"__index");
    lua_pushcfunction(L,array_newindex);
    lua_setfield(L,-2,"__newindex");
    lua_pushcfunction(L,array_call);
    lua_setfield(L,-2,"__call");
    lua_pushcfunction(L,array_len);
    lua_setfield(L,-2,"__len");
    lua_pushcfunction(L,array_pairs);
    lua_setfield(L,-2,"__pairs");
    lua_pushcfunction(L,array_pairs);
    lua_setfield(L,-2,"__ipairs");
    lua_pushcfunction(L,array_tostring);
    lua_setfield(L,-2,"__tostring");
    return 1;
}


/* Lua interface */

int32_t Persistent::pushJson(lua_State* L, Json::Value* item) {
    // Push a Json value onto the Lua stack.
    switch (item->type()) {
        case Json::ValueType::nullValue:
            lua_pushnil(L);
            break;
        case Json::ValueType::intValue:
            lua_pushnumber(L,item->asInt());
            break;
        case Json::ValueType::uintValue:
            lua_pushnumber(L,item->asUInt());
            break;
        case Json::ValueType::realValue:
            lua_pushnumber(L,item->asDouble());
            break;
        case Json::ValueType::stringValue:
            lua_pushstring(L,item->asString().c_str());
            break;
        case Json::ValueType::booleanValue:
            lua_pushboolean(L,item->asBool());
            break;
        case Json::ValueType::arrayValue:
            *static_cast<Json::Value**>(lua_newuserdata(L,sizeof(Json::Value*))) = item;
            init_array_metatable(L);
            lua_setmetatable(L,-2);
            break;
        case Json::ValueType::objectValue:
            *static_cast<Json::Value**>(lua_newuserdata(L,sizeof(Json::Value*))) = item;
            init_object_metatable(L);
            lua_setmetatable(L,-2);
            break;
    }
    return 1;
}

bool Persistent::readJson(lua_State* L, Json::Value& result) {
    // Read a Json userdata value from the top of the Lua stack.
    int32_t value_type = lua_type(L, -1);
    switch ( value_type ) {
        case LUA_TBOOLEAN:
            result = !!lua_toboolean(L, -1);
            break;
        case LUA_TLIGHTUSERDATA:
            // Allow use of df.NULL to insert a null value.
            if (lua_touserdata(L, -1) == NULL)
                result = Json::ValueType::nullValue;
            else
                return false;
            break;
        case LUA_TNIL:
            result = Json::ValueType::nullValue;
            break;
        case LUA_TNUMBER:
            result = lua_tonumber(L, -1);
            break;
        case LUA_TSTRING:
            result = read_str(L, -1);
            break;
        case LUA_TTABLE:
            // Initialize a Json object with (key,value) pairs from the table.
            // Made trickier by the inability to use lua_tolstring on the key.
            result = Json::ValueType::objectValue;
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                lua_pushvalue(L, -2);
                lua_insert(L, -2);
                object_set(L, result);
                lua_pop(L, 2);
            }
            
            break;
        case LUA_TUSERDATA:
            {
                auto ptr = luaL_testudata(L, -1, "persistent.object");
                if (!ptr)
                    ptr = luaL_testudata(L, -1, "persistent.array");
                if (!ptr)
                    return false;
                Json::Value** val = static_cast<Json::Value**>(ptr);
                result = **val;
            }
            break;
        default:
            return false;
    }
    
    return true;
}

int Persistent::getPersistTable(lua_State* L) {
    // Get the persist-table root table.
    // Convert from the historical entity representation if necessary.
    static const int32_t persist_version = 1;
    Json::Value& val = Persistent::get("persist-table");
    int32_t version = Json::get<int>(val, "version", 0);
    if ( version == 0 ) {
        convertPersistTable(val);
        val["version"] = persist_version;
    }
    Persistent::pushJson(L, &val);
    return 1;
}
