#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "df/enabler.h"
#include "df/renderer.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dwarfmodest.h"

#include "renderer_opengl.hpp"
#include "renderer_light.hpp"

using df::viewscreen_dungeonmodest;
using df::viewscreen_dwarfmodest;

using namespace DFHack;
using std::vector;
using std::string;

DFHACK_PLUGIN("rendermax");
REQUIRE_GLOBAL(cur_year_tick);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(enabler);
REQUIRE_GLOBAL(gametype);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);
REQUIRE_GLOBAL(world);

enum RENDERER_MODE
{
    MODE_DEFAULT,MODE_TRIPPY,MODE_TRUECOLOR,MODE_LUA,MODE_LIGHT
};
RENDERER_MODE current_mode=MODE_DEFAULT;
lightingEngine *engine=NULL;

static command_result rendermax(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rendermax",
        "Modify the map lighting.",
        rendermax));
    return CR_OK;
}

struct dwarmode_render_hook : viewscreen_dwarfmodest{
    typedef df::viewscreen_dwarfmodest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void,render,())
    {
        CoreSuspender suspend;
        engine->preRender();
        INTERPOSE_NEXT(render)();
        engine->calculate();
        engine->updateWindow();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(dwarmode_render_hook, render);

struct dungeon_render_hook : viewscreen_dungeonmodest{
    typedef df::viewscreen_dungeonmodest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void,render,())
    {
        CoreSuspender suspend;
        engine->preRender();
        INTERPOSE_NEXT(render)();
        engine->calculate();
        engine->updateWindow();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(dungeon_render_hook, render);

void removeOld()
{
    CoreSuspender lock;
    if(engine)
    {
        INTERPOSE_HOOK(dwarmode_render_hook,render).apply(false);
        INTERPOSE_HOOK(dungeon_render_hook,render).apply(false);
        delete engine;
        engine=0;
    }
    if(current_mode!=MODE_DEFAULT)
        delete enabler->renderer;

    current_mode=MODE_DEFAULT;
}
void installNew(df::renderer* r,RENDERER_MODE newMode)
{
    enabler->renderer=r;
    current_mode=newMode;
}
static void lockGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    r->dataMutex.lock();
}
static void unlockGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    r->dataMutex.unlock();
}
static void resetGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    for(size_t i=0;i<r->foreMult.size();i++)
    {
        r->foreMult[i]=rgbf(1,1,1);
        r->foreOffset[i]=rgbf(0,0,0);
        r->backMult[i]=rgbf(1,1,1);
        r->backOffset[i]=rgbf(0,0,0);
    }
}
static int getGridsSize(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return -1;
    lua_pushnumber(L,gps->dimx);
    lua_pushnumber(L,gps->dimy);
    return 2;
}
static int getCell(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return 0;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    int x=luaL_checknumber(L,1);
    int y=luaL_checknumber(L,2);
    int id=r->xyToTile(x,y);
    rgbf fo=r->foreOffset[id];
    rgbf fm=r->foreMult[id];
    rgbf bo=r->backOffset[id];
    rgbf bm=r->backMult[id];
    lua_newtable(L);

    lua_newtable(L);
    lua_pushnumber(L,fo.r);
    lua_setfield(L,-2,"r");
    lua_pushnumber(L,fo.g);
    lua_setfield(L,-2,"g");
    lua_pushnumber(L,fo.b);
    lua_setfield(L,-2,"b");
    lua_setfield(L,-2,"fo");

    lua_newtable(L);
    lua_pushnumber(L,fm.r);
    lua_setfield(L,-2,"r");
    lua_pushnumber(L,fm.g);
    lua_setfield(L,-2,"g");
    lua_pushnumber(L,fm.b);
    lua_setfield(L,-2,"b");
    lua_setfield(L,-2,"fm");

    lua_newtable(L);
    lua_pushnumber(L,bo.r);
    lua_setfield(L,-2,"r");
    lua_pushnumber(L,bo.g);
    lua_setfield(L,-2,"g");
    lua_pushnumber(L,bo.b);
    lua_setfield(L,-2,"b");
    lua_setfield(L,-2,"bo");

    lua_newtable(L);
    lua_pushnumber(L,bm.r);
    lua_setfield(L,-2,"r");
    lua_pushnumber(L,bm.g);
    lua_setfield(L,-2,"g");
    lua_pushnumber(L,bm.b);
    lua_setfield(L,-2,"b");
    lua_setfield(L,-2,"bm");
    return 1;
}
static int setCell(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return 0;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    int x=luaL_checknumber(L,1);
    int y=luaL_checknumber(L,2);

    rgbf fo;
    lua_getfield(L,3,"fo");
    lua_getfield(L,-1,"r");
    fo.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    fo.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    fo.b=lua_tonumber(L,-1);lua_pop(L,1);
    rgbf fm;
    lua_getfield(L,3,"fm");
    lua_getfield(L,-1,"r");
    fm.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    fm.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    fm.b=lua_tonumber(L,-1);lua_pop(L,1);

    rgbf bo;
    lua_getfield(L,3,"bo");
    lua_getfield(L,-1,"r");
    bo.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    bo.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    bo.b=lua_tonumber(L,-1);lua_pop(L,1);

    rgbf bm;
    lua_getfield(L,3,"bm");
    lua_getfield(L,-1,"r");
    bm.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    bm.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    bm.b=lua_tonumber(L,-1);lua_pop(L,1);
    int id=r->xyToTile(x,y);
    r->foreMult[id]=fm;
    r->foreOffset[id]=fo;
    r->backMult[id]=bm;
    r->backOffset[id]=bo;
    return 0;
}
static int invalidate(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return 0;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(enabler->renderer);
    if(lua_gettop(L)==0)
    {
        r->invalidate();
    }
    else
    {
        int x,y,w,h;
        lua_getfield(L,1,"x");
        x=lua_tonumber(L,-1);lua_pop(L,1);
        lua_getfield(L,1,"y");
        y=lua_tonumber(L,-1);lua_pop(L,1);
        lua_getfield(L,1,"w");
        w=lua_tonumber(L,-1);lua_pop(L,1);
        lua_getfield(L,1,"h");
        h=lua_tonumber(L,-1);lua_pop(L,1);
        r->invalidateRect(x,y,w,h);
    }
    return 0;
}
bool isEnabled()
{
    return current_mode==MODE_LUA;
}
DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(isEnabled),
    DFHACK_LUA_FUNCTION(lockGrids),
    DFHACK_LUA_FUNCTION(unlockGrids),
    DFHACK_LUA_FUNCTION(resetGrids),
    DFHACK_LUA_END
};
DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getCell),
    DFHACK_LUA_COMMAND(setCell),
    DFHACK_LUA_COMMAND(getGridsSize),
    DFHACK_LUA_COMMAND(invalidate),
    DFHACK_LUA_END
};

