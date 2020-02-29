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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "Error.h"
#include "VersionInfo.h"
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"
#include "LuaWrapper.h"
#include "LuaTools.h"
#include "DataFuncs.h"

#include "PluginManager.h"
#include "MiscUtils.h"

#include <lua.h>
#include <lauxlib.h>

using namespace DFHack;
using namespace DFHack::LuaWrapper;

#ifdef _DARWIN
#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_6
size_t strnlen (const char *str, size_t max)
{
    const char *end = (const char*)memchr(str, 0, max);
    return end ? (size_t)(end - str) : max;
}
#endif
#endif

/**************************************
 * Identity object read/write methods *
 **************************************/

void function_identity_base::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    field_error(state, fname_idx, "executable code", "read");
}

void function_identity_base::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    field_error(state, fname_idx, "executable code", "write");
}

void constructed_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    push_object_internal(state, this, ptr);
}

static void invoke_assign(lua_State *state, type_identity *id, void *ptr, int val_index)
{
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_ASSIGN_NAME);
    push_object_internal(state, id, ptr);
    lua_pushvalue(state, val_index);
    lua_call(state, 2, 0);
}

void constructed_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    if (lua_istable(state, val_index))
    {
        invoke_assign(state, this, ptr, val_index);
    }
    // Allow by-value assignment for wrapped function parameters
    else if (fname_idx == UPVAL_METHOD_NAME && lua_isuserdata(state, val_index))
    {
        void *nval = get_object_internal(state, this, val_index, false);
        if (!nval)
            field_error(state, fname_idx, "incompatible type in complex assignment", "write");
        if (!copy(ptr, nval))
            field_error(state, fname_idx, "no copy support", "write");
    }
    else
        field_error(state, fname_idx, "complex object", "write");
}

void enum_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    base_type->lua_read(state, fname_idx, ptr);
}

void enum_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    base_type->lua_write(state, fname_idx, ptr, val_index);
}

void df::integer_identity_base::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_pushinteger(state, read(ptr));
}

void df::integer_identity_base::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    int is_num = 0;
    auto value = lua_tointegerx(state, val_index, &is_num);
    if (!is_num)
        field_error(state, fname_idx, "integer expected", "write");
    write(ptr, value);
}

void df::float_identity_base::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_pushnumber(state, read(ptr));
}

void df::float_identity_base::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    if (!lua_isnumber(state, val_index))
        field_error(state, fname_idx, "number expected", "write");

    write(ptr, lua_tonumber(state, val_index));
}

void df::bool_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_pushboolean(state, *(bool*)ptr);
}

void df::bool_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    char *pb = (char*)ptr;

    if (lua_isboolean(state, val_index) || lua_isnil(state, val_index))
        *pb = lua_toboolean(state, val_index);
    else if (lua_isnumber(state, val_index))
        *pb = lua_tointeger(state, val_index);
    else
        field_error(state, fname_idx, "boolean or number expected", "write");
}

void df::ptr_string_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    auto pstr = (char**)ptr;
    if (*pstr)
        lua_pushstring(state, *pstr);
    else
        lua_pushnil(state);
}

void df::ptr_string_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    field_error(state, fname_idx, "raw pointer string", "write");
}

void df::stl_string_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    auto pstr = (std::string*)ptr;
    lua_pushlstring(state, pstr->data(), pstr->size());
}

void df::stl_string_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    size_t size;
    const char *bytes = lua_tolstring(state, val_index, &size);
    if (!bytes)
        field_error(state, fname_idx, "string expected", "write");

    *(std::string*)ptr = std::string(bytes, size);
}

void df::pointer_identity::lua_read(lua_State *state, int fname_idx, void *ptr, type_identity *target)
{
    push_object_internal(state, target, *(void**)ptr);
}

void df::pointer_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_read(state, fname_idx, ptr, target);
}

static void autovivify_ptr(lua_State *state, int fname_idx, void **pptr,
                          type_identity *target, int val_index)
{
    lua_getfield(state, val_index, "new");

    // false or nil => bail out
    if (!lua_toboolean(state, -1))
        field_error(state, fname_idx, "null and autovivify not requested", "write");

    // not 'true' => call df.new()
    if (!lua_isboolean(state, -1))
    {
        int top = lua_gettop(state);

        // Verify new points to a reasonable type of object
        type_identity *suggested = get_object_identity(state, top, "autovivify", true, true);

        if (!is_type_compatible(state, target, 0, suggested, top+1, false))
            field_error(state, fname_idx, "incompatible suggested autovivify type", "write");

        lua_pop(state, 1);

        // Invoke df.new()
        lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_NEW_NAME);
        lua_swap(state);
        lua_call(state, 1, 1);

        // Retrieve the pointer
        void *nval = get_object_internal(state, target, top, false);

        // shouldn't happen: this means suggested type is compatible,
        // but its new() result isn't for some reason.
        if (!nval)
            field_error(state, fname_idx, "inconsistent autovivify type", "write");

        *pptr = nval;
    }
    // otherwise use the target type
    else
    {
        if (!target)
            field_error(state, fname_idx, "trying to autovivify void*", "write");

        *pptr = target->allocate();

        if (!*pptr)
            field_error(state, fname_idx, "could not allocate in autovivify", "write");
    }

    lua_pop(state, 1);
}

