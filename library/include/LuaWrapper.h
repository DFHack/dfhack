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

#include "DataDefs.h"

#include <lua.h>
#include <lauxlib.h>

/**
 * Internal header file of the lua wrapper.
 */


namespace DFHack {
    struct FunctionReg;
namespace LuaWrapper {
    struct LuaToken;

    /**
     * Metatable pkey: type identity of the object
     */
    extern LuaToken DFHACK_IDENTITY_FIELD_TOKEN;

    /**
     * Registry pkey: hash of type metatables <-> type identities.
     */
    extern LuaToken DFHACK_TYPETABLE_TOKEN;

    /**
     * Registry pkey: hash of type identity -> node in df.etc...
     */
    extern LuaToken DFHACK_TYPEID_TABLE_TOKEN;

    /**
     * Registry pkey: hash of enum/bitfield identity -> index lookup table
     */
    extern LuaToken DFHACK_ENUM_TABLE_TOKEN;

    /**
     * Registry pkey: hash of pointer target identity <-> adhoc pointer identity userdata.
     */
    extern LuaToken DFHACK_PTR_IDTABLE_TOKEN;

// Function registry names
#define DFHACK_CHANGEERROR_NAME "DFHack::ChangeError"
#define DFHACK_COMPARE_NAME "DFHack::ComparePtrs"
#define DFHACK_TYPE_TOSTRING_NAME "DFHack::TypeToString"
#define DFHACK_SIZEOF_NAME "DFHack::Sizeof"
#define DFHACK_DISPLACE_NAME "DFHack::Displace"
#define DFHACK_NEW_NAME "DFHack::New"
#define DFHACK_ASSIGN_NAME "DFHack::Assign"
#define DFHACK_IS_INSTANCE_NAME "DFHack::IsInstance"
#define DFHACK_DELETE_NAME "DFHack::Delete"
#define DFHACK_CAST_NAME "DFHack::Cast"

    extern LuaToken DFHACK_EMPTY_TABLE_TOKEN;

/*
 * Upvalue: contents of DFHACK_TYPETABLE_NAME
 */
#define UPVAL_TYPETABLE lua_upvalueindex(1)

/*
 * Expected metatable of the current object.
 */
#define UPVAL_METATABLE lua_upvalueindex(2)

/*
 * Table mapping field names to indices or data structure pointers.
 * Enum index table is linked into here via getmetatable($).__index.
 * Fields that are actually in UPVAL_METATABLE are marked with NULL light udata.
 */
#define UPVAL_FIELDTABLE lua_upvalueindex(3)
#define UPVAL_METHOD_NAME lua_upvalueindex(3)

/*
 * Only for containers: light udata with container identity.
 */
#define UPVAL_CONTAINER_ID lua_upvalueindex(4)

/*
 * Only for containers: light udata with item identity.
 */
#define UPVAL_ITEM_ID lua_upvalueindex(5)

/*
 * Only for containers: if not nil, overrides the item count.
 */
#define UPVAL_ITEM_COUNT lua_upvalueindex(6)

    inline void lua_dup(lua_State *state) { lua_pushvalue(state, -1); }
    inline void lua_swap(lua_State *state) { lua_insert(state, -2); }

    /**
     * Object references are represented as userdata instances
     * with an appropriate metatable; the payload of userdata is
     * this structure:
     */
    struct DFRefHeader {
        void *ptr;
    };

    /**
     * Push the pointer as DF object ref using metatable on the stack.
     */
    void push_object_ref(lua_State *state, void *ptr);
    void *get_object_ref(lua_State *state, int val_index);

    /*
     * The system might be extended to carry some simple
     * objects inline inside the reference buffer.
     */
    inline bool is_self_contained(DFRefHeader *ptr) {
        void **pp = &ptr->ptr;
        return **(void****)pp == (pp + 1);
    }

    /**
    * Report an error while accessing a field (index = field name).
    */
    void field_error(lua_State *state, int index, const char *err, const char *mode);

