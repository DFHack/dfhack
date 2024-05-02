#include "renderer_light.hpp"

#include <functional>
#include <math.h>
#include <string>
#include <vector>

#include "LuaTools.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Units.h"

#include "df/block_square_event_material_spatterst.h"
#include "df/building.h"
#include "df/building_doorst.h"
#include "df/building_floodgatest.h"
#include "df/flow_info.h"
#include "df/graphic.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/unit.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

using df::global::gps;
using namespace DFHack;
using df::coord2d;

const float RootTwo = 1.4142135623730950488016887242097f;


bool isInRect(const coord2d& pos,const rect2d& rect)
{
    if(pos.x>=rect.first.x && pos.y>=rect.first.y && pos.x<rect.second.x && pos.y<rect.second.y)
        return true;
    return false;
}

lightingEngineViewscreen::~lightingEngineViewscreen()
{
    threading.shutdown();
}

lightSource::lightSource(rgbf power,int radius):power(power),flicker(false)
{
    if(radius >= 0)
        this->radius = radius;
    else
    {
        float levelDim = 0.2f;//TODO this is not correct if you change config
        float totalPower = power.r;
        if(totalPower < power.g)totalPower = power.g;
        if(totalPower < power.b)totalPower = power.b;
        if(totalPower > 0 && levelDim > 0)
            this->radius = (int)((log(levelDim/totalPower)/log(0.85f))) + 1;
        else
            this->radius = 0;
    }
}

rect2d getMapViewport()
{
    const int AREA_MAP_WIDTH = 23;
    const int MENU_WIDTH = 30;
    if(!gps || !df::viewscreen_dwarfmodest::_identity.is_instance(DFHack::Gui::getCurViewscreen()))
    {
        if(gps && df::viewscreen_dungeonmodest::_identity.is_instance(DFHack::Gui::getCurViewscreen()))
        {
            return mkrect_wh(0,0,gps->dimx,gps->dimy);
        }
        else
            return mkrect_wh(0,0,0,0);

    }
    int w=gps->dimx;
    int h=gps->dimy;
    int view_height=h-2;
    int area_x2 = w-AREA_MAP_WIDTH-2;
    int menu_x2=w-MENU_WIDTH-2;
    int menu_x1=area_x2-MENU_WIDTH-1;
    int view_rb=w-1;

    int area_pos=(*df::global::ui_menu_width)[1];
    int menu_pos=(*df::global::ui_menu_width)[0];
    if(area_pos<3)
    {
        view_rb=area_x2;
    }
    if (menu_pos<area_pos || df::global::plotinfo->main.mode!=0)
    {
        if (menu_pos >= area_pos)
            menu_pos = area_pos-1;
        int menu_x = menu_x2;
        if(menu_pos < 2) menu_x = menu_x1;
        view_rb = menu_x;
    }
    return mkrect_wh(1,1,view_rb,view_height+1);
}
lightingEngineViewscreen::lightingEngineViewscreen(renderer_light* target):lightingEngine(target),threading(this),doDebug(false)
{
    reinit();
    defaultSettings();
    int numThreads=std::thread::hardware_concurrency();
    if(numThreads==0)numThreads=1;
    threading.start(numThreads);
}

void lightingEngineViewscreen::reinit()
{
    if(!gps)
        return;
    w=gps->dimx;
    h=gps->dimy;
    size_t size=w*h;
    lightMap.resize(size,rgbf(1,1,1));
    ocupancy.resize(size);
    lights.resize(size);
}

