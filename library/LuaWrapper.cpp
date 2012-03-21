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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "VersionInfo.h"
#include "tinythread.h"
// must be last due to MS stupidity
#include "DataDefs.h"
#include "DataIdentity.h"

#include "MiscUtils.h"

#include <lua.h>
#include <lauxlib.h>

using namespace DFHack;

static luaL_Reg no_functions[] = { { NULL, NULL } };

inline void lua_dup(lua_State *state) { lua_pushvalue(state, -1); }
inline void lua_swap(lua_State *state) { lua_insert(state, -2); }

#define DFHACK_TYPETABLE_NAME "DFHack::DFTypes"
#define DFHACK_TYPEID_TABLE_NAME "DFHack::DFTypeIds"
#define DFHACK_ENUM_TABLE_NAME "DFHack::DFEnums"
#define DFHACK_CHANGEERROR_NAME "DFHack::ChangeError"
#define DFHACK_COMPARE_NAME "DFHack::ComparePtrs"
#define DFHACK_TYPE_TOSTRING_NAME "DFHack::TypeToString"

#define UPVAL_TYPETABLE lua_upvalueindex(1)
#define UPVAL_METATABLE lua_upvalueindex(2)
#define UPVAL_FIELDTABLE lua_upvalueindex(3)

#define UPVAL_CONTAINER_ID lua_upvalueindex(4)
#define UPVAL_ITEM_ID lua_upvalueindex(5)
#define UPVAL_ITEM_COUNT lua_upvalueindex(6)

namespace {
    struct DFRefHeader {
        void *ptr;
    };

    inline bool is_self_contained(DFRefHeader *ptr) {
        void **pp = &ptr->ptr;
        return **(void****)pp == (pp + 1);
    }
}

static void field_error(lua_State *state, int index, const char *err, const char *mode)
{
    lua_getfield(state, UPVAL_METATABLE, "__metatable");
    const char *cname = lua_tostring(state, -1);
    const char *fname = lua_tostring(state, index);
    luaL_error(state, "Cannot %s field %s.%s: %s.",
               mode, (cname ? cname : "?"), (fname ? fname : "?"), err);
}

static void push_object_internal(lua_State *state, type_identity *type, void *ptr, bool in_method = true);
static void *get_object_internal(lua_State *state, type_identity *type, int val_index, bool in_method = true);

void DFHack::PushDFObject(lua_State *state, type_identity *type, void *ptr)
{
    push_object_internal(state, type, ptr, false);
}

void *DFHack::GetDFObject(lua_State *state, type_identity *type, int val_index)
{
    return get_object_internal(state, type, val_index, false);
}

/* Primitive identity methods */

int constructed_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    push_object_internal(state, this, ptr);
    return 1;
}

void constructed_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    field_error(state, fname_idx, "complex object", "write");
}

int enum_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    return base_type->lua_read(state, fname_idx, ptr);
}

void enum_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    base_type->lua_write(state, fname_idx, ptr, val_index);
}

int df::number_identity_base::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_pushnumber(state, read(ptr));
    return 1;
}

void df::number_identity_base::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    if (!lua_isnumber(state, val_index))
        field_error(state, fname_idx, "number expected", "write");

    write(ptr, lua_tonumber(state, val_index));
}

int df::bool_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    lua_pushboolean(state, *(bool*)ptr);
    return 1;
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

int df::stl_string_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    auto pstr = (std::string*)ptr;
    lua_pushlstring(state, pstr->data(), pstr->size());
    return 1;
}

void df::stl_string_identity::lua_write(lua_State *state, int fname_idx, void *ptr, int val_index)
{
    size_t size;
    const char *bytes = lua_tolstring(state, val_index, &size);
    if (!bytes)
        field_error(state, fname_idx, "string expected", "write");

    *(std::string*)ptr = std::string(bytes, size);
}

int df::pointer_identity::lua_read(lua_State *state, int fname_idx, void *ptr, type_identity *target)
{
    push_object_internal(state, target, *(void**)ptr);
    return 1;
}

int df::pointer_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    return lua_read(state, fname_idx, ptr, target);
}