static bool is_null(lua_State *state, int val_index)
{
    return lua_isnil(state, val_index) ||
           (lua_islightuserdata(state, val_index) &&
            !lua_touserdata(state, val_index));
}

void df::pointer_identity::lua_write(lua_State *state, int fname_idx, void *ptr,
                                     type_identity *target, int val_index)
{
    auto pptr = (void**)ptr;

    if (is_null(state, val_index))
        *pptr = NULL;
    else if (lua_istable(state, val_index))
    {
        if (!*pptr)
            autovivify_ptr(state, fname_idx, pptr, target, val_index);

        invoke_assign(state, target, *pptr, val_index);
    }
    else
    {
        void *nval = get_object_internal(state, target, val_index, false);
        if (nval)
            *pptr = nval;
        else
            field_error(state, fname_idx, "incompatible pointer type", "write");
    }
}

void df::pointer_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    lua_write(state, fname_idx, ptr, target, val_index);
}

int container_identity::lua_item_count(lua_State *state, void *ptr, CountMode mode)
{
    if (lua_isnumber(state, UPVAL_ITEM_COUNT))
        return lua_tointeger(state, UPVAL_ITEM_COUNT);
    else
        return item_count(ptr, mode);
}

void container_identity::lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(id, ptr, idx);
    push_object_internal(state, id, pitem);
}

void container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(id, ptr, idx);
    id->lua_read(state, fname_idx, pitem);
}

void container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    if (is_readonly())
        field_error(state, fname_idx, "container is read-only", "write");

    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(id, ptr, idx);
    id->lua_write(state, fname_idx, pitem, val_index);
}

bool container_identity::lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);

    char tmp[32];
    void *pitem = &tmp;

    if (id->isPrimitive())
    {
        if (id->isConstructed())
            luaL_error(state, "Temporaries of type %s not supported", id->getFullName().c_str());

        assert(id->byte_size() <= sizeof(tmp));
        id->lua_write(state, fname_idx, pitem, val_index);
    }
    else
    {
        pitem = get_object_internal(state, id, val_index, false);
        if (!pitem)
            field_error(state, fname_idx, "incompatible object type", "insert");
    }

    return insert(ptr, idx, pitem);
}

void ptr_container_identity::lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(id, ptr, idx);
    push_adhoc_pointer(state, pitem, id);
}

void ptr_container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(&df::identity_traits<void*>::identity, ptr, idx);
    df::pointer_identity::lua_read(state, fname_idx, pitem, id);
}

void ptr_container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = item_pointer(&df::identity_traits<void*>::identity, ptr, idx);
    df::pointer_identity::lua_write(state, fname_idx, pitem, id, val_index);
}

bool ptr_container_identity::lua_insert2(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);

    void *pitem = NULL;
    df::pointer_identity::lua_write(state, fname_idx, &pitem, id, val_index);

    return insert(ptr, idx, pitem);
}

void bit_container_identity::lua_item_reference(lua_State *state, int, void *, int)
{
    lua_pushnil(state);
}

void bit_container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    lua_pushboolean(state, get_item(ptr, idx));
}

void bit_container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    if (lua_isboolean(state, val_index) || lua_isnil(state, val_index))
        set_item(ptr, idx, lua_toboolean(state, val_index));
    else if (lua_isnumber(state, val_index))
        set_item(ptr, idx, lua_tointeger(state, val_index) != 0);
    else
        field_error(state, fname_idx, "boolean or number expected", "write");
}

/**
 * Resolve the field name in UPVAL_FIELDTABLE, die if not found.
 */
static void lookup_field(lua_State *state, int index, const char *mode)
{
    lua_pushvalue(state, index);
    lua_gettable(state, UPVAL_FIELDTABLE); // uses metatable with enum keys

    if (lua_isnil(state, -1))
        field_error(state, index, "not found", mode);
}

// Resolve the field in the metatable and return
static int get_metafield(lua_State *state)
{
    lua_rawget(state, UPVAL_METATABLE);
    return 1;
}

static void *find_field(lua_State *state, int index, const char *mode)
{
    lookup_field(state, index, mode);

    // Methods
    if (lua_isfunction(state, -1))
        return NULL;

    // Otherwise must be a pointer
    if (!lua_isuserdata(state, -1))
        field_error(state, index, "corrupted field table", mode);

    void *p = lua_touserdata(state, -1);
    lua_pop(state, 1);

    // NULL => metafield
    if (!p)
        get_metafield(state);
    return p;
}