void plotCircle(int xm, int ym, int r,const std::function<void(int,int)>& setPixel)
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
void plotSquare(int xm, int ym, int r,const std::function<void(int,int)>& setPixel)
{
    for(int x = 0; x <= r; x++)
    {
        setPixel(xm+r, ym+x); /*   I.1 Quadrant */
        setPixel(xm+x, ym+r); /*   I.2 Quadrant */
        setPixel(xm+r, ym-x); /*   II.1 Quadrant */
        setPixel(xm+x, ym-r); /*   II.2 Quadrant */
        setPixel(xm-r, ym-x); /*   III.1 Quadrant */
        setPixel(xm-x, ym-r); /*   III.2 Quadrant */
        setPixel(xm-r, ym+x); /*   IV.1 Quadrant */
        setPixel(xm-x, ym+r); /*   IV.2 Quadrant */
    }
}
void plotLine(int x0, int y0, int x1, int y1,rgbf power,const std::function<rgbf(rgbf,int,int,int,int)>& setPixel)
{
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2; /* error value e_xy */
    int rdx=0;
    int rdy=0;
    for(;;){  /* loop */
        if(rdx!=0 || rdy!=0) //dirty hack to skip occlusion on the first tile.
        {
            power=setPixel(power,rdx,rdy,x0,y0);
            if(power.dot(power)<0.00001f)
                return ;
        }
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        rdx=rdy=0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx=sx;} /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy=sy;} /* e_xy+e_y < 0 */
    }
    return ;
}
void plotLineDiffuse(int x0, int y0, int x1, int y1,rgbf power,int num_diffuse,const std::function<rgbf(rgbf,int,int,int,int)>& setPixel,bool skip_hack=false)
{

    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int dsq=dx*dx+dy*dy;
    int err = dx+dy, e2; /* error value e_xy */
    int rdx=0;
    int rdy=0;
    for(;;){  /* loop */
        if(rdx!=0 || rdy!=0 || skip_hack) //dirty hack to skip occlusion on the first tile.
        {
            power=setPixel(power,rdx,rdy,x0,y0);
            if(power.dot(power)<0.00001f)
                return ;
        }
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        rdx=rdy=0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx=sx;} /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy=sy;} /* e_xy+e_y < 0 */

        if(num_diffuse>0 && dsq/4<(x1-x0)*(x1-x0)+(y1-y0)*(y1-y0))//reached center?
        {
            const float betta=0.25;
            int nx=y1-y0; //right angle
            int ny=x1-x0;
            if((nx*nx+ny*ny)*betta*betta>2)
            {
                plotLineDiffuse(x0,y0,x0+nx*betta,y0+ny*betta,power,num_diffuse-1,setPixel,true);
                plotLineDiffuse(x0,y0,x0-nx*betta,y0-ny*betta,power,num_diffuse-1,setPixel,true);
            }
        }
    }
    return ;
}
void plotLineAA(int x0, int y0, int x1, int y1,rgbf power,const std::function<rgbf(rgbf,int,int,int,int)>& setPixelAA)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx-dy, e2, x2;                       /* error value e_xy */
    int ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);
    int rdx=0;
    int rdy=0;
    int lrdx,lrdy;
    rgbf sumPower;
    for ( ; ; ){                                         /* pixel loop */
        float strsum=0;
        float str=1-abs(err-dx+dy)/(float)ed;
        strsum=str;
        sumPower=setPixelAA(power*str,rdx,rdy,x0,y0);
        e2 = err; x2 = x0;
        lrdx=rdx;
        lrdy=rdy;
        rdx=rdy=0;
        if (2*e2 >= -dx) {                                    /* x step */
            if (x0 == x1) break;

            if (e2+dy < ed)
                {
                    str=1-(e2+dy)/(float)ed;
                    sumPower+=setPixelAA(power*str,lrdx,lrdy,x0,y0+sy);
                    strsum+=str;
                }
            err -= dy; x0 += sx; rdx=sx;
        }
        if (2*e2 <= dy) {                                     /* y step */
            if (y0 == y1) break;

            if (dx-e2 < ed)
                {
                    str=1-(dx-e2)/(float)ed;
                    sumPower+=setPixelAA(power*str,lrdx,lrdy,x2+sx,y0);
                    strsum+=str;
                }
            err += dx; y0 += sy; rdy=sy;
        }
        if(strsum<0.001f)
            return;
        sumPower=sumPower/strsum;
        if(sumPower.dot(sumPower)<0.00001f)
            return;
        power=sumPower;
    }
}
rgbf blendMax(const rgbf& a,const rgbf& b)
{
    return rgbf(std::max(a.r,b.r),std::max(a.g,b.g),std::max(a.b,b.b));
}
rgbf blend(const rgbf& a,const rgbf& b)
{
    return blendMax(a,b);
}
void lightingEngineViewscreen::clear()
{
    lightMap.assign(lightMap.size(),rgbf(1,1,1));
    std::lock_guard<std::mutex> guard{myRenderer->dataMutex};
    if(lightMap.size()==myRenderer->lightGrid.size())
    {
        std::swap(myRenderer->lightGrid,lightMap);
        myRenderer->invalidate();
    }
}
void lightingEngineViewscreen::calculate()
{
    if(lightMap.size()!=myRenderer->lightGrid.size())
    {
        reinit();
        myRenderer->invalidate();//needs a lock?
    }
    rect2d vp=getMapViewport();
    const rgbf dim(levelDim,levelDim,levelDim);
    lightMap.assign(lightMap.size(),rgbf(1,1,1));
    lights.assign(lights.size(),lightSource());
    for(int i=vp.first.x;i<vp.second.x;i++)
    for(int j=vp.first.y;j<vp.second.y;j++)
    {
        lightMap[getIndex(i,j)]=dim;
    }
    doOcupancyAndLights();
    threading.signalDoneOcclusion();
    threading.waitForWrites();
}
void lightingEngineViewscreen::updateWindow()
{
    std::lock_guard<std::mutex> guard{myRenderer->dataMutex};
    if(lightMap.size()!=myRenderer->lightGrid.size())
    {
        reinit();
        myRenderer->invalidate();
        return;
    }

    bool isAdventure=(*df::global::gametype==df::game_type::ADVENTURE_ARENA)||
        (*df::global::gametype==df::game_type::ADVENTURE_MAIN);
    if(isAdventure)
    {
        fixAdvMode(adv_mode);
    }

    if(doDebug)
        std::swap(ocupancy,myRenderer->lightGrid);
    else
        std::swap(lightMap,myRenderer->lightGrid);
    rect2d vp=getMapViewport();

    myRenderer->invalidateRect(vp.first.x,vp.first.y,vp.second.x-vp.first.x,vp.second.y-vp.first.y);
}
void lightingEngineViewscreen::preRender()
{

}
void lightingEngineViewscreen::fixAdvMode(int mode)
{

    MapExtras::MapCache mc;
    const rgbf dim(levelDim,levelDim,levelDim);
    rect2d vp=getMapViewport();
    int window_x=*df::global::window_x;
    int window_y=*df::global::window_y;
    int window_z=*df::global::window_z;
    //mode 0-> make dark non-visible parts
    if(mode==0)
    {
        for(int x=vp.first.x;x<vp.second.x;x++)
            for(int y=vp.first.y;y<vp.second.y;y++)
            {
                df::tile_designation d=mc.designationAt(DFCoord(window_x+x,window_y+y,window_z));
                if(d.bits.pile!=1)
                {
                    lightMap[getIndex(x,y)]=dim;
                }
            }
    }
    //mode 1-> make everything visible, let the lighting hide stuff
    else if(mode==1)
    {
        for(int x=vp.first.x;x<vp.second.x;x++)
            for(int y=vp.first.y;y<vp.second.y;y++)
            {
                df::tile_designation d=mc.designationAt(DFCoord(window_x+x,window_y+y,window_z));
                d.bits.dig=df::tile_dig_designation::Default;//TODO add union and flag there
                d.bits.hidden=0;
                d.bits.pile = 1;
                mc.setDesignationAt(DFCoord(window_x+x,window_y+y,window_z),d);
            }
    }
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
void lightingEngineViewscreen::addOclusion(int tileId,const rgbf& c,float thickness)
{
    if(thickness > 0.999 && thickness < 1.001)
        ocupancy[tileId]*=c;
    else
        ocupancy[tileId]*=(c.pow(thickness));
}
rgbf getStandartColor(int colorId)
{
    return rgbf(df::global::enabler->ccolor[colorId][0]/255.0f,
        df::global::enabler->ccolor[colorId][1]/255.0f,
        df::global::enabler->ccolor[colorId][2]/255.0f);
}
int getPlantNumber(const std::string& id)
{
    std::vector<df::plant_raw*>& vec=df::plant_raw::get_vector();
    for(size_t i=0;i<vec.size();i++)
    {
        if(vec[i]->id==id)
            return i;
    }
    return -1;
}
void addPlant(const std::string& id,std::map<int,lightSource>& map,const lightSource& v)
{
    int nId=getPlantNumber(id);
    if(nId>0)
    {
        map[nId]=v;
    }
}
matLightDef* lightingEngineViewscreen::getMaterialDef( int matType,int matIndex )
{
    auto it=matDefs.find(std::make_pair(matType,matIndex));
    if(it!=matDefs.end())
        return &it->second;
    else
        return NULL;
}
buildingLightDef* lightingEngineViewscreen::getBuildingDef( df::building* bld )
{
    auto it=buildingDefs.find(std::make_tuple((int)bld->getType(),(int)bld->getSubtype(),(int)bld->getCustomType()));
    if(it!=buildingDefs.end())
        return &it->second;
    else
        return NULL;
}
creatureLightDef* lightingEngineViewscreen::getCreatureDef(df::unit* u)
{
    auto it=creatureDefs.find(std::make_pair(int(u->race),int(u->caste)));
    if(it!=creatureDefs.end())
        return &it->second;
    else
    {
        auto it2=creatureDefs.find(std::make_pair(int(u->race),int(-1)));
        if(it2!=creatureDefs.end())
            return &it2->second;
        else
            return NULL;
    }
}
itemLightDef* lightingEngineViewscreen::getItemDef(df::item* it)
{
    auto iter=itemDefs.find(std::make_pair(int(it->getType()),int(it->getSubtype())));
    if(iter!=itemDefs.end())
        return &iter->second;
    else
    {
        auto iter2=itemDefs.find(std::make_pair(int(it->getType()),int(-1)));
        if(iter2!=itemDefs.end())
            return &iter2->second;
        else
            return NULL;
    }
}
void lightingEngineViewscreen::applyMaterial(int tileId,const matLightDef& mat,float size, float thickness)
{
    if(mat.isTransparent)
    {
       addOclusion(tileId,mat.transparency,thickness);
    }
    else
        ocupancy[tileId]=rgbf(0,0,0);
    if(mat.isEmiting)
        addLight(tileId,mat.makeSource(size));
}
bool lightingEngineViewscreen::applyMaterial(int tileId,int matType,int matIndex,float size,float thickness,const matLightDef* def)
{
    matLightDef* m=getMaterialDef(matType,matIndex);
    if(m)
    {
        applyMaterial(tileId,*m,size,thickness);
        return true;
    }
    else if(def)
    {
        applyMaterial(tileId,*def,size,thickness);
    }
    return false;
}
rgbf lightingEngineViewscreen::propogateSun(MapExtras::Block* b, int x,int y,const rgbf& in,bool lastLevel)
{
    //TODO unify under addLight/addOclusion
    const rgbf matStairCase(0.9f,0.9f,0.9f);
    rgbf ret=in;
    coord2d innerCoord(x,y);
    df::tiletype type = b->staticTiletypeAt(innerCoord);
    df::tile_designation d = b->DesignationAt(innerCoord);
    //df::tile_occupancy o = b->OccupancyAt(innerCoord);
    df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,type);
    df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
    DFHack::t_matpair mat=b->staticMaterialAt(innerCoord);
    df::tiletype_material tileMat= ENUM_ATTR(tiletype,material,type);

    matLightDef* lightDef;
    if(tileMat==df::tiletype_material::FROZEN_LIQUID)
    {
        df::tiletype typeIce = b->tiletypeAt(innerCoord);
        df::tiletype_shape shapeIce = ENUM_ATTR(tiletype,shape,typeIce);
        df::tiletype_shape_basic basicShapeIce = ENUM_ATTR(tiletype_shape,basic_shape,shapeIce);
        if(basicShapeIce==df::tiletype_shape_basic::Wall)
            ret*=matIce.transparency;
        else if(basicShapeIce==df::tiletype_shape_basic::Floor || basicShapeIce==df::tiletype_shape_basic::Ramp || shapeIce==df::tiletype_shape::STAIR_UP)
            if(!lastLevel)
                ret*=matIce.transparency.pow(1.0f/7.0f);
    }

    lightDef=getMaterialDef(mat.mat_type,mat.mat_index);

    if(!lightDef || !lightDef->isTransparent)
        lightDef=&matWall;
    if(basic_shape==df::tiletype_shape_basic::Wall)
    {
        ret*=lightDef->transparency;
    }
    else if(basic_shape==df::tiletype_shape_basic::Floor || basic_shape==df::tiletype_shape_basic::Ramp || shape==df::tiletype_shape::STAIR_UP)
    {

        if(!lastLevel)
            ret*=lightDef->transparency.pow(1.0f/7.0f);
    }
    else if(shape==df::tiletype_shape::STAIR_DOWN || shape==df::tiletype_shape::STAIR_UPDOWN)
    {
        ret*=matStairCase;
    }
    if(d.bits.liquid_type == df::enums::tile_liquid::Water && d.bits.flow_size > 0)
    {
        ret *=matWater.transparency.pow((float)d.bits.flow_size/7.0f);
    }
    else if(d.bits.liquid_type == df::enums::tile_liquid::Magma && d.bits.flow_size > 0)
    {
        ret *=matLava.transparency.pow((float)d.bits.flow_size/7.0f);
    }
    return ret;
}
coord2d lightingEngineViewscreen::worldToViewportCoord(const coord2d& in,const rect2d& r,const coord2d& window2d)
{
    return in-window2d+r.first;
}

