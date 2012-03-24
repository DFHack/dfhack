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

/**
 * Internal header file of the lua wrapper.
 */

namespace DFHack { namespace LuaWrapper {

/*
 * Registry name: hash of type metatables <-> type identities.
 */
#define DFHACK_TYPETABLE_NAME "DFHack::DFTypes"

/*
 * Registry name: hash of type identity -> node in df.etc...
 */
#define DFHACK_TYPEID_TABLE_NAME "DFHack::DFTypeIds"

/*
 * Registry name: hash of enum/bitfield identity -> index lookup table
 */
#define DFHACK_ENUM_TABLE_NAME "DFHack::DFEnums"

/*
 * Registry name: hash of pointer target identity <-> adhoc pointer identity userdata.
 */
#define DFHACK_PTR_IDTABLE_NAME "DFHack::PtrDFTypes"

// Function registry names
#define DFHACK_CHANGEERROR_NAME "DFHack::ChangeError"
#define DFHACK_COMPARE_NAME "DFHack::ComparePtrs"
#define DFHACK_TYPE_TOSTRING_NAME "DFHack::TypeToString"
#define DFHACK_SIZEOF_NAME "DFHack::Sizeof"
#define DFHACK_DISPLACE_NAME "DFHack::Displace"
#define DFHACK_NEW_NAME "DFHack::New"
#define DFHACK_ASSIGN_NAME "DFHack::Assign"

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
    uint8_t *get_object_addr(lua_State *state, int obj, int field, const char *mode);

    void LookupInTable(lua_State *state, void *id, const char *tname);
    void SaveInTable(lua_State *state, void *node, const char *tname);
    void SaveTypeInfo(lua_State *state, void *node);

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
     * Add a struct-style (3 upvalues) metamethod to the metatable.
     */
    void SetStructMethod(lua_State *state, int meta_idx, int ftable_idx,
                         lua_CFunction function, const char *name);
    /**
     * Add a 6 upvalue metamethod to the metatable.
     */
    void SetContainerMethod(lua_State *state, int meta_idx, int ftable_idx,
                            lua_CFunction function, const char *name,
                            type_identity *container, type_identity *item, int count);
    /**
     * If ienum refers to a valid enum, attach its keys to UPVAL_FIELDTABLE,
     * and the enum itself to the _enum metafield.
     */
    void AttachEnumKeys(lua_State *state, int meta_idx, int ftable_idx, type_identity *ienum);
}}