static int cur_iter_index(lua_State *state, int len, int fidx, int first_idx = -1)
{
    int rv;

    if (lua_isnil(state, fidx))
        rv = first_idx;
    else
    {
        if (lua_isnumber(state, fidx))
            rv = lua_tointeger(state, fidx);
        else
        {
            lua_pushvalue(state, fidx);
            lua_rawget(state, UPVAL_FIELDTABLE);
            if (!lua_isnumber(state, -1))
                field_error(state, fidx, "index not found", "iterate");
            rv = lua_tointeger(state, -1);
            lua_pop(state, 1);
        }

        if (rv < 0 || rv >= len)
            field_error(state, fidx, "index out of bounds", "iterate");
    }

    return rv;
}

static void iter_idx_to_name(lua_State *state, int idx)
{
    lua_pushvalue(state, idx);
    lua_rawget(state, UPVAL_FIELDTABLE);
    if (lua_isnil(state, -1))
        lua_pop(state, 1);
    else
        lua_replace(state, idx);
}

static uint8_t *check_method_call(lua_State *state, int min_args, int max_args)
{
    int argc = lua_gettop(state)-1;
    if (argc < min_args || argc > max_args)
        field_error(state, UPVAL_METHOD_NAME, "wrong argument count", "call");

    return get_object_addr(state, 1, UPVAL_METHOD_NAME, "call");
}

static void GetAdHocMetatable(lua_State *state, const struct_field_info *field);

static void read_field(lua_State *state, const struct_field_info *field, void *ptr)
{
    switch (field->mode)
    {
        case struct_field_info::STATIC_STRING:
        {
            int len = strnlen((char*)ptr, field->count);
            lua_pushlstring(state, (char*)ptr, len);
            return;
        }

        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            // error

        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
            field->type->lua_read(state, 2, ptr);
            return;

        case struct_field_info::POINTER:
            df::pointer_identity::lua_read(state, 2, ptr, field->type);
            return;

        case struct_field_info::CONTAINER:
            if (!field->eid || !field->type->isContainer() ||
                field->eid == ((container_identity*)field->type)->getIndexEnumType())
            {
                field->type->lua_read(state, 2, ptr);
                return;
            }
            // fallthrough

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            GetAdHocMetatable(state, field);
            push_object_ref(state, ptr);
            return;

        case struct_field_info::END:
            break;
    }

    lua_pushnil(state);
}

static void field_reference(lua_State *state, const struct_field_info *field, void *ptr)
{
    switch (field->mode)
    {
        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
            push_object_internal(state, field->type, ptr);
            return;

        case struct_field_info::POINTER:
            push_adhoc_pointer(state, ptr, field->type);
            return;

        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            // error

        case struct_field_info::CONTAINER:
            read_field(state, field, ptr);
            return;

        case struct_field_info::STATIC_STRING:
        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            GetAdHocMetatable(state, field);
            push_object_ref(state, ptr);
            return;

        case struct_field_info::END:
            break;
    }

    lua_pushnil(state);
}

static void write_field(lua_State *state, const struct_field_info *field, void *ptr, int value_idx)
{
    switch (field->mode)
    {
        case struct_field_info::STATIC_STRING:
        {
            size_t size;
            const char *str = lua_tolstring(state, value_idx, &size);
            if (!str)
                field_error(state, 2, "string expected", "write");
            memcpy(ptr, str, std::min(size+1, size_t(field->count)));
            return;
        }

        case struct_field_info::OBJ_METHOD:
        case struct_field_info::CLASS_METHOD:
            // error

        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
        case struct_field_info::CONTAINER:
            field->type->lua_write(state, 2, ptr, value_idx);
            return;

        case struct_field_info::POINTER:
            df::pointer_identity::lua_write(state, 2, ptr, field->type, value_idx);
            return;

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_ASSIGN_NAME);
            read_field(state, field, ptr);
            lua_pushvalue(state, value_idx);
            lua_call(state, 2, 0);
            return;

        case struct_field_info::END:
            return;
    }
}

/**
 * Metamethod: __index for structures.
 */
static int meta_struct_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    auto field = (struct_field_info*)find_field(state, 2, "read");
    if (!field)
        return 1;
    read_field(state, field, ptr + field->offset);
    return 1;
}

/**
 * Method: _field for structures.
 */
static int meta_struct_field_reference(lua_State *state)
{
    if (lua_gettop(state) != 2)
        luaL_error(state, "Usage: object._field(name)");
    uint8_t *ptr = get_object_addr(state, 1, 2, "reference");
    auto field = (struct_field_info*)find_field(state, 2, "reference");
    if (!field)
        field_error(state, 2, "builtin property or method", "reference");
    field_reference(state, field, ptr + field->offset);
    return 1;
}