void df::pointer_identity::lua_write(lua_State *state, int fname_idx, void *ptr,
                                     type_identity *target, int val_index)
{
    auto pptr = (void**)ptr;

    if (lua_isnil(state, val_index))
        *pptr = NULL;
    else
    {
        void *nval = get_object_internal(state, target, val_index);
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

int container_identity::lua_item_count(lua_State *state, void *ptr)
{
    if (lua_isnumber(state, UPVAL_ITEM_COUNT))
        return lua_tointeger(state, UPVAL_ITEM_COUNT);
    else
        return item_count(ptr);
}

int container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    void *pitem = item_pointer(ptr, idx);
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    return id->lua_read(state, fname_idx, pitem);
}

void container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    void *pitem = item_pointer(ptr, idx);
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    id->lua_write(state, fname_idx, pitem, val_index);
}

int ptr_container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    void *pitem = item_pointer(ptr, idx);
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    return df::pointer_identity::lua_read(state, fname_idx, pitem, id);
}

void ptr_container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    void *pitem = item_pointer(ptr, idx);
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    df::pointer_identity::lua_write(state, fname_idx, pitem, id, val_index);
}

int bit_container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    lua_pushboolean(state, get_item(ptr, idx));
    return 1;
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

int df::buffer_container_identity::lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = ((uint8_t*)ptr) + idx * id->byte_size();
    return id->lua_read(state, fname_idx, pitem);
}

void df::buffer_container_identity::lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index)
{
    auto id = (type_identity*)lua_touserdata(state, UPVAL_ITEM_ID);
    void *pitem = ((uint8_t*)ptr) + idx * id->byte_size();
    id->lua_write(state, fname_idx, pitem, val_index);
}

/* */

static int change_error(lua_State *state)
{
    luaL_error(state, "Attempt to change a read-only table.\n");
}

static void freeze_table(lua_State *state, bool leave_metatable = false, const char *name = NULL)
{
    // rv = {}; setmetatable(rv, { __index = in, __newindex = change_error, __metatable = name })
    int base = lua_gettop(state);
    lua_newtable(state);
    lua_swap(state);
    lua_setfield(state, base, "__index");
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_CHANGEERROR_NAME);
    lua_setfield(state, base, "__newindex");
    lua_newtable(state);
    lua_swap(state);
    lua_dup(state);
    lua_setmetatable(state, base);
    if (name)
    {
        lua_pushstring(state, name);
        lua_setfield(state, -2, "__metatable");
    }
    // result: [frozen table] [metatable]
    if (!leave_metatable)
        lua_pop(state, 1);
}

static bool LookupTypeInfo(lua_State *state, bool in_method)
{
    // stack: [lookup key]

    if (in_method)
    {
        lua_rawget(state, UPVAL_TYPETABLE);
    }
    else
    {
        lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPETABLE_NAME);
        lua_swap(state);
        lua_rawget(state, -2);
        lua_remove(state, -2);
    }

    // stack: [info]

    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);
        return false;
    }
    else
        return true;
}

static void LookupInTable(lua_State *state, void *id, const char *tname)
{
    lua_getfield(state, LUA_REGISTRYINDEX, tname);
    lua_pushlightuserdata(state, id);
    lua_rawget(state, -2);
    lua_remove(state, -2);
}

static void SaveInTable(lua_State *state, void *node, const char *tname)
{
    // stack: [info]
    lua_getfield(state, LUA_REGISTRYINDEX, tname);

    lua_pushlightuserdata(state, node);
    lua_pushvalue(state, -3);
    lua_rawset(state, -3);

    lua_pushvalue(state, -2);
    lua_pushlightuserdata(state, node);
    lua_rawset(state, -3);

    lua_pop(state, 1);
    // stack: [info]
}

static void SaveTypeInfo(lua_State *state, void *node)
{
    SaveInTable(state, node, DFHACK_TYPETABLE_NAME);
}

static void BuildTypeMetatable(lua_State *state, type_identity *type);

static void push_object_ref(lua_State *state, void *ptr)
{
    // stack: [metatable]
    auto ref = (DFRefHeader*)lua_newuserdata(state, sizeof(DFRefHeader));
    ref->ptr = ptr;

    lua_swap(state);
    lua_setmetatable(state, -2);
    // stack: [userdata]
}

static void push_object_internal(lua_State *state, type_identity *type, void *ptr, bool in_method)
{
    if (!ptr || !type)
    {
        if (!ptr)
            lua_pushnil(state);
        else
            lua_pushlightuserdata(state, ptr);

        return;
    }

    // Resolve actual class using vtable
    if (type->type() == IDTYPE_CLASS)
    {
        virtual_identity *class_vid = virtual_identity::get(virtual_ptr(ptr));
        if (class_vid)
            type = class_vid;
    }

    lua_pushlightuserdata(state, type); // () -> type

    if (!LookupTypeInfo(state, in_method)) // type -> metatable?
        BuildTypeMetatable(state, type); // () -> metatable

    push_object_ref(state, ptr); // metatable -> userdata
}

