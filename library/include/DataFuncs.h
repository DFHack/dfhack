/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <type_traits>

#include "DataIdentity.h"
#include "LuaWrapper.h"

namespace DFHack {
    class color_ostream;

    namespace Lua {
        template<typename T>
        T Get(lua_State *L, int idx) {
            T val;
            df::identity_traits<T>::get()->lua_write(L, UPVAL_METHOD_NAME, &val, idx);
            return val;
        }
    }
}

namespace df {
    class DFHACK_EXPORT cur_lua_ostream_argument {
        DFHack::color_ostream *out;
    public:
        cur_lua_ostream_argument(lua_State *state);
        operator DFHack::color_ostream& () { return *out; }
    };

    template<class T> struct return_type {};

    template<typename rT, typename ...aT>
    struct return_type<rT (*)(aT...)>{
        typedef rT type;
        static const bool is_method = false;
    };

    template<typename rT, class CT, typename ...aT>
    struct return_type<rT (CT::*)(aT...)>{
        typedef rT type;
        typedef CT class_type;
        static const bool is_method = true;
    };


    template<typename rT, class CT, typename ...aT>
    struct return_type<rT (CT::*)(aT...) const>{
        typedef rT type;
        typedef CT class_type;
        static const bool is_method = true;
    };

    template<typename RT, typename... AT, typename FT, typename... ET, std::size_t... I>
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
    void call_and_push_impl(lua_State* L, int base, std::index_sequence<I...>, FT fun, ET... extra)
    {
        if constexpr (std::is_same_v<RT, void>) {
            std::invoke(fun, extra..., (DFHack::Lua::Get<AT>(L, base+I))...);
        }
        else
        {
            RT rv = std::invoke(fun, extra..., (DFHack::Lua::Get<AT>(L, base+I))...);
            df::identity_traits<RT>::get()->lua_read(L, UPVAL_METHOD_NAME, &rv);
        }
    }

    template<typename RT, typename... AT, typename FT, typename... ET, typename indices = std::index_sequence_for<AT...> >
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
    void call_and_push(lua_State* L, int base, FT fun, ET... extra)
    {
        call_and_push_impl<RT, AT...>(L, base, indices{}, fun, extra...);
    }

    template<typename T> struct function_wrapper {};

    template<typename rT, typename ...aT>
    struct function_wrapper<rT(*)(DFHack::color_ostream&, aT...)> {
        static const int num_args = sizeof...(aT);
        static void execute(lua_State *L, int base, rT (fun)(DFHack::color_ostream& out, aT...)) {
             cur_lua_ostream_argument out(L);
             call_and_push<rT, aT...>(L, base, fun, out);
        }
    };

    template<typename rT, typename ...aT>
    struct function_wrapper<rT(*)(aT...)> {
        static const int num_args = sizeof...(aT);
        static void execute(lua_State *L, int base, rT (fun)(aT...)) {
            call_and_push<rT, aT...>(L, base, fun);
        }
    };

    template<typename rT, class CT, typename ...aT>
    struct function_wrapper<rT(CT::*)(aT...)> {
        static const int num_args = sizeof...(aT)+1;
        static void execute(lua_State *L, int base, rT(CT::*mem_fun)(aT...)) {
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<rT, aT...>(L, base, mem_fun, self);
        };
    };

    template<typename rT, class CT, typename ...aT>
    struct function_wrapper<rT(CT::*)(aT...) const> {
        static const int num_args = sizeof...(aT)+1;
        static void execute(lua_State *L, int base, rT(CT::*mem_fun)(aT...) const) {
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<rT, aT...>(L, base, mem_fun, self);
        };
    };

    template<class T>
    class function_identity : public function_identity_base {
        T ptr;

    public:
        typedef function_wrapper<T> wrapper;

        function_identity(T ptr, bool vararg)
            : function_identity_base(wrapper::num_args, vararg), ptr(ptr) {};

        virtual void invoke(lua_State *state, int base) { wrapper::execute(state, base, ptr); }
    };

    template<class T>
    inline function_identity_base *wrap_function(T ptr, bool vararg = false) {
        // bah, but didn't have any idea how to allocate statically
        return new function_identity<T>(ptr, vararg);
    }
}