/**
 * Metamethod: __newindex for structures.
 */
static int meta_struct_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    auto field = (struct_field_info*)find_field(state, 2, "write");
    if (!field)
        field_error(state, 2, "builtin property or method", "write");
    write_field(state, field, ptr + field->offset, 3);
    return 0;
}

/**
 * Metamethod: iterator for structures.
 */
static int meta_struct_next(lua_State *state)
{
    if (lua_gettop(state) < 2) lua_pushnil(state);

    int len = lua_rawlen(state, UPVAL_FIELDTABLE);
    int idx = cur_iter_index(state, len+1, 2, 0);
    if (idx == len)
        return 0;

    lua_rawgeti(state, UPVAL_FIELDTABLE, idx+1);
    lua_dup(state);
    lua_gettable(state, 1);
    return 2;
}

/**
 * Field lookup for primitive refs: behave as a quasi-array with numeric indices.
 */
static type_identity *find_primitive_field(lua_State *state, int field, const char *mode, uint8_t **ptr)
{
    if (lua_type(state, field) == LUA_TNUMBER)
    {
        int idx = lua_tointeger(state, field);
        if (idx < 0)
            field_error(state, 2, "negative index", mode);

        lua_rawgetp(state, UPVAL_METATABLE, &DFHACK_IDENTITY_FIELD_TOKEN);
        auto id = (type_identity *)lua_touserdata(state, -1);
        lua_pop(state, 1);

        *ptr += int(id->byte_size()) * idx;
        return id;
    }

    return (type_identity*)find_field(state, field, mode);
}

/**
 * Metamethod: __index for primitives, i.e. simple object references.
 *   Fields point to identity, or NULL for metafields.
 */
static int meta_primitive_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    auto type = find_primitive_field(state, 2, "read", &ptr);
    if (!type)
        return 1;
    type->lua_read(state, 2, ptr);
    return 1;
}

/**
 * Metamethod: __newindex for primitives.
 */
static int meta_primitive_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    auto type = find_primitive_field(state, 2, "write", &ptr);
    if (!type)
        field_error(state, 2, "builtin property or method", "write");
    type->lua_write(state, 2, ptr, 3);
    return 0;
}

/**
 * Metamethod: __len for containers.
 */
static int meta_container_len(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 0, "get length");
    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_LEN);
    lua_pushinteger(state, len);
    return 1;
}

/**
 * Field lookup for containers:
 *
 *   - Numbers are indices and handled directly.
 *   - NULL userdata are metafields; push and exit;
 */
static int lookup_container_field(lua_State *state, int field, const char *mode = NULL)
{
    if (lua_type(state, field) == LUA_TNUMBER)
        return field;

    lookup_field(state, field, mode ? mode : "read");

    if (lua_isuserdata(state, -1) && !lua_touserdata(state, -1))
    {
        if (mode)
            field_error(state, field, "builtin property or method", mode);

        lua_pop(state, 1);
        get_metafield(state);
        return 0;
    }

    return -1;
}

/**
 * Index verification: number and in range.
 */
static int check_container_index(lua_State *state, int len,
                                 int fidx, int iidx, const char *mode,
                                 bool is_insert = false)
{
    if (is_insert && len >= 0)
    {
        if (lua_type(state, iidx) == LUA_TSTRING
            && strcmp(lua_tostring(state, iidx), "#") == 0)
            return len;

        len++;
    }

    if (!lua_isnumber(state, iidx))
        field_error(state, fidx, "invalid index", mode);

    int idx = lua_tointeger(state, iidx);
    if (idx < 0 || (idx >= len && len >= 0))
        field_error(state, fidx, "index out of bounds", mode);

    return idx;
}

/**
 * Metamethod: __index for containers.
 */
static int meta_container_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    int iidx = lookup_container_field(state, 2);
    if (!iidx)
        return 1;

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_READ);
    int idx = check_container_index(state, len, 2, iidx, "read");
    id->lua_item_read(state, 2, ptr, idx);
    return 1;
}

/**
 * Method: _field for containers.
 */
static int meta_container_field_reference(lua_State *state)
{
    if (lua_gettop(state) != 2)
        luaL_error(state, "Usage: object._field(index)");
    uint8_t *ptr = get_object_addr(state, 1, 2, "reference");
    int iidx = lookup_container_field(state, 2, "reference");

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_WRITE);
    int idx = check_container_index(state, len, 2, iidx, "reference");
    id->lua_item_reference(state, 2, ptr, idx);
    return 1;
}

/**
 * Metamethod: __index for containers.
 */
static int meta_container_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    int iidx = lookup_container_field(state, 2, "write");

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_WRITE);
    int idx = check_container_index(state, len, 2, iidx, "write");
    id->lua_item_write(state, 2, ptr, idx, 3);
    return 0;
}