static void *get_object_internal(lua_State *state, type_identity *type, int val_index, bool in_method)
{
    if (!lua_isuserdata(state, val_index))
        return NULL;

    if (!lua_getmetatable(state, val_index)) // () -> metatable?
    {
        if (!type && lua_islightuserdata(state, val_index))
            return lua_touserdata(state, val_index);

        return NULL;
    }

    // Verify known metatable and correct type
    if (!LookupTypeInfo(state, in_method)) // metatable -> type?
        return NULL;

    bool ok = lua_islightuserdata(state, -1) &&
              (!type || lua_touserdata(state, -1) == type);

    lua_pop(state, 1); // type -> ()

    if (!ok)
        return NULL;

    auto ref = (DFRefHeader*)lua_touserdata(state, val_index);
    return ref->ptr;
}


static int meta_ptr_compare(lua_State *state)
{
    if (!lua_isuserdata(state, 1) || !lua_isuserdata(state, 2) ||
        !lua_getmetatable(state, 1) || !lua_getmetatable(state, 2))
    {
        lua_pushboolean(state, false);
        return 1;
    }

    if (!lua_equal(state, -1, -2))
    {
        // todo: nonidentical type comparison
        lua_pushboolean(state, false);
        return 1;
    }

    auto ref1 = (DFRefHeader*)lua_touserdata(state, 1);
    auto ref2 = (DFRefHeader*)lua_touserdata(state, 2);
    lua_pushboolean(state, ref1->ptr == ref2->ptr);
    return 1;
}

static void lookup_field(lua_State *state, int index, const char *mode)
{
    lua_pushvalue(state, index);
    lua_gettable(state, UPVAL_FIELDTABLE); // uses metatable with enum keys

    if (lua_isnil(state, -1))
        field_error(state, index, "not found", mode);
}

static void *find_field(lua_State *state, int index, const char *mode)
{
    lookup_field(state, index, mode);

    void *p = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return p;
}

static uint8_t *get_object_addr(lua_State *state, int obj, int field, const char *mode)
{
    if (!lua_isuserdata(state, obj) ||
        !lua_getmetatable(state, obj))
        field_error(state, field, "invalid object", mode);

    if (!lua_equal(state, -1, UPVAL_METATABLE))
        field_error(state, field, "invalid object metatable", mode);

    lua_pop(state, 1);

    auto ref = (DFRefHeader*)lua_touserdata(state, obj);
    return (uint8_t*)ref->ptr;
}

static void GetAdHocMetatable(lua_State *state, const struct_field_info *field);

static int read_field(lua_State *state, const struct_field_info *field, void *ptr)
{
    switch (field->mode)
    {
        case struct_field_info::STATIC_STRING:
        {
            int len = strnlen((char*)ptr, field->count);
            lua_pushlstring(state, (char*)ptr, len);
            return 1;
        }

        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
            return field->type->lua_read(state, 2, ptr);

        case struct_field_info::POINTER:
            return df::pointer_identity::lua_read(state, 2, ptr, field->type);

        case struct_field_info::CONTAINER:
            if (!field->eid || !field->type->isContainer() ||
                field->eid == ((container_identity*)field->type)->getIndexEnumType())
                return field->type->lua_read(state, 2, ptr);

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            GetAdHocMetatable(state, field);
            push_object_ref(state, ptr);
            return 1;

        case struct_field_info::END:
            return 0;
    }
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

        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
        case struct_field_info::CONTAINER:
            field->type->lua_write(state, 2, ptr, value_idx);
            return;

        case struct_field_info::POINTER:
            df::pointer_identity::lua_write(state, 2, ptr, field->type, value_idx);

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            field_error(state, 2, "complex object", "write");

        case struct_field_info::END:
            return;
    }
}

static int meta_type_tostring(lua_State *state)
{
    if (!lua_getmetatable(state, 1))
        return 0;

    lua_getfield(state, -1, "__metatable");
    const char *cname = lua_tostring(state, -1);

    lua_pushstring(state, stl_sprintf("<type: %s>", cname).c_str());
    return 1;
}

