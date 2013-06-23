#include "renderer_light.hpp"

#include <functional>

#include "Types.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Maps.h"

#include "df/graphic.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/flow_info.h"
#include "df/world.h"
#include "df/building.h"


using df::global::gps;
using namespace DFHack;
using df::coord2d;

const float levelDim=0.2f;

rect2d getMapViewport()
{
    const int AREA_MAP_WIDTH = 23;
    const int MENU_WIDTH = 30;
    if(!gps || !df::viewscreen_dwarfmodest::_identity.is_instance(DFHack::Gui::getCurViewscreen()))
        return mkrect_wh(0,0,0,0);
    int w=gps->dimx;
    int h=gps->dimy;
    int view_height=h-2;
    int area_x2 = w-AREA_MAP_WIDTH-2;
    int menu_x2=w-MENU_WIDTH-2;
    int menu_x1=area_x2-MENU_WIDTH-1;
    int view_rb=w-1;
    
    int area_pos=*df::global::ui_area_map_width;
    int menu_pos=*df::global::ui_menu_width;
    if(area_pos<3)
    {
        view_rb=area_x2;
    }
    if (menu_pos<area_pos || df::global::ui->main.mode!=0)
    {
        if (menu_pos >= area_pos) 
            menu_pos = area_pos-1;
        int menu_x = menu_x2;
        if(menu_pos < 2) menu_x = menu_x1;
        view_rb = menu_x;
    }
    return mkrect_wh(1,1,view_rb,view_height+1);
}
lightingEngineViewscreen::lightingEngineViewscreen(renderer_light* target):lightingEngine(target)
{
    reinit();
}