/**
 * Metamethod: integer iterator for containers.
 */
static int meta_container_nexti(lua_State *state)
{
    if (lua_gettop(state) < 2) lua_pushnil(state);

    uint8_t *ptr = get_object_addr(state, 1, 2, "iterate");

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_LEN);
    int idx = cur_iter_index(state, len, 2);

    if (++idx >= len)
        return 0;

    lua_pushinteger(state, idx);
    id->lua_item_read(state, 2, ptr, idx);
    return 2;
}

/**
 * Metamethod: name iterator for containers.
 */
static int meta_container_next(lua_State *state)
{
    if (!meta_container_nexti(state))
        return 0;

    iter_idx_to_name(state, lua_gettop(state)-1);
    return 2;
}

/**
 * Method: resize container
 */
static int method_container_resize(lua_State *state)
{
    uint8_t *ptr = check_method_call(state, 1, 1);

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int idx = check_container_index(state, -1, UPVAL_METHOD_NAME, 2, "call");

    if (!id->resize(ptr, idx))
        field_error(state, UPVAL_METHOD_NAME, "not supported", "call");
    return 0;
}

/**
 * Method: erase item from container
 */
static int method_container_erase(lua_State *state)
{
    uint8_t *ptr = check_method_call(state, 1, 1);

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_LEN);
    int idx = check_container_index(state, len, UPVAL_METHOD_NAME, 2, "call");

    if (!id->erase(ptr, idx))
        field_error(state, UPVAL_METHOD_NAME, "not supported", "call");
    return 0;
}

/**
 * Method: insert item into container
 */
static int method_container_insert(lua_State *state)
{
    uint8_t *ptr = check_method_call(state, 2, 2);

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr, container_identity::COUNT_LEN);
    int idx = check_container_index(state, len, UPVAL_METHOD_NAME, 2, "call", true);

    if (!id->lua_insert2(state, UPVAL_METHOD_NAME, ptr, idx, 3))
        field_error(state, UPVAL_METHOD_NAME, "not supported", "call");
    return 0;
}

/**
 * Metamethod: __len for bitfields.
 */
static int meta_bitfield_len(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 0, "get size");
    (void)ptr;
    auto id = (bitfield_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    lua_pushinteger(state, id->getNumBits());
    return 1;
}

static void read_bitfield(lua_State *state, uint8_t *ptr, bitfield_identity *id, int idx)
{
    int size = std::max(1, id->getBits()[idx].size);

    int value = getBitfieldField(ptr, idx, size);
    if (size <= 1)
        lua_pushboolean(state, value != 0);
    else
        lua_pushinteger(state, value);
}

/**
 * Metamethod: __index for bitfields.
 */
static int meta_bitfield_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    int iidx = lookup_container_field(state, 2);
    if (!iidx)
        return 1;

    auto id = (bitfield_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);

    // whole
    if (lua_isuserdata(state, iidx) && lua_touserdata(state, iidx) == id)
    {
        size_t intv = 0;
        memcpy(&intv, ptr, std::min(sizeof(intv), size_t(id->byte_size())));
        lua_pushinteger(state, intv);
        return 1;
    }

    int idx = check_container_index(state, id->getNumBits(), 2, iidx, "read");
    read_bitfield(state, ptr, id, idx);
    return 1;
}

/**
 * Metamethod: __newindex for bitfields.
 */
static int meta_bitfield_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    int iidx = lookup_container_field(state, 2, "write");

    auto id = (bitfield_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);

    // whole
    if (lua_isuserdata(state, iidx) && lua_touserdata(state, iidx) == id)
    {
        if (!lua_isnumber(state, 3))
            field_error(state, 2, "number expected", "write");

        size_t intv = (size_t)lua_tonumber(state, 3);
        memcpy(ptr, &intv, std::min(sizeof(intv), size_t(id->byte_size())));
        return 0;
    }

    int idx = check_container_index(state, id->getNumBits(), 2, iidx, "write");
    int size = std::max(1, id->getBits()[idx].size);

    if (lua_isboolean(state, 3) || lua_isnil(state, 3))
        setBitfieldField(ptr, idx, size, lua_toboolean(state, 3));
    else if (lua_isnumber(state, 3))
        setBitfieldField(ptr, idx, size, lua_tointeger(state, 3));
    else
        field_error(state, 2, "number or boolean expected", "write");
    return 0;
}

/**
 * Metamethod: integer iterator for bitfields.
 */