static int meta_ptr_tostring(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 0, "access");

    lua_getfield(state, UPVAL_METATABLE, "__metatable");
    const char *cname = lua_tostring(state, -1);

    lua_pushstring(state, stl_sprintf("<%s: 0x%08x>", cname, (unsigned)ptr).c_str());
    return 1;
}

static int get_metafield(lua_State *state)
{
    lua_rawget(state, UPVAL_METATABLE);
    return 1;
}

static int meta_struct_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    auto field = (struct_field_info*)find_field(state, 2, "read");
    if (!field)
        return get_metafield(state);
    return read_field(state, field, ptr + field->offset);
}

static int meta_struct_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    auto field = (struct_field_info*)find_field(state, 2, "write");
    write_field(state, field, ptr + field->offset, 3);
    return 0;
}

static int meta_primitive_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    auto type = (type_identity*)find_field(state, 2, "read");
    if (!type)
        return get_metafield(state);
    return type->lua_read(state, 2, ptr);
}

static int meta_primitive_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    auto type = (type_identity*)find_field(state, 2, "write");
    type->lua_write(state, 2, ptr, 3);
    return 0;
}

static int meta_container_len(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 0, "get length");
    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int len = id->lua_item_count(state, ptr);
    lua_pushinteger(state, len);
    return 1;
}

static int lookup_container_field(lua_State *state, int field, const char *mode)
{
    if (lua_type(state, field) == LUA_TNUMBER)
        return field;

    lookup_field(state, field, mode);
    return -1;
}

static int check_container_index(lua_State *state, container_identity *container, void *ptr,
                                 int fidx, int iidx, const char *mode)
{
    if (!lua_isnumber(state, iidx))
        field_error(state, fidx, "invalid index", mode);

    int idx = lua_tointeger(state, iidx);
    int len = container->lua_item_count(state, ptr);
    if (idx < 0 || idx >= len)
        field_error(state, fidx, "index out of bounds", mode);

    return idx;
}

static int meta_container_index(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "read");
    int iidx = lookup_container_field(state, 2, "read");
    if (lua_isuserdata(state, iidx))
    {
        lua_pop(state, 1);
        return get_metafield(state);
    }

    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int idx = check_container_index(state, id, ptr, 2, iidx, "read");
    return id->lua_item_read(state, 2, ptr, idx);
}

static int meta_container_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    int iidx = lookup_container_field(state, 2, "write");
    auto id = (container_identity*)lua_touserdata(state, UPVAL_CONTAINER_ID);
    int idx = check_container_index(state, id, ptr, 2, iidx, "write");
    id->lua_item_write(state, 2, ptr, idx, 3);
    return 0;
}

static int meta_global_index(lua_State *state)
{
    auto field = (struct_field_info*)find_field(state, 2, "read");
    if (!field)
        return get_metafield(state);
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "read");
    return read_field(state, field, ptr);
}

static int meta_global_newindex(lua_State *state)
{
    auto field = (struct_field_info*)find_field(state, 2, "write");
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "write");
    write_field(state, field, ptr, 3);
    return 0;
}

static void IndexFields(lua_State *state, const struct_field_info *fields)
{
    // stack: read write

    int base = lua_gettop(state) - 2;

    for (; fields; ++fields)
    {
        switch (fields->mode)
        {
            case struct_field_info::END:
                return;

            case struct_field_info::PRIMITIVE:
            case struct_field_info::STATIC_STRING:
            case struct_field_info::POINTER:
                lua_pushstring(state,fields->name);
                lua_pushlightuserdata(state,(void*)fields);
                lua_rawset(state,base+2);
                // fallthrough

            case struct_field_info::STATIC_ARRAY:
            case struct_field_info::SUBSTRUCT:
            case struct_field_info::CONTAINER:
            case struct_field_info::STL_VECTOR_PTR:
                lua_pushstring(state,fields->name);
                lua_pushlightuserdata(state,(void*)fields);
                lua_rawset(state,base+1);
                break;
        }
    }
}

static void EnableMetaField(lua_State *state, int ftable_idx, const char *name, void *id = NULL)
{
    lua_pushlightuserdata(state, id);
    lua_setfield(state, ftable_idx, name);
}

