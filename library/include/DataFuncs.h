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

    // the std::is_enum_v alternative is because pushing is_primitive
    // into all the enum identities would require changing codegen

    template<typename RT>
    concept isPrimitive = identity_traits<RT>::is_primitive || std::is_enum_v<RT> || std::is_void_v<RT>;

    // handle call-by-reference function arguments by converting pointers to a reference
    // will throw if lua code tries to pass a null pointer
    template<typename T> requires std::is_reference_v<T>
    T get_from_lua_state(lua_State* L, int idx)
    {
        using DFHack::LuaWrapper::field_error;
        using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
        using Ptr = std::add_pointer_t<std::remove_reference_t<T>>;
        Ptr ptr{};
        df::identity_traits<Ptr>::get()->lua_write(L, UPVAL_METHOD_NAME, &ptr, idx);
        if (ptr == nullptr)
        {
            field_error(L, UPVAL_METHOD_NAME, "cannot convert null pointer to reference", "call");
        }
        return *ptr;
    }

    // handle call-by-value function arguments. only semi-regular types are allowed
    // (semi-regular covers copyable and default-constructible)
    template<std::semiregular T>
    T get_from_lua_state(lua_State* L, int idx)
    {
        using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
        T val{};
        df::identity_traits<T>::get()->lua_write(L, UPVAL_METHOD_NAME, &val, idx);
        return val;
    }

    template<isPrimitive RT, typename... AT, typename FT, typename... ET, std::size_t... I>
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
    void call_and_push_impl(lua_State* L, int base, std::index_sequence<I...>, FT fun, ET... extra)
    {
        using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
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

    template<isPrimitive RT, typename... AT, typename FT, typename... ET, typename indices = std::index_sequence_for<AT...> >
        requires std::is_invocable_r_v<RT, FT, ET..., AT...>
    void call_and_push(lua_State* L, int base, FT fun, ET... extra)
    {
        call_and_push_impl<RT, AT...>(L, base, indices{}, fun, extra...);
    }

    template<typename T> struct function_wrapper {};

    template<isPrimitive RT, typename ...AT>
    struct function_wrapper<RT(*)(DFHack::color_ostream&, AT...)> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State *L, int base, RT (fun)(DFHack::color_ostream& out, AT...)) {
             cur_lua_ostream_argument out(L);
             call_and_push<RT, AT...>(L, base, fun, out);
        }
    };

    template<isPrimitive RT, typename ...AT>
    struct function_wrapper<RT(*)(AT...)> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State *L, int base, RT (fun)(AT...)) {
            call_and_push<RT, AT...>(L, base, fun);
        }
    };

    template<isPrimitive RT, class CT, typename ...AT>
    struct function_wrapper<RT(CT::*)(AT...)> {
        static const int num_args = sizeof...(AT)+1;
        static void execute(lua_State *L, int base, RT(CT::*mem_fun)(AT...)) {
            using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<RT, AT...>(L, base, mem_fun, self);
        };
    };

    template<isPrimitive RT, class CT, typename ...AT>
    struct function_wrapper<RT(CT::*)(AT...) const> {
        static const int num_args = sizeof...(AT)+1;
        static void execute(lua_State *L, int base, RT(CT::*mem_fun)(AT...) const) {
            using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
            CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<RT, AT...>(L, base, mem_fun, self);
        };
    };

    template<isPrimitive RT, typename ...AT>
    struct function_wrapper<RT(*)(DFHack::color_ostream&, AT...) noexcept> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State* L, int base, RT(fun)(DFHack::color_ostream& out, AT...)) {
            cur_lua_ostream_argument out(L);
            call_and_push<RT, AT...>(L, base, fun, out);
        }
    };

    template<isPrimitive RT, typename ...AT>
    struct function_wrapper<RT(*)(AT...) noexcept> {
        static const int num_args = sizeof...(AT);
        static void execute(lua_State* L, int base, RT(fun)(AT...)) {
            call_and_push<RT, AT...>(L, base, fun);
        }
    };

    template<isPrimitive RT, class CT, typename ...AT>
    struct function_wrapper<RT(CT::*)(AT...) noexcept> {
        static const int num_args = sizeof...(AT) + 1;
        static void execute(lua_State* L, int base, RT(CT::* mem_fun)(AT...)) {
            using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
            CT* self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
            call_and_push<RT, AT...>(L, base, mem_fun, self);
        };
    };

    template<isPrimitive RT, class CT, typename ...AT>
    struct function_wrapper<RT(CT::*)(AT...) const noexcept> {
        static const int num_args = sizeof...(AT) + 1;
        static void execute(lua_State* L, int base, RT(CT::* mem_fun)(AT...) const) {
            using DFHack::LuaWrapper::UPVAL_METHOD_NAME;
            CT* self = (CT*)DFHack::LuaWrapper::get_object_addr(L, base++, UPVAL_METHOD_NAME, "invoke");
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
