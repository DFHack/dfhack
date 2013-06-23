#include "renderer_light.hpp"

#include <functional>

#include "Types.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Maps.h"

#include "df/graphic.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/flow_info.h"

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
        lightCell& v=ocupancy[tile];
        bool wallhack=false;
        bool outsidehack=false;
        if(v.r+v.g+v.b==0)
            wallhack=true;
        if(v.r<0)
            outsidehack=true;
        if (dsq>0 && !wallhack && !outsidehack)
        {
            power.r=power.r*(pow(v.r,dsq));
            power.g=power.g*(pow(v.g,dsq));
            power.b=power.b*(pow(v.b,dsq));
        }
        //float dt=sqrt(dsq);
        lightCell oldCol=lightMap[tile];
        lightCell ncol=blend(power,oldCol);
        lightMap[tile]=ncol;
        
        if(wallhack)
            return false;
        if(dsq>0 && outsidehack)
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
    for(size_t i=0;i<lights.size();i++)
    {
        lightSource& csource=lights[i];
        plotCircle(csource.pos.x,csource.pos.y,csource.radius,std::bind(&lightingEngineViewscreen::doRay,this,csource.power,csource.pos.x,csource.pos.y,_1,_2));
    }
}
void lightingEngineViewscreen::calculate()
{
    rect2d vp=getMapViewport();
    const lightCell dim(levelDim,levelDim,levelDim);
    lightMap.assign(lightMap.size(),lightCell(1,1,1));
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
        return;
    }
    std::swap(lightMap,myRenderer->lightGrid);
    rect2d vp=getMapViewport();
    
    //myRenderer->invalidateRect(vp.first.x,vp.first.y,vp.second.x-vp.first.x,vp.second.y-vp.first.y);
    myRenderer->invalidate();
    //std::copy(lightMap.begin(),lightMap.end(),myRenderer->lightGrid.begin());
}
void lightingEngineViewscreen::doOcupancyAndLights()
{
    lights.clear();
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
        lightCell& curCell=ocupancy[getIndex(wx,wy)];
        curCell=lightCell(0.8f,0.8f,0.8f);
        df::tiletype* type = Maps::getTileType(x,y,window_z);
        if(!type)
            continue;
        df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,*type);
        df::tile_designation* d=Maps::getTileDesignation(x,y,window_z);
        df::tile_designation* d2=Maps::getTileDesignation(x,y,window_z-1);
        df::tile_occupancy* o=Maps::getTileOccupancy(x,y,window_z);
        if(!o || !d )
            continue;
        
        if(shape==df::tiletype_shape::BROOK_BED || shape==df::tiletype_shape::WALL || shape==df::tiletype_shape::TREE || o->bits.building)
        {
            curCell=lightCell(0,0,0);
        }
        else if(!d->bits.liquid_type && d->bits.flow_size>3 )
        {
            curCell=lightCell(0.5f,0.5f,0.6f);
        }
        //todo constructions

        //lights
        if((d->bits.liquid_type && d->bits.flow_size>0)|| (d2 && d2->bits.liquid_type && d2->bits.flow_size>0))
        {
            lightSource lava={lightCell(0.8f,0.2f,0.2f),5,coord2d(wx,wy)};
            lights.push_back(lava);
        }
        if(d->bits.outside)
        {
            lightSource sun={lightCell(1,1,1),25,coord2d(wx,wy)};
            lights.push_back(sun);
            curCell=lightCell(-1,-1,-1);//Marking as outside so no calculation is done on it
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
                    lightSource fire={fireColor,f->density/5,coord2d(wx,wy)};
                    lights.push_back(fire);
                }
            }
        }
    }
}