static int meta_bitfield_nexti(lua_State *state)
{
    if (lua_gettop(state) < 2) lua_pushnil(state);

    uint8_t *ptr = get_object_addr(state, 1, 2, "iterate");

    auto id = (bitfield_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->getNumBits();
    int idx = cur_iter_index(state, len, 2);

    if (idx < 0)
        idx = 0;
    else
        idx += std::max(1, (int)id->getBits()[idx].size);

    if (idx >= len)
        return 0;

    lua_pushinteger(state, idx);
    read_bitfield(state, ptr, id, idx);
    return 2;
}

/**
 * Metamethod: name iterator for bitfields.
 */
static int meta_bitfield_next(lua_State *state)
{
    if (!meta_bitfield_nexti(state))
        return 0;

    iter_idx_to_name(state, lua_gettop(state)-1);
    return 2;
}

/**
 * Metamethod: __index for df.global
 */
static int meta_global_index(lua_State *state)
{
    auto field = (struct_field_info*)find_field(state, 2, "read");
    if (!field)
        return 1;
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "read");
    read_field(state, field, ptr);
    return 1;
}

/**
 * Metamethod: __newindex for df.global
 */
static int meta_global_newindex(lua_State *state)
{
    auto field = (struct_field_info*)find_field(state, 2, "write");
    if (!field)
        field_error(state, 2, "builtin property or method", "write");
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "write");
    write_field(state, field, ptr, 3);
    return 0;
}

/**
 * Wrapper for c++ methods and functions.
 */
static int meta_call_function(lua_State *state)
{
    auto id = (function_identity_base*)lua_touserdata(state, UPVAL_CONTAINER_ID);

    return method_wrapper_core(state, id);
}

int LuaWrapper::method_wrapper_core(lua_State *state, function_identity_base *id)
{
    if (id->adjustArgs())
        lua_settop(state, id->getNumArgs());
    else if (lua_gettop(state) != id->getNumArgs())
        field_error(state, UPVAL_METHOD_NAME, "invalid argument count", "invoke");

    try {
        id->invoke(state, 1);
    }
    catch (Error::All &e) {
        field_error(state, UPVAL_METHOD_NAME, e.what(), "invoke");
    }
    catch (std::exception &e) {
        std::string tmp = stl_sprintf("C++ exception: %s", e.what());
        field_error(state, UPVAL_METHOD_NAME, tmp.c_str(), "invoke");
    }

    return 1;
}

int Lua::CallWithCatch(lua_State *state, int (*fn)(lua_State*), const char *context)
{
    if (!context)
        context = "native code";

    try {
        return fn(state);
    }
    catch (Error::All &e) {
        return luaL_error(state, "%s: %s", context, e.what());
    }
    catch (std::exception &e) {
        return luaL_error(state, "%s: C++ exception: %s", context, e.what());
    }
}

/**
 * Push a closure invoking the given function.
 */
void LuaWrapper::PushFunctionWrapper(lua_State *state, int meta_idx,
                                     const char *name, function_identity_base *fun)
{
    lua_rawgetp(state, LUA_REGISTRYINDEX, &DFHACK_TYPETABLE_TOKEN);
    if (meta_idx)
        lua_pushvalue(state, meta_idx);
    else
        lua_pushlightuserdata(state, NULL); // can't be a metatable
    lua_pushfstring(state, "%s()", name);
    lua_pushlightuserdata(state, fun);
    lua_pushcclosure(state, meta_call_function, 4);
}

/**
 * Create a closure invoking the given function, and add it to the field table.
 */
static void AddMethodWrapper(lua_State *state, int meta_idx, int field_idx,
                             const char *name, function_identity_base *fun)
{
    PushFunctionWrapper(state, meta_idx, name, fun);
    lua_setfield(state, field_idx, name);
}

/**
 * Wrap functions and add them to the table on the top of the stack.
 */
void LuaWrapper::SetFunctionWrappers(lua_State *state, const FunctionReg *reg)
{
    int base = lua_gettop(state);

    for (; reg && reg->name; ++reg)
        AddMethodWrapper(state, 0, base, reg->name, reg->identity);
}

/**
 * Add fields in the array to the UPVAL_FIELDTABLE candidates on the stack.
 */
static void IndexFields(lua_State *state, int base, struct_identity *pstruct, bool globals)
{
    if (pstruct->getParent())
        IndexFields(state, base, pstruct->getParent(), globals);

    auto fields = pstruct->getFields();
    if (!fields)
        return;

    int cnt = lua_rawlen(state, base+3); // field iter table

    for (int i = 0; fields[i].mode != struct_field_info::END; ++i)
    {
        // Qualify conflicting field names with the type
        std::string name = fields[i].name;

        lua_getfield(state, base+2, name.c_str());
        if (!lua_isnil(state, -1))
            name = pstruct->getName() + ("." + name);
        lua_pop(state, 1);

        bool add_to_enum = true;

        // Handle the field
        switch (fields[i].mode)
        {
        case struct_field_info::OBJ_METHOD:
            AddMethodWrapper(state, base+1, base+2, name.c_str(),
                             (function_identity_base*)fields[i].type);
            continue;

        case struct_field_info::CLASS_METHOD:
            continue;

        case struct_field_info::POINTER:
            // Skip class-typed pointers within unions and other bad pointers
            if ((pstruct->type() == IDTYPE_UNION || (fields[i].count & 2) != 0) && fields[i].type)
                add_to_enum = false;
            break;

        default:
            break;
        }

        // Do not add invalid globals to the enumeration order
        if (globals && !*(void**)fields[i].offset)
            add_to_enum = false;

        if (add_to_enum)
            AssociateId(state, base+3, ++cnt, name.c_str());

        lua_pushlightuserdata(state, (void*)&fields[i]);
        lua_setfield(state, base+2, name.c_str());
    }
}