void lightingEngineViewscreen::doSun(const lightSource& sky,MapExtras::MapCache& map)
{
    //TODO fix this mess
    int window_x=*df::global::window_x;
    int window_y=*df::global::window_y;
    coord2d window2d(window_x,window_y);
    int window_z=*df::global::window_z;
    rect2d vp=getMapViewport();
    coord2d vpSize=rect_size(vp);
    rect2d blockVp;
    blockVp.first=window2d/16;
    blockVp.second=(window2d+vpSize)/16;
    blockVp.second.x=std::min(blockVp.second.x,(int16_t)df::global::world->map.x_count_block);
    blockVp.second.y=std::min(blockVp.second.y,(int16_t)df::global::world->map.y_count_block);
    //endof mess
    for(int blockX=blockVp.first.x;blockX<=blockVp.second.x;blockX++)
    for(int blockY=blockVp.first.y;blockY<=blockVp.second.y;blockY++)
    {
        rgbf cellArray[16][16];
        for(int block_x = 0; block_x < 16; block_x++)
        for(int block_y = 0; block_y < 16; block_y++)
            cellArray[block_x][block_y] = sky.power;

        int emptyCell=0;
        for(int z=window_z;z< df::global::world->map.z_count && emptyCell<256;z++)
        {
            MapExtras::Block* b=map.BlockAt(DFCoord(blockX,blockY,z));
            if(!b)
                continue;
            emptyCell=0;
            for(int block_x = 0; block_x < 16; block_x++)
            for(int block_y = 0; block_y < 16; block_y++)
            {
                rgbf& curCell=cellArray[block_x][block_y];
                curCell=propogateSun(b,block_x,block_y,curCell,z==window_z);
                if(curCell.dot(curCell)<0.003f)
                    emptyCell++;
            }
        }
        if(emptyCell==256)
            continue;
        for(int block_x = 0; block_x < 16; block_x++)
        for(int block_y = 0; block_y < 16; block_y++)
        {
            rgbf& curCell=cellArray[block_x][block_y];
            df::coord2d pos;
            pos.x = blockX*16+block_x;
            pos.y = blockY*16+block_y;
            pos=worldToViewportCoord(pos,vp,window2d);
            if(isInRect(pos,vp) && curCell.dot(curCell)>0.003f)
            {
                lightSource sun=lightSource(curCell,15);
                addLight(getIndex(pos.x,pos.y),sun);
            }
        }
    }
}
rgbf lightingEngineViewscreen::getSkyColor(float v)
{
    if(dayColors.size()<2)
    {
        v=abs(fmod(v+0.5,1)-0.5)*2;
        return rgbf(v,v,v);
    }
    else
    {
        float pos=v*(dayColors.size()-1);
        int pre=floor(pos);
        pos-=pre;
        if(pre==int(dayColors.size())-1)
            return dayColors[pre];
        return dayColors[pre]*(1-pos)+dayColors[pre+1]*pos;
    }
}
void lightingEngineViewscreen::doOcupancyAndLights()
{
    float daycol;
    if(dayHour<0)
    {
        int length=1200/daySpeed;
        daycol= (*df::global::cur_year_tick % length)/ (float)length;
    }
    else
        daycol= fmod(dayHour,24.0f)/24.0f; //1->12h 0->24h

    rgbf sky_col=getSkyColor(daycol);
    lightSource sky(sky_col, -1);//auto calculate best size

    MapExtras::MapCache cache;
    doSun(sky,cache);

    int window_x=*df::global::window_x;
    int window_y=*df::global::window_y;
    coord2d window2d(window_x,window_y);
    int window_z=*df::global::window_z;
    rect2d vp=getMapViewport();
    coord2d vpSize=rect_size(vp);
    rect2d blockVp;
    blockVp.first=coord2d(window_x,window_y)/16;
    blockVp.second=(window2d+vpSize)/16;
    blockVp.second.x=std::min(blockVp.second.x,(int16_t)df::global::world->map.x_count_block);
    blockVp.second.y=std::min(blockVp.second.y,(int16_t)df::global::world->map.y_count_block);

    for(int blockX=blockVp.first.x;blockX<=blockVp.second.x;blockX++)
    for(int blockY=blockVp.first.y;blockY<=blockVp.second.y;blockY++)
    {
        MapExtras::Block* b=cache.BlockAt(DFCoord(blockX,blockY,window_z));
        MapExtras::Block* bDown=cache.BlockAt(DFCoord(blockX,blockY,window_z-1));
        if(!b)
            continue; //empty blocks fixed by sun propagation

        for(int block_x = 0; block_x < 16; block_x++)
        for(int block_y = 0; block_y < 16; block_y++)
        {
            df::coord2d pos;
            pos.x = blockX*16+block_x;
            pos.y = blockY*16+block_y;
            df::coord2d gpos=pos;
            pos=worldToViewportCoord(pos,vp,window2d);
            if(!isInRect(pos,vp))
                continue;
            int tile=getIndex(pos.x,pos.y);
            rgbf& curCell=ocupancy[tile];
            curCell=matAmbience.transparency;


            df::tiletype type = b->tiletypeAt(gpos);
            df::tile_designation d = b->DesignationAt(gpos);
            if(d.bits.hidden )
            {
                curCell=rgbf(0,0,0);
                continue; // do not process hidden stuff, TODO other hidden stuff
            }
            //df::tile_occupancy o = b->OccupancyAt(gpos);
            df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,type);
            bool is_wall=!ENUM_ATTR(tiletype_shape,passable_high,shape);
            bool is_floor=!ENUM_ATTR(tiletype_shape,passable_low,shape);
            // df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
            df::tiletype_material tileMat= ENUM_ATTR(tiletype,material,type);

            DFHack::t_matpair mat=b->staticMaterialAt(gpos);

            matLightDef* lightDef=getMaterialDef(mat.mat_type,mat.mat_index);
            if(!lightDef || !lightDef->isTransparent)
                lightDef=&matWall;
            if(shape==df::tiletype_shape::BROOK_BED )
            {
                curCell=rgbf(0,0,0);
            }
            else if(is_wall)
            {
                if(tileMat==df::tiletype_material::FROZEN_LIQUID)
                    applyMaterial(tile,matIce);
                else
                    applyMaterial(tile,*lightDef);
            }
            else if(!d.bits.liquid_type && d.bits.flow_size>0 )
            {
                applyMaterial(tile,matWater, (float)d.bits.flow_size/7.0f, (float)d.bits.flow_size/7.0f);
            }
            if(d.bits.liquid_type && d.bits.flow_size>0)
            {
                applyMaterial(tile,matLava,(float)d.bits.flow_size/7.0f,(float)d.bits.flow_size/7.0f);
            }
            else if(!is_floor)
            {
                if(bDown)
                {
                   df::tile_designation d2=bDown->DesignationAt(gpos);
                   if(d2.bits.liquid_type && d2.bits.flow_size>0)
                   {
                       applyMaterial(tile,matLava);
                   }
                }
            }


        }

        df::map_block* block=b->getRaw();
        if(!block)
            continue;
        //flows
        for(size_t i=0;i<block->flows.size();i++)
        {
            df::flow_info* f=block->flows[i];
            if(f && f->density>0 && (f->type==df::flow_type::Dragonfire || f->type==df::flow_type::Fire))
            {
                df::coord2d pos=f->pos;
                pos=worldToViewportCoord(pos,vp,window2d);
                int tile=getIndex(pos.x,pos.y);
                if(isInRect(pos,vp))
                {
                    rgbf fireColor;
                    if(f->density>60)
                    {
                        fireColor=rgbf(0.98f,0.91f,0.30f);
                    }
                    else if(f->density>30)
                    {
                        fireColor=rgbf(0.93f,0.16f,0.16f);
                    }
                    else
                    {
                        fireColor=rgbf(0.64f,0.0f,0.0f);
                    }
                    lightSource fire(fireColor,f->density/5);
                    addLight(tile,fire);
                }
            }
        }

        //blood and other goo
        for(size_t i=0;i<block->block_events.size();i++)
        {
            df::block_square_event* ev=block->block_events[i];
            df::block_square_event_type ev_type=ev->getType();
            if(ev_type==df::block_square_event_type::material_spatter)
            {
                df::block_square_event_material_spatterst* spatter=static_cast<df::block_square_event_material_spatterst*>(ev);
                matLightDef* m=getMaterialDef(spatter->mat_type,spatter->mat_index);
                if(!m)
                    continue;
                if(!m->isEmiting)
                    continue;
                for(int x=0;x<16;x++)
                for(int y=0;y<16;y++)
                {
                    df::coord2d pos;
                    pos.x = blockX*16+x;
                    pos.y = blockY*16+y;
                    int16_t amount=spatter->amount[x][y];
                    if(amount<=0)
                        continue;
                    pos=worldToViewportCoord(pos,vp,window2d);
                    if(isInRect(pos,vp))
                    {
                        addLight(getIndex(pos.x,pos.y),m->makeSource((float)amount/100));
                    }
                }
            }
        }
    }
    if(df::global::cursor->x>-30000)
    {
        int wx=df::global::cursor->x-window_x+vp.first.x;
        int wy=df::global::cursor->y-window_y+vp.first.y;
        int tile=getIndex(wx,wy);
        applyMaterial(tile,matCursor);
    }
    //citizen only emit light, if defined
    //or other creatures
    if(matCitizen.isEmiting || creatureDefs.size()>0)
    for (size_t i=0;i<df::global::world->units.active.size();++i)
    {
        df::unit *u = df::global::world->units.active[i];
        coord2d pos=worldToViewportCoord(coord2d(u->pos.x,u->pos.y),vp,window2d);
        if(u->pos.z==window_z && isInRect(pos,vp))
        {
            if (DFHack::Units::isCitizen(u) && !u->counters.unconscious)
                addLight(getIndex(pos.x,pos.y),matCitizen.makeSource());
            creatureLightDef *def=getCreatureDef(u);
            if(def && Units::isActive(u))
            {
                addLight(getIndex(pos.x,pos.y),def->light.makeSource());
            }
        }
    }
    //items
    if(itemDefs.size()>0)
    {
        std::vector<df::item*>& vec=df::global::world->items.other[items_other_id::IN_PLAY];
        for(size_t i=0;i<vec.size();i++)
        {
            df::item* curItem=vec[i];
            df::coord itemPos=DFHack::Items::getPosition(curItem);
            coord2d pos=worldToViewportCoord(itemPos,vp,window2d);
            itemLightDef* mat=0;
            if( itemPos.z==window_z && isInRect(pos,vp) && (mat=getItemDef(curItem)) )
            {
                if( ((mat->equiped || mat->haul ||mat->inBuilding ||mat->inContainer) && curItem->flags.bits.in_inventory)|| //TODO split this up
                    (mat->onGround && curItem->flags.bits.on_ground) )
                {
                    if(mat->light.isEmiting)
                        addLight(getIndex(pos.x,pos.y),mat->light.makeSource());
                    if(!mat->light.isTransparent)
                        addOclusion(getIndex(pos.x,pos.y),mat->light.transparency,1);
                }
            }
        }
    }

    //buildings
    for(size_t i = 0; i < df::global::world->buildings.all.size(); i++)
    {
        df::building *bld = df::global::world->buildings.all[i];

        if(window_z!=bld->z)
            continue;
        if(bld->getBuildStage()<bld->getMaxBuildStage()) //only work if fully built
            continue;

        df::coord2d p1(bld->x1,bld->y1);
        df::coord2d p2(bld->x2,bld->y2);
        p1=worldToViewportCoord(p1,vp,window2d);
        p2=worldToViewportCoord(p2,vp,window2d);
        if(isInRect(p1,vp)||isInRect(p2,vp))
        {

            int tile;
            if(isInRect(p1,vp))
                tile=getIndex(p1.x,p1.y); //TODO multitile buildings. How they would work?
            else
                tile=getIndex(p2.x,p2.y);
            df::building_type type = bld->getType();
            buildingLightDef* def=getBuildingDef(bld);
            if(!def)
                continue;
            if(type==df::enums::building_type::Door)
            {
                df::building_doorst* door=static_cast<df::building_doorst*>(bld);
                if(!door->door_flags.bits.closed)
                    continue;
            }
            else if(type==df::enums::building_type::Floodgate)
            {
                df::building_floodgatest* gate=static_cast<df::building_floodgatest*>(bld);
                if(!gate->gate_flags.bits.closed)
                    continue;
            }


            if(def->useMaterial)
            {
                matLightDef* mat=getMaterialDef(bld->mat_type,bld->mat_index);
                if(!mat)mat=&matWall;
                if(!def->poweredOnly || !bld->isUnpowered()) //not powered. Add occlusion only.
                {
                    if(def->light.isEmiting)
                    {
                        addLight(tile,def->light.makeSource(def->size));
                    }
                    else if(mat->isEmiting)
                    {
                        addLight(tile,mat->makeSource(def->size));
                    }
                }
                if(def->light.isTransparent)
                {
                    addOclusion(tile,def->light.transparency,def->size);
                }
                else
                {
                    addOclusion(tile,mat->transparency,def->size);
                }
            }
            else
            {
                if(!def->poweredOnly || !bld->isUnpowered())//not powered. Add occlusion only.
                    addOclusion(tile,def->light.transparency,def->size);
                else
                    applyMaterial(tile,def->light,def->size,def->thickness);
            }
        }
    }

}
rgbf lua_parseLightCell(lua_State* L)
{
    rgbf ret;

    lua_pushnumber(L,1);
    lua_gettable(L,-2);
    ret.r=lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_pushnumber(L,2);
    lua_gettable(L,-2);
    ret.g=lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_pushnumber(L,3);
    lua_gettable(L,-2);
    ret.b=lua_tonumber(L,-1);
    lua_pop(L,1);

    //Lua::GetOutput(L)->print("got cell(%f,%f,%f)\n",ret.r,ret.g,ret.b);
    return ret;
}
#define GETLUAFLAG(field,name) lua_getfield(L,-1,"flags");\
    if(lua_isnil(L,-1)){field=false;}\
    else{lua_getfield(L,-1,#name);field=lua_isnil(L,-1);lua_pop(L,1);}\
    lua_pop(L,1)

#define GETLUANUMBER(field,name) lua_getfield(L,-1,#name);\
    if(!lua_isnil(L,-1) && lua_isnumber(L,-1))field=lua_tonumber(L,-1);\
    lua_pop(L,1)
matLightDef lua_parseMatDef(lua_State* L)
{

    matLightDef ret;
    lua_getfield(L,-1,"tr");
    if((ret.isTransparent=!lua_isnil(L,-1)))
    {
        ret.transparency=lua_parseLightCell(L);
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"em");
    if((ret.isEmiting=!lua_isnil(L,-1)))
    {
        ret.emitColor=lua_parseLightCell(L);
        lua_pop(L,1);
        lua_getfield(L,-1,"rad");
        if(lua_isnil(L,-1))
        {
            lua_pop(L,1);
            luaL_error(L,"Material has emittance but no radius");
        }
        else
            ret.radius=lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    else
        lua_pop(L,1);
    GETLUAFLAG(ret.flicker,"flicker");
    return ret;
}
int lightingEngineViewscreen::parseMaterials(lua_State* L)
{
    auto engine= (lightingEngineViewscreen*)lua_touserdata(L, 1);
    engine->matDefs.clear();
    //color_ostream* os=Lua::GetOutput(L);
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"materials");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Materials table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        int type=lua_tonumber(L,-2);
        //os->print("Processing type:%d\n",type);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int index=lua_tonumber(L,-2);
            //os->print("\tProcessing index:%d\n",index);
            engine->matDefs[std::make_pair(type,index)]=lua_parseMatDef(L);

            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    return 0;
}
#define LOAD_SPECIAL(lua_name,class_name) \
    lua_getfield(L,-1,#lua_name);\
    if(!lua_isnil(L,-1))engine->class_name=lua_parseMatDef(L);\
    lua_pop(L,1)
int lightingEngineViewscreen::parseSpecial(lua_State* L)
{
    auto engine= (lightingEngineViewscreen*)lua_touserdata(L, 1);
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"special");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Special table not found.");
        return 0;
    }
    LOAD_SPECIAL(LAVA,matLava);
    LOAD_SPECIAL(WATER,matWater);
    LOAD_SPECIAL(FROZEN_LIQUID,matIce);
    LOAD_SPECIAL(AMBIENT,matAmbience);
    LOAD_SPECIAL(CURSOR,matCursor);
    LOAD_SPECIAL(CITIZEN,matCitizen);
    GETLUANUMBER(engine->levelDim,levelDim);
    GETLUANUMBER(engine->dayHour,dayHour);
    GETLUANUMBER(engine->daySpeed,daySpeed);
    GETLUANUMBER(engine->num_diffuse,diffusionCount);
    GETLUANUMBER(engine->adv_mode,advMode);
    lua_getfield(L,-1,"dayColors");
    if(lua_istable(L,-1))
    {
        engine->dayColors.clear();
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            engine->dayColors.push_back(lua_parseLightCell(L));
            lua_pop(L,1);
        }
        lua_pop(L,1);
    }
    return 0;
}
#undef LOAD_SPECIAL
int lightingEngineViewscreen::parseItems(lua_State* L)
{
    auto engine= (lightingEngineViewscreen*)lua_touserdata(L, 1);
    engine->itemDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"items");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Items table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if(!lua_istable(L,-1))
            luaL_error(L,"Broken item definitions.");
        lua_getfield(L,-1,"type");
        int type=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"subtype");
        int subtype=luaL_optinteger(L,-1,-1);
        lua_pop(L,1);
        itemLightDef item;
        lua_getfield(L,-1,"light");
        item.light=lua_parseMatDef(L);
        GETLUAFLAG(item.haul,"hauling");
        GETLUAFLAG(item.equiped,"equiped");
        GETLUAFLAG(item.inBuilding,"inBuilding");
        GETLUAFLAG(item.inContainer,"contained");
        GETLUAFLAG(item.onGround,"onGround");
        GETLUAFLAG(item.useMaterial,"useMaterial");
        engine->itemDefs[std::make_pair(type,subtype)]=item;
        lua_pop(L,2);
    }
    lua_pop(L,1);
    return 0;
}
int lightingEngineViewscreen::parseCreatures(lua_State* L)
{
    auto engine= (lightingEngineViewscreen*)lua_touserdata(L, 1);
    engine->creatureDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"creatures");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Creatures table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if(!lua_istable(L,-1))
            luaL_error(L,"Broken creature definitions.");
        lua_getfield(L,-1,"race");
        int race=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"caste");
        int caste=lua_tonumber(L,-1);
        lua_pop(L,1);
        creatureLightDef cr;
        lua_getfield(L,-1,"light");
        cr.light=lua_parseMatDef(L);
        engine->creatureDefs[std::make_pair(race,caste)]=cr;
        lua_pop(L,2);
    }
    lua_pop(L,1);
    return 0;
}
int lightingEngineViewscreen::parseBuildings(lua_State* L)
{
    auto engine= (lightingEngineViewscreen*)lua_touserdata(L, 1);
    engine->buildingDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"buildings");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Buildings table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        int type=lua_tonumber(L,-2);
        if(!lua_istable(L,-1))
        {
            luaL_error(L,"Broken building definitions.");
        }
        //os->print("Processing type:%d\n",type);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int subtype=lua_tonumber(L,-2);
            //os->print("\tProcessing subtype:%d\n",index);
            lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int custom=lua_tonumber(L,-2);
            //os->print("\tProcessing custom:%d\n",index);
            buildingLightDef current;
            current.light=lua_parseMatDef(L);
            engine->buildingDefs[std::make_tuple(type,subtype,custom)]=current;
            GETLUAFLAG(current.poweredOnly,"poweredOnly");
            GETLUAFLAG(current.useMaterial,"useMaterial");

            lua_getfield(L,-1,"size");
            current.size=luaL_optnumber(L,-1,1);
            lua_pop(L,1);

            lua_getfield(L,-1,"thickness");
            current.thickness=luaL_optnumber(L,-1,1);
            lua_pop(L,1);

            lua_pop(L, 1);
        }

            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L,1);
    return 0;
}
void lightingEngineViewscreen::defaultSettings()
{
    matAmbience=matLightDef(rgbf(0.85f,0.85f,0.85f));
    matLava=matLightDef(rgbf(0.8f,0.2f,0.2f),rgbf(0.8f,0.2f,0.2f),5);
    matWater=matLightDef(rgbf(0.6f,0.6f,0.8f));
    matIce=matLightDef(rgbf(0.7f,0.7f,0.9f));
    matCursor=matLightDef(rgbf(0.96f,0.84f,0.03f),11);
    matCursor.flicker=true;
    matWall=matLightDef(rgbf(0,0,0));
    matCitizen=matLightDef(rgbf(0.8f,0.8f,0.9f),6);
    levelDim=0.2f;
    dayHour=-1;
    daySpeed=1;
    adv_mode=0;
    num_diffuse=0;
    dayColors.push_back(rgbf(0,0,0));
    dayColors.push_back(rgbf(1,1,1));
    dayColors.push_back(rgbf(0,0,0));
}
void lightingEngineViewscreen::loadSettings()
{
    std::string rawFolder;
    if(df::global::world->cur_savegame.save_dir!="")
    {
        rawFolder= "save/" + (df::global::world->cur_savegame.save_dir) + "/";
    }
    else
    {
        rawFolder= "dfhack-config/";
    }
    const std::string settingsfile=rawFolder+"rendermax.lua";

    CoreSuspender lock;
    color_ostream_proxy out(Core::getInstance().getConsole());

    lua_State* s=DFHack::Lua::Core::State;
    lua_newtable(s);
    int env=lua_gettop(s);
    try{
        int ret=luaL_loadfile(s,settingsfile.c_str());
        if(ret==LUA_ERRFILE)
        {
            out.printerr("File not found:%s\n",settingsfile.c_str());
            lua_pop(s,1);
        }
        else if(ret==LUA_ERRSYNTAX)
        {
            out.printerr("Syntax error:\n\t%s\n",lua_tostring(s,-1));
        }
        else
        {
            lua_pushvalue(s,env);

            if(Lua::SafeCall(out,s,1,0))
            {
                lua_pushcfunction(s, parseMaterials);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%zu materials loaded\n",matDefs.size());

                lua_pushcfunction(s, parseSpecial);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%zu day light colors loaded\n",dayColors.size());

                lua_pushcfunction(s, parseBuildings);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%zu buildings loaded\n",buildingDefs.size());

                lua_pushcfunction(s, parseCreatures);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%zu creatures loaded\n",creatureDefs.size());

                lua_pushcfunction(s, parseItems);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%zu items loaded\n",itemDefs.size());
            }

        }
    }
    catch(std::exception& e)
    {
        out.printerr("%s",e.what());
    }
    lua_pop(s,1);
}
#undef GETLUAFLAG
#undef GETLUANUMBER
/*
 *      Threading stuff
 */
