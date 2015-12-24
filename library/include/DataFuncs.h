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

    class DFHACK_EXPORT cur_lua_ostream_argument {
        DFHack::color_ostream *out;
    public:
        cur_lua_ostream_argument(lua_State *state);
        operator DFHack::color_ostream& () { return *out; }
    };

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
#define OSTREAM_ARG DFHack::color_ostream&
#define LOAD_OSTREAM(name) \
    cur_lua_ostream_argument name(state);

#define INSTANTIATE_RETURN_TYPE(FArgs) \
    template<FW_TARGSC class RT> struct return_type<RT (*) FArgs> { \
        typedef RT type; \
        static const bool is_method = false; \
    }; \
    template<FW_TARGSC class RT, class CT> struct return_type<RT (CT::*) FArgs> { \
        typedef RT type; \
        typedef CT class_type; \
        static const bool is_method = true; \
    };

#define INSTANTIATE_WRAPPERS2(Count, FArgs, Args, Loads) \
    template<FW_TARGS> struct function_wrapper<void (*) FArgs, true> { \
        static const int num_args = Count; \
        static void execute(lua_State *state, int base, void (*cb) FArgs) { Loads; INVOKE_VOID(cb Args); } \
    }; \
    template<FW_TARGSC class RT> struct function_wrapper<RT (*) FArgs, false> { \
        static const int num_args = Count; \
        static void execute(lua_State *state, int base, RT (*cb) FArgs) { Loads; INVOKE_RV(cb Args); } \
    }; \
    template<FW_TARGSC class CT> struct function_wrapper<void (CT::*) FArgs, true> { \
        static const int num_args = Count+1; \
        static void execute(lua_State *state, int base, void (CT::*cb) FArgs) { \
            LOAD_CLASS() Loads; INVOKE_VOID((self->*cb) Args); } \
    }; \
    template<FW_TARGSC class RT, class CT> struct function_wrapper<RT (CT::*) FArgs, false> { \
        static const int num_args = Count+1; \
        static void execute(lua_State *state, int base, RT (CT::*cb) FArgs) { \
            LOAD_CLASS(); Loads; INVOKE_RV((self->*cb) Args); } \
    };

#define INSTANTIATE_WRAPPERS(Count, FArgs, OFArgs, Args, OArgs, Loads) \
    INSTANTIATE_WRAPPERS2(Count, FArgs, Args, Loads) \
    INSTANTIATE_WRAPPERS2(Count, OFArgs, OArgs, LOAD_OSTREAM(out); Loads)

#define FW_TARGSC
#define FW_TARGS
INSTANTIATE_RETURN_TYPE(())
INSTANTIATE_WRAPPERS(0, (), (OSTREAM_ARG), (), (out), ;)
#undef FW_TARGS

#undef FW_TARGSC
#define FW_TARGSC FW_TARGS,
#define FW_TARGS class A1
INSTANTIATE_RETURN_TYPE((A1))
INSTANTIATE_WRAPPERS(1, (A1), (OSTREAM_ARG,A1), (vA1), (out,vA1), LOAD_ARG(A1);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2
INSTANTIATE_RETURN_TYPE((A1,A2))
INSTANTIATE_WRAPPERS(2, (A1,A2), (OSTREAM_ARG,A1,A2), (vA1,vA2), (out,vA1,vA2),
                     LOAD_ARG(A1); LOAD_ARG(A2);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3
INSTANTIATE_RETURN_TYPE((A1,A2,A3))
INSTANTIATE_WRAPPERS(3, (A1,A2,A3), (OSTREAM_ARG,A1,A2,A3), (vA1,vA2,vA3), (out,vA1,vA2,vA3),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4))
INSTANTIATE_WRAPPERS(4, (A1,A2,A3,A4), (OSTREAM_ARG,A1,A2,A3,A4),
                        (vA1,vA2,vA3,vA4), (out,vA1,vA2,vA3,vA4),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5))
INSTANTIATE_WRAPPERS(5, (A1,A2,A3,A4,A5), (OSTREAM_ARG,A1,A2,A3,A4,A5),
                        (vA1,vA2,vA3,vA4,vA5), (out,vA1,vA2,vA3,vA4,vA5),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6))
INSTANTIATE_WRAPPERS(6, (A1,A2,A3,A4,A5,A6), (OSTREAM_ARG,A1,A2,A3,A4,A5,A6),
                        (vA1,vA2,vA3,vA4,vA5,vA6), (out,vA1,vA2,vA3,vA4,vA5,vA6),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6, class A7
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6,A7))
INSTANTIATE_WRAPPERS(7, (A1,A2,A3,A4,A5,A6,A7), (OSTREAM_ARG,A1,A2,A3,A4,A5,A6,A7),
                        (vA1,vA2,vA3,vA4,vA5,vA6,vA7), (out,vA1,vA2,vA3,vA4,vA5,vA6,vA7),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6); LOAD_ARG(A7);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6,A7,A8))
INSTANTIATE_WRAPPERS(8, (A1,A2,A3,A4,A5,A6,A7,A8), (OSTREAM_ARG,A1,A2,A3,A4,A5,A6,A7,A8),
                        (vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8), (out,vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6); LOAD_ARG(A7); LOAD_ARG(A8);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6,A7,A8,A9))
INSTANTIATE_WRAPPERS(9, (A1,A2,A3,A4,A5,A6,A7,A8,A9),
                        (OSTREAM_ARG,A1,A2,A3,A4,A5,A6,A7,A8,A9),
                        (vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9),
                        (out,vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6); LOAD_ARG(A7); LOAD_ARG(A8);
                     LOAD_ARG(A9);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6,A7,A8,A9,A10))
INSTANTIATE_WRAPPERS(10, (A1,A2,A3,A4,A5,A6,A7,A8,A9,A10),
                         (OSTREAM_ARG,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10),
                         (vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9,vA10),
                         (out,vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9,vA10),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6); LOAD_ARG(A7); LOAD_ARG(A8);
                     LOAD_ARG(A9); LOAD_ARG(A10);)
#undef FW_TARGS

#define FW_TARGS class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11
INSTANTIATE_RETURN_TYPE((A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11))
INSTANTIATE_WRAPPERS(11, (A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11),
                         (OSTREAM_ARG,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11),
                         (vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9,vA10,vA11),
                         (out,vA1,vA2,vA3,vA4,vA5,vA6,vA7,vA8,vA9,vA10,vA11),
                     LOAD_ARG(A1); LOAD_ARG(A2); LOAD_ARG(A3); LOAD_ARG(A4);
                     LOAD_ARG(A5); LOAD_ARG(A6); LOAD_ARG(A7); LOAD_ARG(A8);
                     LOAD_ARG(A9); LOAD_ARG(A10); LOAD_ARG(A11);)
#undef FW_TARGS

#undef FW_TARGSC
#undef INSTANTIATE_WRAPPERS
#undef INSTANTIATE_WRAPPERS2
#undef INVOKE_VOID
#undef INVOKE_RV
#undef LOAD_CLASS
#undef LOAD_ARG
#undef OSTREAM_ARG
#undef LOAD_OSTREAM

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
