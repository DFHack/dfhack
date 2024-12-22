#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "VersionInfo.h"
#include "VTableInterpose.h"
#include "LuaTools.h"

#include "DataDefs.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/init.h"
#include "df/renderer.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/map_renderer.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("map-render");
REQUIRE_GLOBAL(window_x)
REQUIRE_GLOBAL(window_y)
REQUIRE_GLOBAL(window_z)
REQUIRE_GLOBAL_NO_USE(gps)
REQUIRE_GLOBAL_NO_USE(enabler)
REQUIRE_GLOBAL_NO_USE(twbt_render_map)
REQUIRE_GLOBAL(init)

#ifdef WIN32
    // On Windows there's no parameter pointing to the map_renderer structure
    typedef void(_stdcall *RENDER_MAP)(int);

    RENDER_MAP _render_map;

    void render_map(){ _render_map(0); }
#else
REQUIRE_GLOBAL(map_renderer)

    typedef void(*RENDER_MAP)(void*, int);

    RENDER_MAP _render_map;

    void render_map(){ _render_map(map_renderer,0); }
#endif
static int render_map_rect(lua_State* L)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int z = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);
    uint8_t *s = df::global::gps->screen;
    //backup state
    //TODO: figure out if we can replace screen with other pointer. That way it could be a bit more tidy
    int32_t win_h = df::global::gps->dimy;
    int32_t was_x = *window_x;
    int32_t was_y = *window_y;
    int32_t was_z = *window_z;
    int32_t gx = init->display.grid_x;
    int32_t gy = init->display.grid_y;
    init->display.grid_x = w+1;
    init->display.grid_y = h+1;
    *window_x = x;
    *window_y = y;
    *window_z = z;
    //force full redraw
    df::global::gps->force_full_display_count = 1;
    //this modifies screen so it REALLY wants to redraw stuff
    for (int ty = 0; ty < h; ty++)
    for (int tx = 0; tx < w; tx++)
    {
        for (int i = 0; i < 4; i++)
        {
            int t = (tx + 1)*win_h + ty + 1;
            s[t * 4 + i] = 0;
        }
    }
    render_map();
    //restore state
    *window_x = was_x;
    *window_y = was_y;
    *window_z = was_z;
    init->display.grid_x = gx;
    init->display.grid_y = gy;

    lua_createtable(L,w*h*4,0);

    int counter = 0;
    for (int ty = 0; ty < h; ty++)
    for (int tx = 0; tx < w; tx++)
    {
        for (int i = 0; i < 4;i++)
        {
            int t = (tx + 1)*win_h + ty + 1;
            lua_pushnumber(L, s[t*4+i]);
            lua_rawseti(L, -2, counter);
            counter++;
        }
    }
    return 1;
}

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(render_map_rect),
    DFHACK_LUA_END
};

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands)
{
    auto addr =reinterpret_cast<RENDER_MAP>(Core::getInstance().vinfo->getAddress("twbt_render_map"));
    if (addr == nullptr)
        return CR_FAILURE;
    _render_map = addr;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}
