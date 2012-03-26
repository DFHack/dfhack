/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#include "DataIdentity.h"
#include "LuaWrapper.h"

namespace df {
    // A very simple and stupid implementation of some stuff from boost
    template<class U, class V> struct is_same_type { static const bool value = false; };
    template<class T> struct is_same_type<T,T> { static const bool value = true; };
    template<class T> struct return_type {};

    /*
     * Workaround for a msvc bug suggested by:
     *
     * http://stackoverflow.com/questions/5110529/class-template-partial-specialization-parametrized-on-member-function-return-typ
     */
    template<class T, bool isvoid = is_same_type<typename return_type<T>::type,void>::value>
    struct function_wrapper {};

    /*
     * Since templates can't match variable arg count,
     * a separate specialization is needed for every
     * supported count value...
     *
     * The FW_TARGS ugliness is needed because of
     * commas not wrapped in ()
     */

#define INVOKE_VOID(call) \
    call; lua_pushnil(state);
#define INVOKE_RV(call) \
    RT rv = call; df::identity_traits<RT>::get()->lua_read(state, UPVAL_METHOD_NAME, &rv);
#define LOAD_CLASS() \
    CT *self = (CT*)DFHack::LuaWrapper::get_object_addr(state, base++, UPVAL_METHOD_NAME, "invoke");
#define LOAD_ARG(type) \
    type v##type; df::identity_traits<type>::get()->lua_write(state, UPVAL_METHOD_NAME, &v##type, base++);

#define INSTANTIATE_WRAPPERS(Count, FArgs, Args, Loads) \
    template<FW_TARGSC class RT> struct return_type<RT (*) FArgs> { typedef RT type; }; \
    template<FW_TARGSC class RT, class CT> struct return_type<RT (CT::*) FArgs> { typedef RT type; }; \
    template<FW_TARGS> struct function_wrapper<void (*) FArgs, true> { \
        static const bool is_method = false; \
        static const int num_args = Count; \
        static void execute(lua_State *state, int base, void (*cb) FArgs) { Loads; INVOKE_VOID(cb Args); } \
    }; \
    template<FW_TARGSC class RT> struct function_wrapper<RT (*) FArgs, false> { \
        static const bool is_method = false; \
        static const int num_args = Count; \
        static void execute(lua_State *state, int base, RT (*cb) FArgs) { Loads; INVOKE_RV(cb Args); } \
    }; \
    template<FW_TARGSC class CT> struct function_wrapper<void (CT::*) FArgs, true> { \
        static const bool is_method = true; \
        static const int num_args = Count+1; \
        static void execute(lua_State *state, int base, void (CT::*cb) FArgs) { \
            LOAD_CLASS() Loads; INVOKE_VOID((self->*cb) Args); } \
    }; \
    template<FW_TARGSC class RT, class CT> struct function_wrapper<RT (CT::*) FArgs, false> { \
        static const bool is_method = true; \
        static const int num_args = Count+1; \
        static void execute(lua_State *state, int base, RT (CT::*cb) FArgs) { \
            LOAD_CLASS(); Loads; INVOKE_RV((self->*cb) Args); } \
    };

#define FW_TARGSC
#define FW_TARGS
INSTANTIATE_WRAPPERS(0, (), (), ;)
#undef FW_TARGS

#undef FW_TARGSC
#define FW_TARGSC FW_TARGS,
#define FW_TARGS class A1
INSTANTIATE_WRAPPERS(1, (A1), (vA1), LOAD_ARG(A1);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2
INSTANTIATE_WRAPPERS(2, (A1,A2), (vA1,vA2), LOAD_ARG(A1); LOAD_ARG(A2);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3
INSTANTIATE_WRAPPERS(3, (A1,A2,A3), (vA1,vA2,vA3), LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4
INSTANTIATE_WRAPPERS(4, (A1,A2,A3,A4), (vA1,vA2,vA3,vA4),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);)
#undef FW_TARGS

#undef FW_TARGSC
#undef INSTANTIATE_WRAPPERS
#undef INVOKE_VOID
#undef INVOKE_RV
#undef LOAD_CLASS
#undef LOAD_ARG

    template<class T>
    class function_identity : public function_identity_base {
        T ptr;

    public:
        typedef function_wrapper<T> wrapper;

        function_identity(T ptr)
            : function_identity_base(wrapper::num_args), ptr(ptr) {};

        virtual void invoke(lua_State *state, int base) { wrapper::execute(state, base, ptr); }
    };

    template<class T>
    inline function_identity_base *wrap_function(T ptr) {
        // bah, but didn't have any idea how to allocate statically
        return new function_identity<T>(ptr);
    }
}