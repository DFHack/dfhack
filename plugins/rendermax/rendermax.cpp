#include <vector>
#include <string>

#include <LuaTools.h>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include <VTableInterpose.h>
#include "df/renderer.h"
#include "df/enabler.h"

#include "renderer_opengl.hpp"
#include "renderer_light.hpp"

using namespace DFHack;
using std::vector;
using std::string;
enum RENDERER_MODE
{
    MODE_DEFAULT,MODE_TRIPPY,MODE_TRUECOLOR,MODE_LUA,MODE_LIGHT,MODE_LIGHT_OFF
};
RENDERER_MODE current_mode=MODE_DEFAULT;
lightingEngine *engine=NULL;

static command_result rendermax(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("rendermax");


DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rendermax", "switch rendering engine.", rendermax, false,
        "  rendermax trippy\n"
        "  rendermax truecolor red|green|blue|white\n"
        "  rendermax lua\n"
        "  rendermax light\n"
        "  rendermax disable\n"
        ));
    return CR_OK;
}
void removeOld()
{
    if(current_mode!=MODE_DEFAULT)
        delete df::global::enabler->renderer;
    current_mode=MODE_DEFAULT;
}
void installNew(df::renderer* r,RENDERER_MODE newMode)
{
    df::global::enabler->renderer=r;
    current_mode=newMode;
}
static void lockGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    r->dataMutex.lock();
}
static void unlockGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    r->dataMutex.unlock();
}
static void resetGrids()
{
    if(current_mode!=MODE_LUA)
        return ;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    for(size_t i=0;i<r->foreMult.size();i++)
    {
        r->foreMult[i]=lightCell(1,1,1);
        r->foreOffset[i]=lightCell(0,0,0);
        r->backMult[i]=lightCell(1,1,1);
        r->backOffset[i]=lightCell(0,0,0);
    }
}
static int getGridsSize(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return -1;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    lua_pushnumber(L,df::global::gps->dimx);
    lua_pushnumber(L,df::global::gps->dimy);
    return 2;
}
static int getCell(lua_State* L)
{
    if(current_mode!=MODE_LUA)
        return 0;
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    int x=luaL_checknumber(L,1);
    int y=luaL_checknumber(L,2);
    int id=r->xyToTile(x,y);
    lightCell fo=r->foreOffset[id];
    lightCell fm=r->foreMult[id];
    lightCell bo=r->backOffset[id];
    lightCell bm=r->backMult[id];
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
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
    int x=luaL_checknumber(L,1);
    int y=luaL_checknumber(L,2);

    lightCell fo;
    lua_getfield(L,3,"fo");
    lua_getfield(L,-1,"r");
    fo.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    fo.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    fo.b=lua_tonumber(L,-1);lua_pop(L,1);
    lightCell fm;
    lua_getfield(L,3,"fm");
    lua_getfield(L,-1,"r");
    fm.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    fm.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    fm.b=lua_tonumber(L,-1);lua_pop(L,1);

    lightCell bo;
    lua_getfield(L,3,"bo");
    lua_getfield(L,-1,"r");
    bo.r=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"g");
    bo.g=lua_tonumber(L,-1);lua_pop(L,1);
    lua_getfield(L,-1,"b");
    bo.b=lua_tonumber(L,-1);lua_pop(L,1);

    lightCell bm;
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
    renderer_lua* r=reinterpret_cast<renderer_lua*>(df::global::enabler->renderer);
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
static command_result rendermax(color_ostream &out, vector <string> & parameters)
{
    if(parameters.size()==0)
        return CR_WRONG_USAGE;
    if(!df::global::enabler->renderer->uses_opengl())
    {
        out.printerr("Sorry, this plugin needs open gl enabled printmode. Try STANDARD or other non-2d");
        return CR_FAILURE;
    }
    string cmd=parameters[0];
    if(cmd=="trippy")
    {
        removeOld();
        installNew(new renderer_trippy(df::global::enabler->renderer),MODE_TRIPPY);
        return CR_OK;
    }
    else if(cmd=="truecolor")
    {
        if(current_mode!=MODE_TRUECOLOR)
        {
            removeOld();
            installNew(new renderer_test(df::global::enabler->renderer),MODE_TRUECOLOR);
        }
        if(current_mode==MODE_TRUECOLOR && parameters.size()==2)
        {
            lightCell red(1,0,0),green(0,1,0),blue(0,0,1),white(1,1,1);
            lightCell cur=white;
            lightCell dim(0.2f,0.2f,0.2f);
            string col=parameters[1];
            if(col=="red")
                cur=red;
            else if(col=="green")
                cur=green;
            else if(col=="blue")
                cur=blue;

            renderer_test* r=reinterpret_cast<renderer_test*>(df::global::enabler->renderer);
            tthread::lock_guard<tthread::fast_mutex> guard(r->dataMutex);
            int h=df::global::gps->dimy;
            int w=df::global::gps->dimx;
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
        installNew(new renderer_lua(df::global::enabler->renderer),MODE_LUA);
        lockGrids();
        resetGrids();
        unlockGrids();
        return CR_OK;
    }
    else if(cmd=="light")
    {
        removeOld();
        renderer_light *myRender=new renderer_light(df::global::enabler->renderer);
        installNew(myRender,MODE_LIGHT);
        engine=new lightingEngineViewscreen(myRender);
        engine->calculate();
        engine->updateWindow();
        return CR_OK;
    }
    else if(cmd=="disable")
    {
        if(current_mode==MODE_DEFAULT)
            out.print("%s\n","Not installed, doing nothing.");
        else if(current_mode==MODE_LIGHT)
            current_mode=MODE_LIGHT_OFF;
        else
            removeOld();
        
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}
DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    if(engine)
    {
        if(current_mode==MODE_LIGHT_OFF)
        {
            delete engine;
            engine=0;
            removeOld();
        }
        else
        {
            engine->calculate();
            engine->updateWindow();
        }
    }
    
    return CR_OK;
}
DFhackCExport command_result plugin_shutdown(color_ostream &)
{
    removeOld();
    return CR_OK;
}