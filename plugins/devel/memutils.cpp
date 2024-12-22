#include <string>

#include "LuaTools.h"
#include "LuaWrapper.h"

using namespace DFHack;
using namespace DFHack::LuaWrapper;
using namespace std;

namespace memutils {
    static lua_State *state;
    static color_ostream_proxy *out;

    struct initializer {
        Lua::StackUnwinder *unwinder;
        initializer()
        {
            if (!out)
                out = new color_ostream_proxy(Core::getInstance().getConsole());
            if (!state)
                state = Lua::Open(*out);
            unwinder = new Lua::StackUnwinder(state);
        }
        ~initializer()
        {
            delete unwinder;
        }
    };

    struct cleaner {
        ~cleaner()
        {
            if (state)
            {
                lua_close(state);
                state = NULL;
            }
            if (out)
            {
                delete out;
                out = NULL;
            }
        }
    };

    static cleaner g_cleaner;

    void *lua_expr_to_addr(const char *expr)
    {
        initializer init;
        Lua::PushModulePublic(*out, state, "utils", "df_expr_to_ref");
        lua_pushstring(state, expr);
        if (!Lua::SafeCall(*out, state, 1, 1))
        {
            out->printerr("Failed to evaluate %s\n", expr);
            return NULL;
        }

        Lua::PushModulePublic(*out, state, "utils", "addressof");
        lua_swap(state);
        if (!Lua::SafeCall(*out, state, 1, 1) || !lua_isinteger(state, -1))
        {
            out->printerr("Failed to get address: %s\n", expr);
            return NULL;
        }

        auto addr = uintptr_t(lua_tointeger(state, -1));
        return (void*)addr;
    }
}
