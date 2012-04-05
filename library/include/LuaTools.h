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

#include "DataDefs.h"

#include <lua.h>
#include <lauxlib.h>

namespace DFHack { namespace Lua {
    /**
     * Create or initialize a lua interpreter with access to DFHack tools.
     */
    DFHACK_EXPORT lua_State *Open(color_ostream &out, lua_State *state = NULL);

    /**
     * Load a module using require().
     */
    DFHACK_EXPORT bool Require(color_ostream &out, lua_State *state,
                               const std::string &module, bool setglobal = false);

    /**
     * Check if the object at the given index is NIL or NULL.
     */
    DFHACK_EXPORT bool IsDFNull(lua_State *state, int val_index);

    enum ObjectClass {
        /** Not a DF wrapper object */
        OBJ_INVALID = 0,
        /** NIL or NULL */
        OBJ_NULL,
        /** A named type identity object */
        OBJ_TYPE,
        /** A void* reference, i.e. non-null lightuserdata */
        OBJ_VOIDPTR,
        /** A typed object reference */
        OBJ_REF
    };

    /**
     * Check if the object at the given index is a valid wrapper object.
     */
    DFHACK_EXPORT ObjectClass IsDFObject(lua_State *state, int val_index);

    /**
     * Push the pointer onto the stack as a wrapped DF object of the given type.
     */
    DFHACK_EXPORT void PushDFObject(lua_State *state, type_identity *type, void *ptr);

    /**
     * Check that the value is a wrapped DF object of the given type, and if so return the pointer.
     */
    DFHACK_EXPORT void *GetDFObject(lua_State *state, type_identity *type, int val_index, bool exact_type = false);

    /**
     * Assign the value at val_index to the target of given identity using df.assign().
     * Return behavior is of SafeCall below.
     */
    DFHACK_EXPORT bool AssignDFObject(color_ostream &out, lua_State *state,
                                      type_identity *type, void *target, int val_index, bool perr = true);

    /**
     * Push the pointer onto the stack as a wrapped DF object of a specific type.
     */
    template<class T>
    void PushDFObject(lua_State *state, T *ptr) {
        PushDFObject(state, df::identity_traits<T>::get(), ptr);
    }

    /**
     * Check that the value is a wrapped DF object of the correct type, and if so return the pointer.
     */
    template<class T>
    T *GetDFObject(lua_State *state, int val_index, bool exact_type = false) {
        return (T*)GetDFObject(state, df::identity_traits<T>::get(), val_index, exact_type);
    }

    /**
     * Assign the value at val_index to the target using df.assign().
     */
    template<class T>
    bool AssignDFObject(color_ostream &out, lua_State *state, T *target, int val_index, bool perr = true) {
        return AssignDFObject(out, state, df::identity_traits<T>::get(), target, val_index, perr);
    }

    /**
     * Invoke lua function via pcall. Returns true if success.
     * If an error is signalled, and perr is true, it is printed and popped from the stack.
     */
    DFHACK_EXPORT bool SafeCall(color_ostream &out, lua_State *state, int nargs, int nres, bool perr = true);

    /**
     * Parse code from string with debug_tag and env_idx, then call it using SafeCall.
     * In case of error, it is either left on the stack, or printed like SafeCall does.
     */
    DFHACK_EXPORT bool SafeCallString(color_ostream &out, lua_State *state, const std::string &code,
                                      int nargs, int nres, bool perr = true,
                                      const char *debug_tag = NULL, int env_idx = 0);

    /**
     * Returns the ostream passed to SafeCall.
     */
    DFHACK_EXPORT color_ostream *GetOutput(lua_State *state);

    /**
     * Run an interactive interpreter loop if possible, or return false.
     */
    DFHACK_EXPORT bool InterpreterLoop(color_ostream &out, lua_State *state,
                                       const char *prompt = NULL, int env = 0, const char *hfile = NULL);
}}

