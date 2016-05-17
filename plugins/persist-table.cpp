#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Persistent.h"
#include "modules/World.h"

#include "jsoncpp.h"

//#include "df/world.h"

#include <map>
#include <sstream>
#include <vector>
using namespace std;

using namespace DFHack;

DFHACK_PLUGIN("persist-table");
REQUIRE_GLOBAL(world);
static const int32_t persist_version=1;

//command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    /*commands.push_back(PluginCommand(
        "skeleton2",
        "shortHelpString",
        skeleton2,
        false, //allow non-interactive use
        "longHelpString"
    ));*/
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

/*command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    out.print("blah");
    return CR_OK;
}*/

int32_t depth;
void convertHistfigsHelper(color_ostream& out, const map<string,map<string,string>>& entries, const string& currentNode, Json::Value& result) {
    auto i = entries.find(currentNode);
    if ( i == entries.end() ) {
        return;
    }
    //out << "{" << endl;
    const map<string,string>& fields = i->second;
    for ( auto j = fields.begin(); j != fields.end(); ++j ) {
        const string& fieldName = j->first;
        const string& fieldVal2 = j->second;
        string fieldVal = fieldVal2.substr(1);
        bool is_ptr = fieldVal2[0] == 'P';
        //out << fieldName << " : ";
        if ( is_ptr ) {
            Json::Value& substructure = result[fieldName];
            convertHistfigsHelper(out,entries,fieldVal,substructure);
        } else {
            //out << fieldVal << endl;
            result[fieldName] = fieldVal;
        }
    }
    //out << "}" << endl;
}

void convertHistfigs(color_ostream& out) {
    /*
Internal format: all names are prefixed with persist-table
the nodes with key=persist-table$available are the available indicies
persist-table$smallest_unused is the smallest unused index

entry: persist-table<table-index>$$<field-name>, value=table-index of the guy or actual value (int_vals[6] = 1 iff pointer)

root: persist-tablemastertable
    */
    vector<PersistentDataItem> items;
    World::GetPersistentData(&items,"",true);
    string prefix("persist-table");
    string separator("$$");

    map<string,map<string,string>> entries;
    vector<int32_t> is_ptr;
    for ( size_t a = 0; a < items.size(); a++ ) {
        PersistentDataItem& item = items[a];
        const string& key = item.key();
        const string& val = item.val();
        //out << __FILE__ << ":" << __LINE__ << ": " << key << " = " << val << endl;
        int32_t comp = key.compare(0,prefix.size(),prefix);
        //out << __FILE__ << ":" << __LINE__ << ": comp = " << comp << endl;
        if ( comp != 0 )
            continue;
        //out << __FILE__ << ":" << __LINE__ << endl;
        size_t startOfSeparator = key.find(separator);
        if ( startOfSeparator == string::npos ) {
            World::DeletePersistentData(item);
            continue;
        }
        //out << __FILE__ << ":" << __LINE__ << endl;
        size_t endOfPrefix = prefix.size();
        size_t endOfSeparator = startOfSeparator+separator.size();
        string itemName = key.substr(endOfPrefix,startOfSeparator-prefix.size());
        string argName = key.substr(endOfSeparator);
        bool is_ptr = item.ival(5)==1;
        string val2 = is_ptr ? ("P" + val) : ("L" + val); //P for ptr, L for literal
        //out << __FILE__ << ":" << __LINE__ << ": entries[" << itemName << "][" << argName << "] = " << val2 << endl;
        entries[itemName][argName] = val2;
        World::DeletePersistentData(item);
    }
    items.clear();

    Json::Value& val = Persistent::get("persist-table");
    string startNode = "mastertable";
    convertHistfigsHelper(out, entries, startNode, val);
    //out << endl << "val = " << val << endl;
    //out << endl << "base = " << Persistent::getBase() << endl << endl;
}

static void load_config(color_ostream& out) {
    Json::Value& val = Persistent::get("persist-table");
    int32_t version = val["version"].isInt() ? val["version"].asInt() : 0;
    if ( version == 0 ) {
        convertHistfigs(out);
        val["version"] = persist_version;
    } else if ( version == 1 ) {
    } else {
        out << __FILE__ << ":" << __LINE__ << ": invalid version: " << version << " (current version = " << persist_version << ")" << endl;
        exit(1);
    }
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        load_config(out);
        break;
    case DFHack::SC_MAP_UNLOADED:
        break;
    default:
        break;
    }
    return CR_OK;
}

