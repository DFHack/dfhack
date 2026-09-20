// Regression test for https://github.com/DFHack/dfhack/issues/5450
// (fixed by https://github.com/DFHack/dfhack/pull/5451)
//
// df::identity_traits<T>::get() returns a pointer to a *const* instance of
// a type_identity subclass for most types. When the df_object constraint
// required a pointer to non-const type_identity, Lua::Push rejected those
// types and fell back to the bool overload, so e.g. a pointer to a vector
// arrived in Lua as a boolean.

#include "LuaTools.h"
#include "LuaWrapper.h"

#include <gtest/gtest.h>
#include <lualib.h>

#include <cstdint>
#include <vector>

// The constraint must accept types whose identity is exposed through a
// pointer-to-const. This is the compile-time half of the regression check:
// it fails to build if df_object ever becomes too strict again.
static_assert(DFHack::Lua::df_object<std::vector<int32_t>>);
static_assert(DFHack::Lua::df_object<int32_t>);
static_assert(DFHack::Lua::df_object<df::coord>);

class LuaPushTest : public testing::Test {
protected:
    lua_State *L = nullptr;

    void SetUp() override
    {
        L = luaL_newstate();
        ASSERT_NE(nullptr, L);
        luaL_openlibs(L);
        DFHack::LuaWrapper::AttachDFGlobals(L);
    }

    void TearDown() override
    {
        if (L)
            lua_close(L);
    }
};

TEST_F(LuaPushTest, PushVectorPointer)
{
    std::vector<int32_t> vec{ 7, 42 };

    DFHack::Lua::Push(L, &vec);

    // the pointer must arrive as a wrapped DF object, not a boolean
    ASSERT_EQ(LUA_TUSERDATA, lua_type(L, -1));
    ASSERT_EQ(DFHack::Lua::OBJ_REF, DFHack::Lua::IsDFObject(L, -1));

    // the same pointer must round-trip back out of Lua
    EXPECT_EQ(&vec, DFHack::Lua::GetDFObject<std::vector<int32_t>>(L, -1));

    // and the pushed object must behave like the vector it wraps
    lua_len(L, -1);
    EXPECT_EQ(lua_Integer(vec.size()), lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_geti(L, -1, 0);
    EXPECT_EQ(7, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_geti(L, -1, 1);
    EXPECT_EQ(42, lua_tointeger(L, -1));
    lua_pop(L, 1);
}