static void SetPtrMethods(lua_State *state, int meta_idx, int read_idx)
{
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_COMPARE_NAME);
    lua_setfield(state, meta_idx, "__eq");

    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPETABLE_NAME);
    lua_pushvalue(state, meta_idx);
    lua_pushcclosure(state, meta_ptr_tostring, 2);
    lua_setfield(state, meta_idx, "__tostring");

    EnableMetaField(state, read_idx, "_type");
}

static void SetStructMethod(lua_State *state, int meta_idx, int ftable_idx,
                            lua_CFunction function, const char *name)
{
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPETABLE_NAME);
    lua_pushvalue(state, meta_idx);
    lua_pushvalue(state, ftable_idx);
    lua_pushcclosure(state, function, 3);
    lua_setfield(state, meta_idx, name);
}

static void MakeMetatable(lua_State *state, type_identity *type)
{
    int base = lua_gettop(state);
    lua_newtable(state); // metatable

    lua_pushstring(state, type->getFullName().c_str());
    lua_setfield(state, base+1, "__metatable");

    lua_pushlightuserdata(state, type);
    lua_setfield(state, base+1, "_identity");

    LookupInTable(state, type, DFHACK_TYPEID_TABLE_NAME);
    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);
        lua_getfield(state, base+1, "__metatable");
    }
    lua_setfield(state, base+1, "_type");

    lua_newtable(state); // read
    lua_newtable(state); // write
}

static void MakeFieldMetatable(lua_State *state, struct_identity *pstruct,
                               lua_CFunction reader, lua_CFunction writer)
{
    int base = lua_gettop(state);

    MakeMetatable(state, pstruct); // meta, read, write

    for (struct_identity *p = pstruct; p; p = p->getParent())
    {
        IndexFields(state, p->getFields());
    }

    SetStructMethod(state, base+1, base+2, reader, "__index");
    SetStructMethod(state, base+1, base+3, writer, "__newindex");

    // returns: [metatable readfields writefields];
}

static void MakePrimitiveMetatable(lua_State *state, type_identity *type)
{
    int base = lua_gettop(state);

    MakeMetatable(state, type);
    SetPtrMethods(state, base+1, base+2);

    EnableMetaField(state, base+2, "value", type);
    EnableMetaField(state, base+3, "value", type);

    SetStructMethod(state, base+1, base+2, meta_primitive_index, "__index");
    SetStructMethod(state, base+1, base+3, meta_primitive_newindex, "__newindex");
}

static void SetContainerMethod(lua_State *state, int meta_idx, int ftable_idx,
                               lua_CFunction function, const char *name,
                               type_identity *container, type_identity *item, int count)
{
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPETABLE_NAME);
    lua_pushvalue(state, meta_idx);
    lua_pushvalue(state, ftable_idx);

    lua_pushlightuserdata(state, container);
    lua_pushlightuserdata(state, item);
    if (count < 0)
        lua_pushnil(state);
    else
        lua_pushinteger(state, count);

    lua_pushcclosure(state, function, 6);
    lua_setfield(state, meta_idx, name);
}

static void AttachEnumKeys(lua_State *state, int base, type_identity *ienum)
{
    LookupInTable(state, ienum, DFHACK_ENUM_TABLE_NAME);

    if (!lua_isnil(state, -1))
    {
        lua_newtable(state);
        lua_swap(state);
        lua_setfield(state, -2, "__index");
        lua_dup(state);
        lua_setmetatable(state, base+2);
        lua_setmetatable(state, base+3);
    }
    else
        lua_pop(state, 1);
}

static void MakeContainerMetatable(lua_State *state, container_identity *type,
                                   type_identity *item, int count, type_identity *ienum)
{
    int base = lua_gettop(state);

    MakeMetatable(state, type);
    SetPtrMethods(state, base+1, base+2);

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
    SetContainerMethod(state, base+1, base+3, meta_container_newindex, "__newindex", type, item, count);

    AttachEnumKeys(state, base, ienum);
}

void type_identity::build_metatable(lua_State *state)
{
    MakePrimitiveMetatable(state, this);
}

void container_identity::build_metatable(lua_State *state)
{
    MakeContainerMetatable(state, this, getItemType(), -1, getIndexEnumType());
}

void pointer_identity::build_metatable(lua_State *state)
{
    int base = lua_gettop(state);

    primitive_identity::build_metatable(state);

    EnableMetaField(state, base+2, "target", this);
    EnableMetaField(state, base+3, "target", this);
}