static bool null_terminated(const char* c, size_t size) {
    for ( size_t a = 0; a < size; a++ ) {
        if ( c[a] == static_cast<char>(0) )
            return false;
    }
    return c[size] == static_cast<char>(0);
}

static int32_t init(lua_State* L);
static int32_t assign_wrapper_metatable(lua_State* L);
static inline int32_t make_json(lua_State* L, Json::Value* v);
static int32_t empty(lua_State* L);
static int32_t getRoot(lua_State* L) {
    //TODO: has_child, is_null
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value& val = Persistent::get("persist-table");
    make_json(L,&val);
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": &val = 0x" << static_cast<void*>(&val) << endl;
    if ( val.type() != Json::objectValue ) {
        string blah("aeoigjaeogijeaogjeaogj");
        val[blah] = "hello";
        val.removeMember(blah);
    }
    return 1;
}
static inline int32_t assert_json_wrapper(lua_State* L, int32_t index) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    //stringstream bob;
    init(L);
    lua_pop(L,1);
    luaL_checktype(L,index,LUA_TUSERDATA);
    luaL_checkudata(L,index,"persist-table.json_wrapper");
    return 0;
}
static inline Json::Value* read_json(lua_State* L, int32_t index) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    assert_json_wrapper(L,index);
    Json::Value** ptr = static_cast<Json::Value**>(lua_touserdata(L,index));
    Json::Value* result = *ptr;
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": result = 0x" << static_cast<void*>(result) << endl;
    return result;
}
static inline int32_t make_json(lua_State* L, Json::Value* v) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value** ptr = static_cast<Json::Value**>(lua_newuserdata(L,sizeof(Json::Value*)));
    *ptr = v;
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": *ptr = 0x" << static_cast<void*>(*ptr) << endl;
    assign_wrapper_metatable(L);
    return 1;
}

static int32_t len(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value* val = read_json(L,-1);
    int32_t size = (int32_t)val->size();
    lua_pushinteger(L,size);
    return 1;
}

static const char* read_str(lua_State* L, int32_t index) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    size_t size;
    const char* result = lua_tolstring(L,index,&size);
    if ( !null_terminated(result,size) )
        luaL_error(L,"Error: in persist-table.cpp::read_str: string is not both null-terminated and nonzero before its end");
    return result;
}

static int32_t index(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value& base = *read_json(L,1);
    Json::Value* child = NULL;
    int32_t type = lua_type(L,2);
    string childrenStr("_children");
    string emptyStr("empty");

    //check if it's in our metatable already
    lua_getmetatable(L,1); //oldsize+1
    lua_pushvalue(L,2); //oldsize+2
    lua_rawget(L,-2); //oldsize+2
    bool is_nil = lua_isnil(L,-1);
    if ( !is_nil ) {
        lua_remove(L,-2); //pop metatable
        lua_remove(L,-2); //pop index string
        //leave the base table
        return 1;
    } else {
        lua_pop(L,2); //result and metatable
    }

    if ( type != LUA_TNUMBER && type != LUA_TSTRING )
        luaL_error(L,"Error: persist-table.cpp::__index: index argument is a bad type: %d",type);
    if ( type == LUA_TNUMBER ) {
        int32_t index = lua_tonumber(L,2)-1;
        if ( index < 0 || index >= base.size() )
            luaL_error(L,"Error: in persist-table.cpp::__index int argument is out of range: %d (size = %d)",index,(int32_t)base.size());
        child = &base[index];
    } else {
        const char* arg = read_str(L,2);
        if ( childrenStr.compare(arg) == 0 ) {
            lua_pop(L,1);
            lua_newtable(L);
            //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": base.size() = " << base.size() << endl;
            int32_t counter = 0;
            Json::Value::Members members = base.getMemberNames();
            for ( auto i = members.begin(); i != members.end(); ++i ) {
                const std::string& str = *i;
                lua_pushstring(L,str.c_str());
                lua_rawseti(L,-2,counter);
                counter++;
            }
            return 1;
        } else if ( emptyStr.compare(arg) == 0 ) {
            lua_pop(L,1);
            lua_getmetatable(L,1);
            lua_pushstring(L,"empty");
            lua_rawget(L,-2);
            lua_remove(L,-2);
            return 1;
        }
        child = &base[arg];
    }
    lua_pop(L,1);
    make_json(L,child);
    return 1;
}