void LuaWrapper::IndexStatics(lua_State *state, int meta_idx, int ftable_idx, struct_identity *pstruct)
{
    // stack: metatable fieldtable

    for (struct_identity *p = pstruct; p; p = p->getParent())
    {
        auto fields = p->getFields();
        if (!fields)
            continue;

        for (int i = 0; fields[i].mode != struct_field_info::END; ++i)
        {
            switch (fields[i].mode)
            {
            case struct_field_info::CLASS_METHOD:
                AddMethodWrapper(state, meta_idx, ftable_idx, fields[i].name,
                                 (function_identity_base*)fields[i].type);
                break;

            default:
                break;
            }
        }
    }
}

/**
 * Make a struct-style object metatable.
 */
static void MakeFieldMetatable(lua_State *state, struct_identity *pstruct,
                               lua_CFunction reader, lua_CFunction writer, bool globals = false)
{
    int base = lua_gettop(state);

    MakeMetatable(state, pstruct, "struct"); // meta, fields

    // Index the fields
    lua_newtable(state);

    IndexFields(state, base, pstruct, globals);

    // Add the iteration metamethods
    PushStructMethod(state, base+1, base+3, meta_struct_next);
    SetPairsMethod(state, base+1, "__pairs");
    lua_pushnil(state);
    SetPairsMethod(state, base+1, "__ipairs");

    lua_setfield(state, base+1, "_index_table");

    // Add the indexing metamethods
    SetStructMethod(state, base+1, base+2, reader, "__index");
    SetStructMethod(state, base+1, base+2, writer, "__newindex");

    // returns: [metatable readfields writefields];
}

/**
 * Make a primitive-style metatable
 */
static void MakePrimitiveMetatable(lua_State *state, type_identity *type)
{
    int base = lua_gettop(state);

    MakeMetatable(state, type, "primitive");

    SetPtrMethods(state, base+1, base+2);

    // Index the fields
    lua_newtable(state);

    if (type->type() != IDTYPE_OPAQUE)
    {
        EnableMetaField(state, base+2, "value", type);
        AssociateId(state, base+3, 1, "value");
    }

    // Add the iteration metamethods
    PushStructMethod(state, base+1, base+3, meta_struct_next);
    SetPairsMethod(state, base+1, "__pairs");
    lua_pushnil(state);
    SetPairsMethod(state, base+1, "__ipairs");

    lua_setfield(state, base+1, "_index_table");

    // Add the indexing metamethods
    SetStructMethod(state, base+1, base+2, meta_primitive_index, "__index");
    SetStructMethod(state, base+1, base+2, meta_primitive_newindex, "__newindex");
}

static void AddContainerMethodFun(lua_State *state, int meta_idx, int field_idx,
                                  lua_CFunction function, const char *name,
                                  type_identity *container, type_identity *item, int count)
{
    lua_pushfstring(state, "%s()", name);
    SetContainerMethod(state, meta_idx, lua_gettop(state), function, name, container, item, count);
    lua_pop(state, 1);

    EnableMetaField(state, field_idx, name);
}

/**
 * Make a container-style object metatable.
 */
static void MakeContainerMetatable(lua_State *state, container_identity *type,
                                   type_identity *item, int count, type_identity *ienum)
{
    int base = lua_gettop(state);

    MakeMetatable(state, type, "container");
    SetPtrMethods(state, base+1, base+2);

    // Update the type name using full info
    lua_pushstring(state, type->getFullName(item).c_str());
    lua_dup(state);
    lua_setfield(state, base+1, "__metatable");
    lua_setfield(state, base+1, "_type");

    lua_pushlightuserdata(state, item);
    lua_setfield(state, base+1, "_field_identity");

    if (count >= 0)
    {
        lua_pushinteger(state, count);
        lua_setfield(state, base+1, "_count");
    }

    SetContainerMethod(state, base+1, base+2, meta_container_len, "__len", type, item, count);
    SetContainerMethod(state, base+1, base+2, meta_container_index, "__index", type, item, count);
    SetContainerMethod(state, base+1, base+2, meta_container_newindex, "__newindex", type, item, count);

    SetContainerMethod(state, base+1, base+2, meta_container_field_reference, "_field", type, item, count);

    AddContainerMethodFun(state, base+1, base+2, method_container_resize, "resize", type, item, count);
    AddContainerMethodFun(state, base+1, base+2, method_container_erase, "erase", type, item, count);
    AddContainerMethodFun(state, base+1, base+2, method_container_insert, "insert", type, item, count);

    // push the index table
    AttachEnumKeys(state, base+1, base+2, ienum);

    PushContainerMethod(state, base+1, base+3, meta_container_next, type, item, count);
    SetPairsMethod(state, base+1, "__pairs");
    PushContainerMethod(state, base+1, base+3, meta_container_nexti, type, item, count);
    SetPairsMethod(state, base+1, "__ipairs");

    lua_pop(state, 1);
}

