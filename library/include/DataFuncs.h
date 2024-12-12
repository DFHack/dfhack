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

#include "ColorText.h"
#include "DataIdentity.h"
#include "LuaWrapper.h"

namespace df {
    class DFHACK_EXPORT cur_lua_ostream_argument {
        DFHack::color_ostream *out;
    public:
        cur_lua_ostream_argument(lua_State *state);
        operator DFHack::color_ostream& () { return *out; }
    };

    template<class T> struct return_type {};

    template<typename RT, typename ...AT>
    struct return_type<RT (*)(AT...)>{
        using type = RT;
        static const bool is_method = false;
    };

    template<typename RT, class CT, typename ...AT>
    struct return_type<RT (CT::*)(AT...)>{
        using type = RT;
        using class_type = CT;
        static const bool is_method = true;
    };

    template<typename RT, class CT, typename ...AT>
    struct return_type<RT (CT::*)(AT...) const>{
        using type = RT;
        using class_type = CT;
        static const bool is_method = true;
    };

    // the std::is_enum_v alternative is because pushing is_primitive
    // into all the enum identities would require changing codegen

    template<typename RT>
    concept isPrimitive = identity_traits<RT>::is_primitive || std::is_enum_v<RT> || std::is_void_v<RT>;

    template<typename T>
    T get_from_lua_state(lua_State* L, int idx) {
        T val;
        df::identity_traits<T>::get()->lua_write(L, UPVAL_METHOD_NAME, &val, idx);
        return val;
    }


    template<typename RT, typename... AT, typename FT, typename... ET, std::size_t... I>
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
        && isPrimitive<RT>
        void call_and_push_impl(lua_State* L, int base, std::index_sequence<I...>, FT fun, ET... extra)
    {
        if constexpr (std::is_same_v<RT, void>) {
            std::invoke(fun, extra..., (get_from_lua_state<AT>(L, base+I))...);
            lua_pushnil(L);
        }
        else
        {
            RT rv = std::invoke(fun, extra..., (get_from_lua_state<AT>(L, base+I))...);
            df::identity_traits<RT>::get()->lua_read(L, UPVAL_METHOD_NAME, &rv);
        }
    }

    template<typename RT, typename... AT, typename FT, typename... ET, typename indices = std::index_sequence_for<AT...> >
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
        && isPrimitive<RT>

    void call_and_push(lua_State* L, int base, FT fun, ET... extra)
    {
        call_and_push_impl<RT, AT...>(L, base, indices{}, fun, extra...);
    }

    template<typename T> struct function_wrapper {};

    template<typename RT, typename ...AT>
        requires isPrimitive<RT>
    struct function_wrapper<RT(*)(DFHack::color_ostream&, AT...)> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State *L, int base, RT (fun)(DFHack::color_ostream& out, AT...)) {
             cur_lua_ostream_argument out(L);
             call_and_push<RT, AT...>(L, base, fun, out);
        }
    };

    template<typename RT, typename ...AT>
        requires isPrimitive<RT>
    struct function_wrapper<RT(*)(AT...)> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State *L, int base, RT (fun)(AT...)) {
            call_and_push<RT, AT...>(L, base, fun);
        }
    };

    template<typename RT, class CT, typename ...AT>
        requires isPrimitive<RT>
    struct function_wrapper<RT(CT::*)(AT...)> {
        static const int num_args = sizeof...(AT)+1;
        static void execute(lua_State *L, int base, RT(CT::*mem_fun)(AT...)) {
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<RT, AT...>(L, base, mem_fun, self);
        };
    };

    template<typename RT, class CT, typename ...AT>
        requires isPrimitive<RT>
    struct function_wrapper<RT(CT::*)(AT...) const> {
        static const int num_args = sizeof...(AT)+1;
        static void execute(lua_State *L, int base, RT(CT::*mem_fun)(AT...) const) {
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<RT, AT...>(L, base, mem_fun, self);
        };
    };

    template<typename T>
    class function_identity : public function_identity_base {
        T ptr;

    public:
        using wrapper = function_wrapper<T>;

        function_identity(T ptr, bool vararg)
            : function_identity_base(wrapper::num_args, vararg), ptr(ptr) {};

        virtual void invoke(lua_State *state, int base) const { wrapper::execute(state, base, ptr); }
    };

    template<typename T>
    inline function_identity_base *wrap_function(T ptr, bool vararg = false) {
        using RT = return_type<T>::type;
        if constexpr (isPrimitive<RT>)
            return new function_identity<T>(ptr, vararg);
        else
            return nullptr;
    }}