static int32_t newindex(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value& base = *read_json(L,1);
    int32_t rhs_type = lua_type(L,3);
    if ( rhs_type != LUA_TNUMBER && rhs_type != LUA_TBOOLEAN && rhs_type != LUA_TSTRING && rhs_type != LUA_TUSERDATA && rhs_type != LUA_TNIL )
        luaL_error(L,"Error: in persist-table.cpp, __newindex: rhs_type is wrong (%d)",rhs_type);
    int32_t index_type = lua_type(L,2);
    if ( index_type != LUA_TNUMBER && index_type != LUA_TSTRING )
        luaL_error(L,"Error: in persist-table.cpp, __newindex: index_type is wrong (%d)",index_type);
    if ( rhs_type == LUA_TNIL ) {
        if ( index_type == LUA_TNUMBER ) {
            int32_t i = lua_tonumber(L,2)-1;
            if ( i < 0 || i >= base.size() )
                luaL_error(L,"Error in persist-table.cpp::__newindex: index %d is out of bounds (size=%d)",i,(int32_t)base.size());
            Json::Value removed;
            base.removeIndex(i,&removed);
        } else {
            const char* indexVal = read_str(L,2);
            base.removeMember(indexVal);
        }
        lua_pop(L,3);
        return 0;
    }
    Json::Value* child = NULL;
    if ( index_type == LUA_TNUMBER ) {
        int32_t i = lua_tonumber(L,2)-1;
        if ( i < 0 || i > base.size() )
            luaL_error(L,"Error in persist-table.cpp::__newindex: index %d is out of bounds (size=%d)",i,(int32_t)base.size());
        child = &base[i];
    } else {
        const char* indexVal = read_str(L,2);
        child = &base[indexVal];
    }

    if ( rhs_type == LUA_TUSERDATA ) {
        Json::Value* rhs = read_json(L,3);
        *child = *rhs;
    } else {
        const char* rhsVal = read_str(L,3);
        *child = rhsVal;
    }
    lua_pop(L,3);
    return 0;
}

static int32_t tostring(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    Json::Value& val = *read_json(L,-1);
    lua_pop(L,1);
    stringstream out;
    out << val;
    string result = out.str();
    lua_pop(L,1);
    lua_pushstring(L,result.c_str());
    return 1;
}

static int32_t assign_wrapper_metatable(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    luaL_checktype(L,-1,LUA_TUSERDATA);
    init(L);
    lua_setmetatable(L,-2);
    return 0;
}

static int32_t empty(lua_State* L) {
    Json::Value& base = *read_json(L,1);
    bool result = base.empty();
    lua_pop(L,1);
    lua_pushboolean(L,result);
    return 1;
}

static int32_t get_type(lua_State* L) {
    int32_t top = lua_gettop(L);
    if ( top != 1 ) {
        stringstream bob;
        bob << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ": #args = " << top << " != 1" << endl;
        string s(bob.str());
        luaL_error(L,s.c_str());
    }
    Json::Value* val = read_json(L,1);
    int32_t type = (int32_t)val->type();
    string str;
    switch(type) {
    case Json::nullValue   : str = string("nullValue"   ); break;
    case Json::intValue    : str = string("intValue"    ); break;
    case Json::uintValue   : str = string("uintValue"   ); break;
    case Json::realValue   : str = string("realValue"   ); break;
    case Json::stringValue : str = string("stringValue" ); break;
    case Json::booleanValue: str = string("booleanValue"); break;
    case Json::arrayValue  : str = string("arrayValue"  ); break;
    case Json::objectValue : str = string("objectValue" ); break;
    default: break;
    }
    lua_pushstring(L,str.c_str());
    return 1;
}

static int32_t init(lua_State* L) {
    //cerr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << endl;
    //initialize the metatable for the wrapper to json and return it
    if ( luaL_newmetatable(L,"persist-table.json_wrapper") == 0 ) {
        return 1;
    }
    lua_pushcfunction(L,index);
    lua_setfield(L,-2,"__index");
    lua_pushcfunction(L,newindex);
    lua_setfield(L,-2,"__newindex");
    lua_pushcfunction(L,len);
    lua_setfield(L,-2,"__len");
    lua_pushcfunction(L,tostring);
    lua_setfield(L,-2,"__tostring");
    lua_pushcfunction(L,empty);
    lua_setfield(L,-2,"empty");
    lua_pushcfunction(L,get_type);
    lua_setfield(L,-2,"getType");
    return 1;
}
/*
DFHACK_PLUGIN_LUA_FUNCTIONS takes real args
DFHACK_PLUGIN_LUA_COMMANDS takes lua_State*

which ones are allowed up-values?
*/
DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getRoot),
    DFHACK_LUA_END
};