void lightingEngineViewscreen::reinit()
{
    if(!gps)
        return;
    w=gps->dimx;
    h=gps->dimy;
    size_t size=w*h;
    lightMap.resize(size,lightCell(1,1,1));
    ocupancy.resize(size);
    lights.resize(size);
}
void plotCircle(int xm, int ym, int r,std::function<void(int,int)> setPixel)
{
    int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */ 
    do {
        setPixel(xm-x, ym+y); /*   I. Quadrant */
        setPixel(xm-y, ym-x); /*  II. Quadrant */
        setPixel(xm+x, ym-y); /* III. Quadrant */
        setPixel(xm+y, ym+x); /*  IV. Quadrant */
        r = err;
        if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
        if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
    } while (x < 0);
}
void plotLine(int x0, int y0, int x1, int y1,std::function<bool(int,int,int,int)> setPixel)
{
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = dx+dy, e2; /* error value e_xy */
    int rdx=0;
    int rdy=0;
    for(;;){  /* loop */
        if(!setPixel(rdx,rdy,x0,y0))
            return;
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        rdx=rdy=0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx=sx;} /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy=sy;} /* e_xy+e_y < 0 */
    }
}
lightCell blend(lightCell a,lightCell b)
{
    return lightCell(std::max(a.r,b.r),std::max(a.g,b.g),std::max(a.b,b.b));
}
bool lightingEngineViewscreen::lightUpCell(lightCell& power,int dx,int dy,int tx,int ty)
{
    
    if(tx>=mapPort.first.x && ty>=mapPort.first.y && tx<=mapPort.second.x && ty<=mapPort.second.y)
    {
        size_t tile=getIndex(tx,ty);
        float dsq=dx*dx+dy*dy;
        float dt=sqrt(dsq);
        lightCell& v=ocupancy[tile];
        lightSource& ls=lights[tile];
        bool wallhack=false;
        if(v.r+v.g+v.b==0)
            wallhack=true;
        
        if (dsq>0 && !wallhack)
        {
            power.r=power.r*(pow(v.r,dt));
            power.g=power.g*(pow(v.g,dt));
            power.b=power.b*(pow(v.b,dt));
        }
        if(ls.radius>0 && dsq>0)
        {
            if(power<ls.power)
                return false;
        }
        //float dt=sqrt(dsq);
        lightCell oldCol=lightMap[tile];
        lightCell ncol=blend(power,oldCol);
        lightMap[tile]=ncol;
        
        if(wallhack)
            return false;
        float pwsq=power.r*power.r+power.g*power.g+power.b*power.b;
        return pwsq>levelDim*levelDim;
    }
    else
        return false;
}
void lightingEngineViewscreen::doRay(lightCell power,int cx,int cy,int tx,int ty)
{
    using namespace std::placeholders;
    lightCell curPower=power;
    plotLine(cx,cy,tx,ty,std::bind(&lightingEngineViewscreen::lightUpCell,this,std::ref(curPower),_1,_2,_3,_4));
}
void lightingEngineViewscreen::doFovs()
{
    mapPort=getMapViewport();
    using namespace std::placeholders;

    for(int i=mapPort.first.x;i<mapPort.second.x;i++)
        for(int j=mapPort.first.y;j<mapPort.second.y;j++)
        {
            lightSource& csource=lights[getIndex(i,j)];
            if(csource.radius>0)
            {
                lightCell power=csource.power;
                int radius =csource.radius;
                if(csource.flicker)
                {
                    float flicker=(rand()/(float)RAND_MAX)/2.0f+0.5f;
                    radius*=flicker;
                    power=power*flicker;
                }
                plotCircle(i,j,radius,
                std::bind(&lightingEngineViewscreen::doRay,this,power,i,j,_1,_2));
            }
        }
}
void lightingEngineViewscreen::calculate()
{
    rect2d vp=getMapViewport();
    const lightCell dim(levelDim,levelDim,levelDim);
    lightMap.assign(lightMap.size(),lightCell(1,1,1));
    lights.assign(lights.size(),lightSource());
    for(int i=vp.first.x;i<vp.second.x;i++)
    for(int j=vp.first.y;j<vp.second.y;j++)
    {
        lightMap[getIndex(i,j)]=dim;
    }
    doOcupancyAndLights();
    doFovs();
    //for each lightsource in viewscreen+x do light
}
void lightingEngineViewscreen::updateWindow()
{
    tthread::lock_guard<tthread::fast_mutex> guard(myRenderer->dataMutex);
    if(lightMap.size()!=myRenderer->lightGrid.size())
    {
        reinit();
        myRenderer->invalidate();
        return;
    }
    std::swap(lightMap,myRenderer->lightGrid);
    rect2d vp=getMapViewport();
    
    //myRenderer->invalidateRect(vp.first.x,vp.first.y,vp.second.x-vp.first.x,vp.second.y-vp.first.y);
    myRenderer->invalidate();
    //std::copy(lightMap.begin(),lightMap.end(),myRenderer->lightGrid.begin());
}
void lightSource::combine(const lightSource& other)
{
    power=blend(power,other.power);
    radius=std::max(other.radius,radius);//hack... but who cares
}
bool lightingEngineViewscreen::addLight(int tileId,const lightSource& light)
{
    bool wasLight=lights[tileId].radius>0;
    lights[tileId].combine(light);
    if(light.flicker)
        lights[tileId].flicker=true;
    return wasLight;
}
static size_t max_list_size = 100000; // Avoid iterating over huge lists
void lightingEngineViewscreen::doOcupancyAndLights()
{
    lightSource sun(lightCell(1,1,1),15);
    lightSource lava(lightCell(0.8f,0.2f,0.2f),5);
    lightSource candle(lightCell(0.96f,0.84f,0.03f),5);
    candle.flicker=true;
    lightSource torch(lightCell(0.96f,0.5f,0.1f),8);
    rect2d vp=getMapViewport();
    
    int window_x=*df::global::window_x;
    int window_y=*df::global::window_y;
    int window_z=*df::global::window_z;
    int vpW=vp.second.x-vp.first.x;
    int vpH=vp.second.y-vp.first.y;
    for(int x=window_x;x<window_x+vpW;x++)
    for(int y=window_y;y<window_y+vpH;y++)
    {
        int wx=x-window_x+vp.first.x;
        int wy=y-window_y+vp.first.y;
        int tile=getIndex(wx,wy);
        lightCell& curCell=ocupancy[tile];
        curCell=lightCell(0.85f,0.85f,0.85f);
        df::tiletype* type = Maps::getTileType(x,y,window_z);
        if(!type)
        {
            //unallocated, do sky
                addLight(tile,sun);
            continue;
        }
        df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,*type);
        df::tile_designation* d=Maps::getTileDesignation(x,y,window_z);
        df::tile_designation* d2=Maps::getTileDesignation(x,y,window_z-1);
        df::tile_occupancy* o=Maps::getTileOccupancy(x,y,window_z);
        if(!o || !d )
            continue;
        if(shape==df::tiletype_shape::BROOK_BED || shape==df::tiletype_shape::WALL || shape==df::tiletype_shape::TREE || d->bits.hidden )
        {
            curCell=lightCell(0,0,0);
        }
        else if(o->bits.building)
        {
            // Fixme: don't iterate the list every frame
            size_t count = df::global::world->buildings.all.size();
            if (count <= max_list_size)
            {
                for(size_t i = 0; i < count; i++)
                {
                    df::building *bld = df::global::world->buildings.all[i];

                    if (window_z == bld->z && 
                        x >= bld->x1 && x <= bld->x2 &&
                        y >= bld->y1 && y <= bld->y2)
                    {
                        df::building_type type = bld->getType();

                        if (type == df::enums::building_type::WindowGlass)
                        {
                            if(bld->mat_type == 3)//green glass
                            {
                                curCell*=lightCell(0.1f,0.9f,0.5f);
                            }
                            else if(bld->mat_type == 4)//clear glass
                            {
                                curCell*=lightCell(0.5f,0.95f,0.9f);
                            }
                            else if(bld->mat_type == 5)//crystal glass
                            {
                                curCell*=lightCell(0.75f,0.95f,0.95f);
                            }
                        }
                        if (type == df::enums::building_type::Table)
                        {
                            addLight(tile,candle);
                        }
                        if (type==df::enums::building_type::Statue)
                        {
                            addLight(tile,torch);
                        }
                        if (type==df::enums::building_type::WindowGem)
                        {
                            DFHack::MaterialInfo mat(bld->mat_index,bld->mat_type);
                            if(mat.isInorganic())
                            {
                                int color=mat.inorganic->material.basic_color[0];
                                curCell*=lightCell(df::global::enabler->ccolor[color][0]/255.0f,
                                    df::global::enabler->ccolor[color][1]/255.0f,
                                    df::global::enabler->ccolor[color][2]/255.0f);
                            }
                        }
                    }
                }
            }
        }
        else if(!d->bits.liquid_type && d->bits.flow_size>3 )
        {
            curCell*=lightCell(0.7f,0.7f,0.8f);
        }

        //lights
        if((d->bits.liquid_type && d->bits.flow_size>0)|| 
            (
            (shape==df::tiletype_shape::EMPTY || shape==df::tiletype_shape::RAMP_TOP || shape==df::tiletype_shape::STAIR_DOWN || shape==df::tiletype_shape::STAIR_UPDOWN )
            && d2 && d2->bits.liquid_type && d2->bits.flow_size>0)
            )
        {
            
            addLight(tile,lava);
        }
        if(d->bits.outside)
        {
            
            addLight(tile,sun);
        }
        
    }

    for(int blockx=window_x/16;blockx<(window_x+vpW)/16;blockx++)
    for(int blocky=window_y/16;blocky<(window_x+vpW)/16;blocky++)
    {
        df::map_block* block=Maps::getBlock(blockx,blocky,window_z);
        
        if(!block)
            continue;
        for(int i=0;i<block->flows.size();i++)
        {
            df::flow_info* f=block->flows[i];
            if(f && f->density>0 && f->type==df::flow_type::Dragonfire || f->type==df::flow_type::Fire)
            {
                df::coord2d pos=f->pos;
                int wx=pos.x-window_x+vp.first.x;
                int wy=pos.y-window_y+vp.first.y;
                int tile=getIndex(wx,wy);
                if(wx>=vp.first.x && wy>=vp.first.y && wx<=vp.second.x && wy<=vp.second.y)
                {
                    lightCell fireColor;
                    if(f->density>60)
                    {
                        fireColor=lightCell(0.98f,0.91f,0.30f);
                    }
                    else if(f->density>30)
                    {
                        fireColor=lightCell(0.93f,0.16f,0.16f);
                    }
                    else
                    {
                        fireColor=lightCell(0.64f,0.0f,0.0f);
                    }
                    lightSource fire(fireColor,f->density/5);
                    addLight(tile,fire);
                }
            }
        }
    }
    if(df::global::cursor->x>-30000)
    {
        lightSource cursor(lightCell(0.96f,0.84f,0.03f),11);
        cursor.flicker=true;
        int wx=df::global::cursor->x-window_x+vp.first.x;
        int wy=df::global::cursor->y-window_y+vp.first.y;
        int tile=getIndex(wx,wy);
        addLight(tile,cursor);
    }
}