static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(dwarmode_render_hook,render).apply(enable);
    INTERPOSE_HOOK(dungeon_render_hook,render).apply(enable);
    if(enable && engine)
    {
        engine->loadSettings();
    }
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if(current_mode!=MODE_LIGHT)
        return CR_OK;
    switch(event)
    {
    case SC_VIEWSCREEN_CHANGED:
        {
            CoreSuspender suspender;
            if(current_mode==MODE_LIGHT)
            {
                engine->clear();
            }
        }
        break;
    case SC_WORLD_LOADED:
        enable_hooks(true);
        break;
    case SC_WORLD_UNLOADED:
        enable_hooks(false);
        break;
    default:
        break;
    }
    return CR_OK;
}


static command_result rendermax(color_ostream &out, vector <string> & parameters)
{
    if(parameters.size()==0)
        return CR_WRONG_USAGE;
    if(!enabler->renderer->uses_opengl())
    {
        out.printerr("Sorry, this plugin needs open GL-enabled printmode. Try STANDARD or other non-2D.\n");
        return CR_FAILURE;
    }
    string cmd=parameters[0];
    if(cmd=="trippy")
    {
        removeOld();
        installNew(new renderer_trippy(enabler->renderer),MODE_TRIPPY);
        return CR_OK;
    }
    else if(cmd=="truecolor")
    {
        if(current_mode!=MODE_TRUECOLOR)
        {
            removeOld();
            installNew(new renderer_test(enabler->renderer),MODE_TRUECOLOR);
        }
        if(current_mode==MODE_TRUECOLOR && parameters.size()==2)
        {
            rgbf red(1,0,0),green(0,1,0),blue(0,0,1),white(1,1,1);
            rgbf cur=white;
            rgbf dim(0.2f,0.2f,0.2f);
            string col=parameters[1];
            if(col=="red")
                cur=red;
            else if(col=="green")
                cur=green;
            else if(col=="blue")
                cur=blue;

            renderer_test* r=reinterpret_cast<renderer_test*>(enabler->renderer);
            std::lock_guard<std::mutex> guard{r->dataMutex};
            int h=gps->dimy;
            int w=gps->dimx;
            int cx=w/2;
            int cy=h/2;
            int rad=cx;
            if(rad>cy)rad=cy;
            rad/=2;
            int radsq=rad*rad;
            for(size_t i=0;i<r->lightGrid.size();i++)
            {
                r->lightGrid[i]=dim;
            }
            for(int i=-rad;i<rad;i++)
            for(int j=-rad;j<rad;j++)
            {
                if((i*i+j*j)<radsq)
                {
                    float val=(radsq-i*i-j*j)/(float)radsq;
                    r->lightGrid[(cx+i)*h+(cy+j)]=dim+cur*val;
                }
            }
            return CR_OK;
        }
    }
    else if(cmd=="lua")
    {
        removeOld();
        installNew(new renderer_lua(enabler->renderer),MODE_LUA);
        lockGrids();
        resetGrids();
        unlockGrids();
        return CR_OK;
    }
    else if(cmd=="light")
    {
        if(current_mode!=MODE_LIGHT)
        {
            removeOld();
            renderer_light *myRender=new renderer_light(enabler->renderer);
            installNew(myRender,MODE_LIGHT);
            engine=new lightingEngineViewscreen(myRender);

            if (Core::getInstance().isWorldLoaded())
                plugin_onstatechange(out, SC_WORLD_LOADED);
        }
        else if(current_mode==MODE_LIGHT && parameters.size()>1)
        {
            if(parameters[1]=="reload")
            {
                enable_hooks(true);
            }
            else if(parameters[1]=="sun" && parameters.size()==3)
            {
                if(parameters[2]=="cycle")
                {
                    engine->setHour(-1);
                }
                else
                {
                    std::stringstream ss;
                    ss<<parameters[2];
                    float h;
                    ss>>h;
                    engine->setHour(h);
                }
            }
            else if(parameters[1]=="occlusionON")
            {
                engine->debug(true);
            }else if(parameters[1]=="occlusionOFF")
            {
                engine->debug(false);
            }
        }
        else
            out.printerr("Light mode already enabled");

        return CR_OK;
    }
    else if(cmd=="disable")
    {
        if(current_mode==MODE_DEFAULT)
            out.print("%s\n","Not installed, doing nothing.");
        else
            removeOld();
        CoreSuspender guard;
        gps->force_full_display_count++;
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_shutdown(color_ostream &)
{
    removeOld();
    return CR_OK;
}