/*
 * Metatable construction identity methods.
 */
void type_identity::build_metatable(lua_State *state)
{
    MakePrimitiveMetatable(state, this);
}

void container_identity::build_metatable(lua_State *state)
{
    MakeContainerMetatable(state, this, getItemType(), -1, getIndexEnumType());
}

void bitfield_identity::build_metatable(lua_State *state)
{
    int base = lua_gettop(state);

    MakeMetatable(state, this, "bitfield");

    SetPtrMethods(state, base+1, base+2);

    SetContainerMethod(state, base+1, base+2, meta_bitfield_len, "__len", this, NULL, -1);
    SetContainerMethod(state, base+1, base+2, meta_bitfield_index, "__index", this, NULL, -1);
    SetContainerMethod(state, base+1, base+2, meta_bitfield_newindex, "__newindex", this, NULL, -1);

    AttachEnumKeys(state, base+1, base+2, this);

    PushContainerMethod(state, base+1, base+3, meta_bitfield_next, this, NULL, -1);
    SetPairsMethod(state, base+1, "__pairs");
    PushContainerMethod(state, base+1, base+3, meta_bitfield_nexti, this, NULL, -1);
    SetPairsMethod(state, base+1, "__ipairs");

    lua_pop(state, 1);

    EnableMetaField(state, base+2, "whole", this);
}

void struct_identity::build_metatable(lua_State *state)
{
    int base = lua_gettop(state);
    MakeFieldMetatable(state, this, meta_struct_index, meta_struct_newindex);
    SetStructMethod(state, base+1, base+2, meta_struct_field_reference, "_field");
    SetPtrMethods(state, base+1, base+2);
}

void global_identity::build_metatable(lua_State *state)
{
    MakeFieldMetatable(state, this, meta_global_index, meta_global_newindex, true);
}

/**
 * Construct a metatable for an object type folded into the field descriptor.
 * This is done to reduce compile-time symbol table bloat due to templates.
 */
static void GetAdHocMetatable(lua_State *state, const struct_field_info *field)
{
    lua_pushlightuserdata(state, (void*)field);

    if (!LookupTypeInfo(state, true))
    {
        switch (field->mode)
        {
        case struct_field_info::CONTAINER:
        {
            auto ctype = (container_identity*)field->type;
            MakeContainerMetatable(state, ctype, ctype->getItemType(), -1, field->eid);
            break;
        }

        case struct_field_info::STATIC_STRING:
            MakeContainerMetatable(state, &df::buffer_container_identity::base_instance,
                                   &df::identity_traits<char>::identity, field->count, NULL);
            break;

        case struct_field_info::STATIC_ARRAY:
            MakeContainerMetatable(state, &df::buffer_container_identity::base_instance,
                                   field->type, field->count, field->eid);
            break;

        case struct_field_info::STL_VECTOR_PTR:
            MakeContainerMetatable(state, &df::identity_traits<std::vector<void*> >::identity,
                                   field->type, -1, field->eid);
            break;

        default:
            luaL_error(state, "Invalid ad-hoc field: %d", field->mode);
        }

        lua_pop(state, 1);

        SaveTypeInfo(state, (void*)field);
    }
}

void LuaWrapper::push_adhoc_pointer(lua_State *state, void *ptr, type_identity *target)
{
    if (!target)
    {
        push_object_internal(state, &df::identity_traits<void*>::identity, ptr);
        return;
    }

    LookupInTable(state, target, &DFHACK_PTR_IDTABLE_TOKEN);

    type_identity *id = (type_identity*)lua_touserdata(state, -1);
    lua_pop(state, 1);

    if (!id)
    {
        /*
         * HACK: relies on
         *   1) pointer_identity destructor being no-op
         *   2) lua gc never moving objects in memory
         */

        void *newobj = lua_newuserdata(state, sizeof(pointer_identity));
        id = new (newobj) pointer_identity(target);

        SaveInTable(state, target, &DFHACK_PTR_IDTABLE_TOKEN);
        lua_pop(state, 1);
    }

    push_object_internal(state, id, ptr);
}
