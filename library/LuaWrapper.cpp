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

static void field_error(lua_State *state, int index, const char *err, const char *mode = "read")
{
    lua_getfield(state, UPVAL_METATABLE, "__metatable");
    const char *cname = lua_tostring(state, -1);
    const char *fname = lua_tostring(state, index);
    luaL_error(state, "Cannot %s field %s.%s: %s.",
               mode, (cname ? cname : "?"), (fname ? fname : "?"), err);
}

static int push_object_internal(lua_State *state, type_identity *type, void *ptr, bool in_method = true);
static void *get_object_internal(lua_State *state, type_identity *type, int val_index, bool in_method = true);

int DFHack::PushDFObject(lua_State *state, type_identity *type, void *ptr)
{
    return push_object_internal(state, type, ptr, false);
}

void *DFHack::GetDFObject(lua_State *state, type_identity *type, int val_index)
{
    return get_object_internal(state, type, val_index, false);
}

/* Primitive identity methods */

int constructed_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    return push_object_internal(state, this, ptr);
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

static int do_read_pointer(lua_State *state, int, void *ptr, type_identity *target)
{
    void *val = *(void**)ptr;

    if (val == NULL)
    {
        lua_pushnil(state);
        return 1;
    }
    else
        return push_object_internal(state, target, val);
}

int df::pointer_identity::lua_read(lua_State *state, int fname_idx, void *ptr)
{
    return do_read_pointer(state, fname_idx, ptr, target);
}

static void do_write_pointer(lua_State *state, int fname_idx, void *ptr, type_identity *target, int val_index)
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
    do_write_pointer(state, fname_idx, ptr, target, val_index);
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
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.ChangeError");
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

static void SaveTypeInfo(lua_State *state, void *node)
{
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.DFTypes");
    lua_pushlightuserdata(state, node);
    lua_pushvalue(state, -3);
    lua_settable(state, -3);
    lua_pop(state, 1);
}

static bool RegisterTypeInfo(lua_State *state, void *node)
{
    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack.DFTypes");
    int base = lua_gettop(state);

    lua_pushlightuserdata(state, node);
    lua_rawget(state, base);

    bool added = false;

    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);

        lua_newtable(state);
        lua_pushlightuserdata(state, node);
        lua_pushvalue(state, -2);
        lua_rawset(state, base);

        added = true;
    }

    lua_remove(state, -2);
    return added;
}

static const struct_field_info *find_field(lua_State *state, int index, const char *mode = "read")
{
    lua_pushvalue(state, index);
    lua_rawget(state, UPVAL_FIELDTABLE);

    if (!lua_islightuserdata(state, -1))
        field_error(state, index, "not found");

    void *p = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return (struct_field_info*)p;
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
            return do_read_pointer(state, 2, ptr, field->type);

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:

        case struct_field_info::END:
            return 0;
    }
}

static void write_field(lua_State *state, const struct_field_info *field, void *ptr)
{
    switch (field->mode)
    {
        case struct_field_info::STATIC_STRING:
        {
            size_t size;
            const char *str = lua_tolstring(state, -1, &size);
            if (!str)
                field_error(state, 2, "string expected", "write");
            memcpy(ptr, str, std::min(size+1, size_t(field->count)));
            return;
        }

        case struct_field_info::PRIMITIVE:
        case struct_field_info::SUBSTRUCT:
        case struct_field_info::CONTAINER:
            field->type->lua_write(state, 2, ptr, 3);
            return;

        case struct_field_info::POINTER:
            do_write_pointer(state, 2, ptr, field->type, 3);

        case struct_field_info::STATIC_ARRAY:
        case struct_field_info::STL_VECTOR_PTR:
            field_error(state, 2, "complex object", "write");

        case struct_field_info::END:
            return;
    }
}

static int meta_global_index(lua_State *state)
{
    const struct_field_info *field = find_field(state, 2);
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known");
    return read_field(state, field, ptr);
}

static int meta_global_newindex(lua_State *state)
{
    const struct_field_info *field = find_field(state, 2);
    void *ptr = *(void**)field->offset;
    if (!ptr)
        field_error(state, 2, "global address not known", "write");
    write_field(state, field, ptr);
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
                lua_settable(state,base+2);
                // fallthrough

            case struct_field_info::STATIC_ARRAY:
            case struct_field_info::SUBSTRUCT:
            case struct_field_info::CONTAINER:
            case struct_field_info::STL_VECTOR_PTR:
                lua_pushstring(state,fields->name);
                lua_pushlightuserdata(state,(void*)fields);
                lua_settable(state,base+1);
                break;
        }
    }
}

static void MakeFieldMetatable(lua_State *state, struct_identity *pstruct,
                               lua_CFunction reader, lua_CFunction writer)
{
    int base = lua_gettop(state);

    lua_newtable(state); // metatable
    IndexFields(state, pstruct->getFields()); // read, write

    lua_pushstring(state, pstruct->getName());
    lua_setfield(state, base+1, "__metatable");

    lua_pushlightuserdata(state, pstruct);
    lua_setfield(state, base+1, "_identity");

    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack::DFTypes");
    lua_pushvalue(state, base+1);
    lua_pushvalue(state, base+2);
    lua_pushcclosure(state, reader, 3);
    lua_setfield(state, base+1, "__index");

    lua_getfield(state, LUA_REGISTRYINDEX, "DFHack::DFTypes");
    lua_pushvalue(state, base+1);
    lua_pushvalue(state, base+3);
    lua_pushcclosure(state, writer, 3);
    lua_setfield(state, base+1, "__newindex");

    // returns: [metatable readfields writefields];
}

static int push_object_internal(lua_State *state, type_identity *type, void *ptr, bool in_method)
{
    return 0;
}

static void *get_object_internal(lua_State *state, type_identity *type, int val_index, bool in_method)
{
    return NULL;
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

                lua_settable(state, base);
                lua_settable(state, base);
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
        lua_pop(state, 1);

        lua_remove(state, base);
    }
    else
        freeze_table(state, false, node->getName());
}

static void RenderTypeChildren(lua_State *state, const std::vector<compound_identity*> &children)
{
    for (size_t i = 0; i < children.size(); i++)
    {
        RenderType(state, children[i]);
        lua_pushstring(state, children[i]->getName());
        lua_swap(state);
        lua_settable(state, -3);
    }
}

static void DoAttach(lua_State *state)
{
    int base = lua_gettop(state);

    lua_pushcfunction(state, change_error);
    lua_setfield(state, LUA_REGISTRYINDEX, "DFHack.ChangeError");

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
    if (luaL_newmetatable(state, "DFHack.DFTypes"))
        DoAttach(state);

    lua_pop(state, 1);
}
