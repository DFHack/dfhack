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
#define DFHACK_CHANGEERROR_NAME "DFHack::ChangeError"
#define DFHACK_COMPARE_NAME "DFHack::ComparePtrs"
#define DFHACK_TYPE_TOSTRING_NAME "DFHack::TypeToString"

#define UPVAL_TYPETABLE lua_upvalueindex(1)
#define UPVAL_METATABLE lua_upvalueindex(2)
#define UPVAL_FIELDTABLE lua_upvalueindex(3)

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

    if (lua_isboolean(state, val_index))
        *pb = lua_toboolean(state, val_index);
    else if (lua_isnumber(state, val_index))
        *pb = lua_tonumber(state, val_index);
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
    {
        BuildTypeMetatable(state, type); // () -> metatable
        SaveTypeInfo(state, type);
    }

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

static const struct_field_info *find_field(lua_State *state, int index, const char *mode)
{
    lua_pushvalue(state, index);
    lua_rawget(state, UPVAL_FIELDTABLE);

    if (!lua_islightuserdata(state, -1))
        field_error(state, index, "not found", mode);

    void *p = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return (struct_field_info*)p;
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
        case struct_field_info::CONTAINER:
            return field->type->lua_read(state, 2, ptr);

        case struct_field_info::POINTER:
            return df::pointer_identity::lua_read(state, 2, ptr, field->type);

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:

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
    const struct_field_info *field = find_field(state, 2, "read");
    if (!field)
        return get_metafield(state);
    return read_field(state, field, ptr + field->offset);
}

static int meta_struct_newindex(lua_State *state)
{
    uint8_t *ptr = get_object_addr(state, 1, 2, "write");
    const struct_field_info *field = find_field(state, 2, "write");
    write_field(state, field, ptr + field->offset, 3);
    return 0;
}

static int meta_global_index(lua_State *state)
{
    const struct_field_info *field = find_field(state, 2, "read");
    if (!field)
        return get_metafield(state);
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "read");
    return read_field(state, field, ptr);
}

static int meta_global_newindex(lua_State *state)
{
    const struct_field_info *field = find_field(state, 2, "write");
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "write");
    write_field(state, field, ptr, 3);
    return 0;
}

static void IndexFields(lua_State *state, const struct_field_info *fields)
{
    int base = lua_gettop(state);
    lua_newtable(state); // read
    lua_newtable(state); // write

    for (; fields->mode != struct_field_info::END; ++fields)
    {
        switch (fields->mode)
        {
            case struct_field_info::END:
                break;

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

static void SetPtrMethods(lua_State *state, int meta_idx, void *node)
{
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_COMPARE_NAME);
    lua_setfield(state, meta_idx, "__eq");

    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPETABLE_NAME);
    lua_pushvalue(state, meta_idx);
    lua_pushcclosure(state, meta_ptr_tostring, 2);
    lua_setfield(state, meta_idx, "__tostring");

    // type field
    lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPEID_TABLE_NAME);
    lua_pushlightuserdata(state, node);
    lua_rawget(state, -2);
    lua_setfield(state, meta_idx, "_type");
    lua_pop(state, 1);
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

static void MakeFieldMetatable(lua_State *state, struct_identity *pstruct,
                               lua_CFunction reader, lua_CFunction writer)
{
    int base = lua_gettop(state);

    lua_newtable(state); // metatable
    IndexFields(state, pstruct->getFields()); // read, write

    lua_pushstring(state, pstruct->getName());
    lua_setfield(state, base+1, "__metatable");

    SetStructMethod(state, base+1, base+2, reader, "__index");
    SetStructMethod(state, base+1, base+3, writer, "__newindex");

    // Custom fields

    lua_pushlightuserdata(state, pstruct);
    lua_setfield(state, base+1, "_identity");

    // returns: [metatable readfields writefields];
}

static void EnableMetaField(lua_State *state, int ftable_idx, const char *name)
{
    lua_pushlightuserdata(state, NULL);
    lua_setfield(state, ftable_idx, name);
}

static void BuildTypeMetatable(lua_State *state, type_identity *type)
{
    int base = lua_gettop(state);

    switch (type->type())
    {
    case IDTYPE_GLOBAL:
        assert(false);

    case IDTYPE_STRUCT:
    case IDTYPE_CLASS:
        MakeFieldMetatable(state, (struct_identity*)type, meta_struct_index, meta_struct_newindex);
        SetPtrMethods(state, base+1, type);
        EnableMetaField(state, base+2, "_type");
        lua_pop(state, 2);
        return;

    case IDTYPE_PRIMITIVE:
    case IDTYPE_ENUM:
    case IDTYPE_POINTER:
        luaL_error(state, "primitive not implemented");

    case IDTYPE_BITFIELD:
        luaL_error(state, "bitfield not implemented");

    case IDTYPE_CONTAINER:
    case IDTYPE_STL_PTR_VECTOR:
        luaL_error(state, "container not implemented");
    }
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children);

static void RenderType(lua_State *state, compound_identity *node)
{
    assert(node->getName());

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

            SaveTypeInfo(state, node);
        }
        break;

    default:
        break;
    }

    RenderTypeChildren(state, node->getScopeChildren());

    assert(base == lua_gettop(state));

    if (node->type() == IDTYPE_GLOBAL)
    {
        auto gid = (global_identity*)node;

        MakeFieldMetatable(state, gid, meta_global_index, meta_global_newindex);
        lua_pop(state, 2);

        lua_dup(state);
        lua_setmetatable(state, base);
        lua_swap(state); // -> meta curtable

        freeze_table(state, true, "global");

        lua_getfield(state, base, "__newindex");
        lua_setfield(state, -2, "__newindex");

        lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPE_TOSTRING_NAME);
        lua_setfield(state, -2, "__tostring");

        lua_pop(state, 1);

        lua_remove(state, base);
    }
    else
    {
        freeze_table(state, true, node->getName());

        lua_getfield(state, LUA_REGISTRYINDEX, DFHACK_TYPE_TOSTRING_NAME);
        lua_setfield(state, -2, "__tostring");

        lua_pop(state, 1);
    }

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