lightThread::lightThread( lightThreadDispatch& dispatch ):dispatch(dispatch),myThread(0),isDone(false)
{

}
lightThread::~lightThread()
{
    delete myThread;
}

void lightThread::run()
{
    while(!isDone)
    {
        //TODO: get area to process, and then process (by rounds): 1. occlusions, 2.sun, 3.lights(could be difficult, units/items etc...)
        {//wait for occlusion (and lights) to be ready
            std::unique_lock<std::mutex> guard(dispatch.occlusionMutex);
            if(!dispatch.occlusionReady)
                dispatch.occlusionDone.wait(guard);//wait for work
            if(dispatch.unprocessed.size()==0 || !dispatch.occlusionReady) //spurious wake-up
                continue;
            if(dispatch.occlusion.size()!=canvas.size()) //oh no somebody resized stuff
                canvas.resize(dispatch.occlusion.size());
        }


        { //get my rectangle (any will do)
            std::lock_guard<std::mutex> guard(dispatch.unprocessedMutex);
            if (dispatch.unprocessed.size()==0)
            {
                //wtf?? why?!
                continue;
            }
            myRect=dispatch.unprocessed.top();
            dispatch.unprocessed.pop();
            if (dispatch.unprocessed.size()==0)
            {
                dispatch.occlusionReady=false;
            }

        }
        work();
        {
            std::lock_guard<std::mutex> guard(dispatch.writeLock);
            combine();//write it back
            dispatch.writeCount++;
        }
        dispatch.writesDone.notify_one();//tell about it to the dispatch.
    }
}

