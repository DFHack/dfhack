#include <vector>
#include <map>
#include <string>

#include "modules/RestApi.h"
#include "LuaTools.h"

#include "json/json.h"
#include "tinythread.h"
#include "lua.h"
#include "lauxlib.h"
#define CURL_STATICLIB
#include "curl/curl.h"

__pragma(optimize("",off))

namespace DFHack { ;
namespace RestApi { ;

static const char* const s_module_name = "RestApi";

namespace {	;
    
enum LUA_VARIANT_TYPE : uint32_t
{
    LUA_VARIANT_DOUBLE,
    LUA_VARIANT_INT,
    LUA_VARIANT_POINTER,
    LUA_VARIANT_STRING,
    LUA_VARIANT_BOOL,
    LUA_VARIANT_FUNCTION,
    LUA_VARIANT_FUNCTION_REF,
    LUA_VARIANT_ITABLE,
    LUA_VARIANT_KTABLE,
    LUA_VARIANT_NIL
};

#define CONST_TO_STRING_SWITCH(c) case c: { return #c; }

typedef Json::Value LuaVariant;
typedef double LuaVariantDouble;
typedef LUA_INTEGER LuaVariantInt;
typedef void* LuaVariantPointer;
typedef std::string LuaVariantString;
typedef bool LuaVariantBool;
typedef lua_CFunction LuaVariantFunction;
typedef std::vector<LuaVariant> LuaVariantITable;
typedef std::map<LuaVariant, LuaVariant> LuaVariantKTable;

static LuaVariant FromFunctionRef(LuaVariantInt ref)
{
    return LuaVariant{"<function ptr>"};
    //LuaVariant v;
    //v.type = LUA_VARIANT_FUNCTION_REF;
    //v.valueInt = ref;
    //return v;
}

static Json::Value parse(lua_State *L, int32_t index, bool parseNumbersAsDouble = false, bool pop = true)
{

    LuaVariant v;

    auto type = lua_type(L, index);
    if (type == LUA_TUSERDATA || type == LUA_TLIGHTUSERDATA)
    {
        LuaVariantPointer value = (LuaVariantPointer)lua_touserdata(L, index);
        v = LuaVariant(value);
    }
    else if (type == LUA_TNUMBER)
    {
        if (parseNumbersAsDouble)
        {
            LuaVariantDouble value = lua_tonumber(L, index);
            v = LuaVariant(value);
        }
        else
        {
            LuaVariantInt value = lua_tointeger(L, index);
            v = LuaVariant(value);
        }
    }
    else if (type == LUA_TSTRING)
    {
        LuaVariantString value = lua_tostring(L, index);
        v = LuaVariant(value);
    }
    else if (type == LUA_TBOOLEAN)
    {
        LuaVariantBool value = (lua_toboolean(L, index) == 1);
        v = LuaVariant(value);
    }
    else if (type == LUA_TFUNCTION)
    {
        // copy the function since we're about to pop it off
        lua_pushvalue(L, index);
        LuaVariantInt index = luaL_ref(L, LUA_REGISTRYINDEX);
        v = FromFunctionRef(index);
    }
    else if (type == LUA_TTABLE)
    {
        // if the indexds is in the negative, parsing a table
        // will put values "above" it, moving it down by 1 negative index
        if (index < 0) index--;

        static const int32_t KEY_INDEX = -2;
        static const int32_t VALUE_INDEX = -1;
        typedef std::function<void(lua_State*)> WalkCallback;


        // The table parsing loop is conducted by lua_next(), which:
        //    - reads a key from the stack
        //    - read the value of the next key from the table (table index specified by second arg)
        //    - pushes the key (KEY_INDEX) and value (VALUE_INDEX) to the stack
        //    - returns 0 and pops all extraneous stuff on completion
        // Starting with a key of nil will allow us to process the entire table.
        // Leaving the last key on the stack each iteration will go to the next item
        auto walk = [](lua_State *L, int32_t index, WalkCallback callback) -> void
        {
            lua_pushnil(L);
            while (lua_next(L, index))
            {
                callback(L);
                lua_pop(L, 1);
            }
        };

        // we're going to walk the table one time to see if we can use an LuaVariantITable,
        // or if it we need a LuaVariantKTable.
        bool requiresStringKeys = false;
        walk(L, index, [&requiresStringKeys](lua_State *L) -> void
        {
            if (lua_type(L, KEY_INDEX) == LUA_TSTRING)
                requiresStringKeys = true;
        });

        // the below are variations, since the table might have only numeric indices (iTable)
        // or it might also have string indices (kTable)
        //LuaVariantKTable kTableValue;
        //LuaVariantITable iTableValue;
        auto parseKTableItem =
            [parseNumbersAsDouble, &v](lua_State *L) -> void
        {
            // TODO: probably type-check the key here and have some error handling mechanism
            std::string key = lua_tostring(L, KEY_INDEX);
            int32_t valueIndex = (lua_type(L, VALUE_INDEX) == LUA_TTABLE) ? lua_gettop(L) : VALUE_INDEX;
            v[key] = parse(L, valueIndex, parseNumbersAsDouble, false);
        };
        auto parseITableItem =
            [parseNumbersAsDouble, &v](lua_State *L) -> void
        {
            // TODO: probably type-check the key here and have some error handling mechanism
            auto key = lua_tonumber(L, KEY_INDEX) - 1; // -1 because lua is 1-indexed and we are 0-indexed
            int32_t valueIndex = (lua_type(L, VALUE_INDEX) == LUA_TTABLE) ? lua_gettop(L) : VALUE_INDEX;

            //// fill with nil values up to key
            //while (key >= v.size())
            //	iTableValue.push_back(LuaVariant());
            v[static_cast<int>(key)] = parse(L, valueIndex, parseNumbersAsDouble, false);
        };

        if (requiresStringKeys)
        {
            v = LuaVariant(Json::objectValue);
            walk(L, index, parseKTableItem);
            //v = LuaVariant(kTableValue);
        }
        else
        {
            v = LuaVariant(Json::arrayValue);
            walk(L, index, parseITableItem);
            //v = LuaVariant(iTableValue);
        }
    }

    //if (pop) // TODO: verify this works as expected .. don't think so
        //lua_pop(L, index);
    return v;
}

}

static int print(lua_State *S, const std::string& str)
{
    if (color_ostream *out = Lua::GetOutput(S))
        out->print("%s\n", str.c_str());//*out << str;
    else
        Core::print("%s\n", str.c_str());
    return 0;
}

struct SendThread
{	
    std::shared_ptr<CURL> curl;
    std::unique_ptr<tthread::thread> post_thread;