void struct_identity::build_metatable(lua_State *state)
{
    int base = lua_gettop(state);
    MakeFieldMetatable(state, this, meta_struct_index, meta_struct_newindex);
    SetPtrMethods(state, base+1, base+2);
}

void global_identity::build_metatable(lua_State *state)
{
    MakeFieldMetatable(state, this, meta_global_index, meta_global_newindex);
}

static void BuildTypeMetatable(lua_State *state, type_identity *type)
{
    type->build_metatable(state);

    lua_pop(state, 2);

    SaveTypeInfo(state, type);
}

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

        lua_pop(state, 2);

        SaveTypeInfo(state, (void*)field);
    }
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children);

static void RenderType(lua_State *state, compound_identity *node)
{
    assert(node->getName());
    std::string name = node->getFullName();

    lua_newtable(state);
    if (!lua_checkstack(state, 20))
        return;

    int base = lua_gettop(state);

    lua_pushlightuserdata(state, node);
    lua_setfield(state, base, "_identity");

    switch (node->type())
    {
    case IDTYPE_ENUM:
        {
            enum_identity *eid = (enum_identity*)node;
            const char *const *keys = eid->getKeys();

            // For enums, set mapping between keys and values
            for (int64_t i = eid->getFirstItem(), j = 0; i <= eid->getLastItem(); i++, j++)
            {
                if (!keys[j])
                    continue;

                lua_pushinteger(state, i);
                lua_pushstring(state, keys[j]);
                lua_dup(state);
                lua_pushinteger(state, i);

                lua_rawset(state, base);
                lua_rawset(state, base);
            }

            if (eid->getFirstItem() <= eid->getLastItem())
            {
                lua_pushinteger(state, eid->getFirstItem());
                lua_setfield(state, base, "_first_item");

                lua_pushinteger(state, eid->getLastItem());
                lua_setfield(state, base, "_last_item");
            }

            SaveInTable(state, node, DFHACK_ENUM_TABLE_NAME);
        }
        break;

    default:
        break;
    }

    RenderTypeChildren(state, node->getScopeChildren());

    assert(base == lua_gettop(state));

    if (node->type() == IDTYPE_GLOBAL)
    {
        BuildTypeMetatable(state, node);

        lua_dup(state);
        lua_setmetatable(state, base);
        lua_swap(state); // -> meta curtable

        freeze_table(state, true, "global");

        lua_getfield(state, base, "__newindex");
        lua_setfield(state, -2, "__newindex");

        lua_remove(state, base);
    }
    else
    {
        freeze_table(state, true, name.c_str());
    }

    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPE_TOSTRING_NAME);
    lua_setfield(state, -2, "__tostring");

    lua_pop(state, 1);

    SaveInTable(state, node, DFHACK_TYPEID_TABLE_NAME);
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children)
{
    for (size_t i = 0; i < children.size(); i++)
    {
        RenderType(state, children[i]);
        lua_pushstring(state, children[i]->getName());
        lua_swap(state);
        lua_rawset(state, -3);
    }
}

static void DoAttach(lua_State *state)
{
    int base = lua_gettop(state);

    lua_newtable(state);
    lua_setfield(state, LUA_REGISTRYINDEX, DFHACK_TYPEID_TABLE_NAME);

    lua_newtable(state);
    lua_setfield(state, LUA_REGISTRYINDEX, DFHACK_ENUM_TABLE_NAME);

    lua_pushcfunction(state, change_error);
    lua_setfield(state, LUA_REGISTRYINDEX, DFHACK_CHANGEERROR_NAME);

    lua_pushcfunction(state, meta_ptr_compare);
    lua_setfield(state, LUA_REGISTRYINDEX, DFHACK_COMPARE_NAME);

    lua_pushcfunction(state, meta_type_tostring);
    lua_setfield(state, LUA_REGISTRYINDEX, DFHACK_TYPE_TOSTRING_NAME);

    luaL_register(state, "df", no_functions);

    {
        // Assign df a metatable with read-only contents
        lua_newtable(state);

        // Render the type structure
        RenderTypeChildren(state, compound_identity::getTopScope());

        freeze_table(state, true, "df");
        lua_remove(state, -2);
        lua_setmetatable(state, -2);
    }

    lua_pop(state, 1);
}

void DFHack::AttachDFGlobals(lua_State *state)
{
    if (luaL_newmetatable(state, DFHACK_TYPETABLE_NAME))
        DoAttach(state);

    lua_pop(state, 1);
}