void lightThread::work()
{
    canvas.assign(canvas.size(),rgbf(0,0,0));
    for(int i=myRect.first.x;i<myRect.second.x;i++)
    for(int j=myRect.first.y;j<myRect.second.y;j++)
    {
        doLight(i,j);
    }
}

void lightThread::combine()
{
    for(size_t i=0;i<canvas.size();i++)
    {
        rgbf& c=dispatch.lightMap[i];
        c=blend(c,canvas[i]);
    }
}


rgbf lightThread::lightUpCell(rgbf power,int dx,int dy,int tx,int ty)
{
    int h=dispatch.getH();
    if(isInRect(coord2d(tx,ty),dispatch.viewPort))
    {
        size_t tile=tx*h+ty;
        int dsq=dx*dx+dy*dy;
        float dt=1;
        if(dsq == 1)//array lookup might be faster still
            dt=1;
        else if(dsq == 2)
            dt = RootTwo;
        else if(dsq == 0)
            dt = 0;
        else
            dt=sqrt((float)dsq);
        rgbf& v=dispatch.occlusion[tile];
        lightSource& ls=dispatch.lights[tile];
        bool wallhack=false;
        if(v.r+v.g+v.b==0)
            wallhack=true;

        if (dsq>0 && !wallhack)
        {
            power*=v.pow(dt);
        }
        if(ls.radius>0 && dsq>0)
        {
            if(power<=ls.power) //quit early if hitting another (stronger) lightsource
                return rgbf();
        }

        rgbf oldCol=canvas[tile];
        rgbf ncol=blendMax(power,oldCol);
        canvas[tile]=ncol;

        if(wallhack)
            return rgbf();


        return power;
    }
    else
        return rgbf();
}
void lightThread::doRay(const rgbf& power,int cx,int cy,int tx,int ty,int num_diffuse)
{
    using namespace std::placeholders;
    plotLineDiffuse(cx,cy,tx,ty,power,num_diffuse,std::bind(&lightThread::lightUpCell,this,_1,_2,_3,_4,_5));
}