    struct Post
    {
        std::string url;
        Json::Value value;
    };

    tthread::mutex post_queue_mutex;
    tthread::condition_variable post_queue_items_present;
    std::vector<Post> post_queue;
    bool terminate_post_thread = false;
    bool debug = false;

    // Make it a singleton for now.
    static SendThread& get()
    {
        static std::shared_ptr<SendThread> instance = std::make_shared<SendThread>();
        return *instance;
    }
    
    SendThread()
    {
        auto curl_ptr = curl_easy_init();
        if (curl_ptr == nullptr)
        {
            Core::printerr("%s init failed: error initializing libcurl\n", s_module_name);
            return;
        }
        curl = std::shared_ptr<CURL>(curl_ptr, [](CURL* ptr) { curl_easy_cleanup(ptr); });
        post_thread = std::make_unique<tthread::thread>(&post_thread_fn, this);
    }

    ~SendThread()
    {
        terminate_post_thread = true;
        post_queue_items_present.notify_all();
        post_thread.reset();

        curl.reset();
    }

    void send(const std::string& url, const Json::Value& value/*, color_ostream& out, bool debug*/)
    {
        tthread::lock_guard<tthread::mutex> guard(post_queue_mutex);
        post_queue.push_back({ url, value });
        post_queue_items_present.notify_all();
    }

private:
    static void post_thread_fn(void* this_raw)
    {
        auto this_ptr = static_cast<SendThread*>(this_raw);

        std::vector<Post> curr_post_queue;
        while (!this_ptr->terminate_post_thread)
        {
            {
                tthread::lock_guard<tthread::mutex> guard(this_ptr->post_queue_mutex);
                if (this_ptr->post_queue.empty())
                {
                    this_ptr->post_queue_items_present.wait(this_ptr->post_queue_mutex);
                }
                if (this_ptr->debug) 
                {
                    Core::print("%s %d items to send on post thread\n", s_module_name, this_ptr->post_queue.size());
                }
                curr_post_queue.swap(this_ptr->post_queue);
            }
            for (auto&& post : curr_post_queue)
            {
                this_ptr->post_value(post.url, post.value);
            }
            curr_post_queue.clear();
        }
    }

    void post_value(const std::string& url, const Json::Value& value)
    {
        CURLcode res;
        std::string str = value.toStyledString();
        //char *postFields = ;
        curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, str.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, str.length());
        if (debug)
        {
            Core::print("%s Sending to %s...\n%s\n", s_module_name, url.c_str(), str.c_str());
        }
        res = curl_easy_perform(curl.get());
        if (res != CURLE_OK)
        {
            Core::print("%s Error sending to %s: %s\n", s_module_name, url.c_str(), curl_easy_strerror(res));
        }
        if (debug)
        {
            Core::print("%s ...done\n", s_module_name);
        }
    }
};

int send_as_json(lua_State* state)
{
    if (lua_gettop(state) != 2)
    {
        Core::printerr("Usage: send_as_json(url, object)\n");
        return 0;
    }
    auto url = lua_tostring(state, 1);
    auto val = parse(state, 2);
    lua_pop(state, 2);
    SendThread::get().send(url, val);

    return 0;
}

int to_json_string(lua_State* state)
{
    if (lua_gettop(state) != 1)
    {
        Core::printerr("Usage: to_json_string(object)\n");
        return 0;
    }
    auto val = parse(state, 1);
    lua_pop(state, 1);
    lua_pushstring(state, val.toStyledString().c_str());
    return 1;
}

std::string get_timestamp()
{
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    return buf;
}

int get_iso8601_timestamp(lua_State* state)
{
    if (lua_gettop(state) != 0)
    {
        Core::printerr("Usage: get_iso8601_timestamp()\n");
        return 0;
    }
    lua_pushstring(state, get_timestamp().c_str());
    return 1;
}

}

}