    /*
     * If is_method is true, these use UPVAL_TYPETABLE to save a hash lookup.
     */
    void push_object_internal(lua_State *state, type_identity *type, void *ptr, bool in_method = true);
    void *get_object_internal(lua_State *state, type_identity *type, int val_index, bool exact_type, bool in_method = true);

    void push_adhoc_pointer(lua_State *state, void *ptr, type_identity *target);

    /**
     * Verify that the object is a DF ref with UPVAL_METATABLE.
     * If everything ok, extract the address.
     */
    DFHACK_EXPORT uint8_t *get_object_addr(lua_State *state, int obj, int field, const char *mode);

    bool is_type_compatible(lua_State *state, type_identity *type1, int meta1,
                            type_identity *type2, int meta2, bool exact_equal);

    type_identity *get_object_identity(lua_State *state, int objidx,
                                       const char *ctx, bool allow_type = false,
                                       bool keep_metatable = false);

    void LookupInTable(lua_State *state, void *id, LuaToken *tname);
    void SaveInTable(lua_State *state, void *node, LuaToken *tname);
    void SaveTypeInfo(lua_State *state, void *node);

    void AssociateId(lua_State *state, int table, int val, const char *name);

    /**
     * Look up the key on the stack in DFHACK_TYPETABLE;
     * if found, put result on the stack and return true.
     */
    bool LookupTypeInfo(lua_State *state, bool in_method);

    /**
     * Make a metatable with most common fields, and an empty table for UPVAL_FIELDTABLE.
     */
    void MakeMetatable(lua_State *state, type_identity *type, const char *kind);
    /**
     * Enable a metafield by injecting an entry into a UPVAL_FIELDTABLE.
     */
    void EnableMetaField(lua_State *state, int ftable_idx, const char *name, void *id = NULL);
    /**
     * Set metatable properties common to all actual DF object references.
     */
    void SetPtrMethods(lua_State *state, int meta_idx, int read_idx);
    /**
     * Add a __pairs/__ipairs metamethod using iterator on the top of stack.
     */
    void SetPairsMethod(lua_State *state, int meta_idx, const char *name);
    /**
     * Add a struct-style (3 upvalues) metamethod to the stack.
     */
    void PushStructMethod(lua_State *state, int meta_idx, int ftable_idx,
                          lua_CFunction function);
    /**
     * Add a struct-style (3 upvalues) metamethod to the metatable.
     */
    void SetStructMethod(lua_State *state, int meta_idx, int ftable_idx,
                         lua_CFunction function, const char *name);
    /**
     * Add a 6 upvalue metamethod to the stack.
     */
    void PushContainerMethod(lua_State *state, int meta_idx, int ftable_idx,
                             lua_CFunction function,
                             type_identity *container, type_identity *item, int count);
    /**
     * Add a 6 upvalue metamethod to the metatable.
     */
    void SetContainerMethod(lua_State *state, int meta_idx, int ftable_idx,
                            lua_CFunction function, const char *name,
                            type_identity *container, type_identity *item, int count);
    /**
     * If ienum refers to a valid enum, attach its keys to UPVAL_FIELDTABLE,
     * and the enum itself to the _enum metafield. Pushes the key table on the stack.
     */
    void AttachEnumKeys(lua_State *state, int meta_idx, int ftable_idx, type_identity *ienum);

    /**
     * Push a closure invoking the given function.
     */
    void PushFunctionWrapper(lua_State *state, int meta_idx,
                             const char *name, function_identity_base *fun);

    /**
     * Wrap functions and add them to the table on the top of the stack.
     */
    typedef DFHack::FunctionReg FunctionReg;
    void SetFunctionWrappers(lua_State *state, const FunctionReg *reg);

    int method_wrapper_core(lua_State *state, function_identity_base *id);

    void IndexStatics(lua_State *state, int meta_idx, int ftable_idx, struct_identity *pstruct);

    void AttachDFGlobals(lua_State *state);
}}