void lightThread::doLight( int x,int y )
{
    using namespace std::placeholders;
    lightSource& csource=dispatch.lights[x*dispatch.getH()+y];
    int num_diffuse=dispatch.num_diffusion;
    if(csource.radius>0)
    {
        rgbf power=csource.power;
        int radius =csource.radius;
        if(csource.flicker)
        {
            float flicker=(rand()/(float)RAND_MAX)/2.0f+0.5f;
            radius*=flicker;
            power=power*flicker;
        }
        rgbf surrounds;
        lightUpCell( power, 0, 0,x, y); //light up the source itself
        for(int i=-1;i<2;i++)
            for(int j=-1;j<2;j++)
                if(i!=0||j!=0)
                    surrounds += lightUpCell( power, i, j,x+i, y+j); //and this is wall hack (so that walls look nice)
        if(surrounds.dot(surrounds)>0.00001f) //if we needed to light up the suroundings, then raycast
        {

            plotSquare(x,y,radius,
                std::bind(&lightThread::doRay,this,power,x,y,_1,_2,num_diffuse));
        }
    }
}
void lightThreadDispatch::signalDoneOcclusion()
{
    {
        std::lock_guard<std::mutex> guardWrite(writeLock);
        writeCount=0;
    }
    std::lock_guard<std::mutex> guard1(occlusionMutex);
    std::lock_guard<std::mutex> guard2(unprocessedMutex);
    while(!unprocessed.empty())
        unprocessed.pop();
    viewPort=getMapViewport();
    int threadCount=threadPool.size();
    int w=viewPort.second.x-viewPort.first.x;
    int slicew=w/threadCount;
    for(int i=0;i<threadCount;i++)
    {
        rect2d area=viewPort;
        area.first.x=viewPort.first.x+i*slicew;
        if(i==threadCount-1)
            area.second.x=viewPort.second.x; //fix, when area is even number or sth.
        else
            area.second.x=viewPort.first.x+(i+1)*slicew;
        unprocessed.push(area);
    }
    occlusionReady=true;
    occlusionDone.notify_all();
}

lightThreadDispatch::lightThreadDispatch( lightingEngineViewscreen* p ):parent(p),lights(parent->lights),
    occlusionReady(false),occlusion(parent->ocupancy),num_diffusion(parent->num_diffuse),
    lightMap(parent->lightMap),writeCount(0)
{

}

void lightThreadDispatch::shutdown()
{
    for(size_t i=0;i<threadPool.size();i++)
    {
        threadPool[i]->isDone=true;

    }
    occlusionDone.notify_all();//if stuck signal that you are done with stuff.
    for(size_t i=0;i<threadPool.size();i++)
    {
        threadPool[i]->myThread->join();
    }
    threadPool.clear();
}

int lightThreadDispatch::getW()
{
    return parent->getW();
}

int lightThreadDispatch::getH()
{
    return parent->getH();
}
void threadStub(void * arg)
{
    if(arg)
        ((lightThread*)arg)->run();
}
void lightThreadDispatch::start(int count)
{
    for(int i=0;i<count;i++)
    {
        std::unique_ptr<lightThread> nthread(new lightThread(*this));
        nthread->myThread=new std::thread(&threadStub,nthread.get());
        threadPool.push_back(std::move(nthread));
    }
}

void lightThreadDispatch::waitForWrites()
{
    std::unique_lock<std::mutex> lock(writeLock);
    while(threadPool.size()>size_t(writeCount))//missed it somehow already.
    {
        writesDone.wait(lock); //if not, wait a bit
    }
}

lightThreadDispatch::~lightThreadDispatch()
{
    shutdown();
}
