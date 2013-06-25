#ifndef RENDERER_LIGHT_INCLUDED
#define RENDERER_LIGHT_INCLUDED
#include "renderer_opengl.hpp"
#include "Types.h"
#include <map>
#include "modules/MapCache.h"

struct renderer_light : public renderer_wrap {
private:
    void colorizeTile(int x,int y)
    {
        const int tile = x*(df::global::gps->dimy) + y;
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        float *tex = p->tex + tile * 2 * 6;
        lightCell light=lightGrid[tile];
        for (int i = 0; i < 6; i++) {
            *(fg++) *= light.r;
            *(fg++) *= light.g;
            *(fg++) *= light.b;
            *(fg++) = 1;

            *(bg++) *= light.r;
            *(bg++) *= light.g;
            *(bg++) *= light.b;
            *(bg++) = 1;
        }
    }
    void reinitLightGrid(int w,int h)
    {
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        lightGrid.resize(w*h);
    }
    void reinitLightGrid()
    {
        reinitLightGrid(df::global::gps->dimy,df::global::gps->dimx);
    }
public:
    tthread::fast_mutex dataMutex;
    std::vector<lightCell> lightGrid;
    renderer_light(renderer* parent):renderer_wrap(parent)
    {
        reinitLightGrid();
    }
    virtual void update_tile(int32_t x, int32_t y) { 
        renderer_wrap::update_tile(x,y);
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        colorizeTile(x,y);
    };
    virtual void update_all() { 
        renderer_wrap::update_all();
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        for (int x = 0; x < df::global::gps->dimx; x++)
            for (int y = 0; y < df::global::gps->dimy; y++)
                colorizeTile(x,y);
    };
    virtual void grid_resize(int32_t w, int32_t h) { 
        renderer_wrap::grid_resize(w,h);
        reinitLightGrid(w,h);
    };
    virtual void resize(int32_t w, int32_t h) {
        renderer_wrap::resize(w,h);
        reinitLightGrid();
    }
};
class lightingEngine
{
public:
    lightingEngine(renderer_light* target):myRenderer(target){}

    virtual void reinit()=0;
    virtual void calculate()=0;

    virtual void updateWindow()=0;

    virtual void loadSettings()=0;
    virtual void clear()=0;
    
protected:
    renderer_light* myRenderer;
};
struct lightSource
{
    lightCell power;
    int radius;
    bool flicker;
    lightSource():power(0,0,0),radius(0),flicker(false)
    {

    }
    lightSource(lightCell power,int radius);
    float powerSquared()const
    {
        return power.r*power.r+power.g*power.g+power.b*power.b;
    }
    void combine(const lightSource& other);

};
struct matLightDef
{
    int mat_index;
    int mat_type;
    bool isTransparent;
    lightCell transparency;
    bool isEmiting;
    bool sizeModifiesPower;
    bool sizeModifiesRange;
    bool flicker;
    lightCell emitColor;
    int radius;
    matLightDef():isTransparent(false),isEmiting(false),transparency(0,0,0),emitColor(0,0,0),radius(0){}
    matLightDef(lightCell transparency,lightCell emit,int rad):isTransparent(true),isEmiting(true),
        transparency(transparency),emitColor(emit),radius(rad){}
    matLightDef(lightCell emit,int rad):isTransparent(false),isEmiting(true),emitColor(emit),radius(rad),transparency(0,0,0){}
    matLightDef(lightCell transparency):isTransparent(true),isEmiting(false),transparency(transparency){}
    lightSource makeSource(float size=1) const
    {
        //TODO implement sizeModifiesPower/range
        return lightSource(emitColor,radius);
    }
};
class lightingEngineViewscreen:public lightingEngine
{
public:
    lightingEngineViewscreen(renderer_light* target);

    void reinit();
    void calculate();

    void updateWindow();

    void loadSettings();
    void clear();
private:

    df::coord2d worldToViewportCoord(const df::coord2d& in,const DFHack::rect2d& r,const df::coord2d& window2d) ;
    bool isInViewport(const df::coord2d& in,const DFHack::rect2d& r);

    void doSun(const lightSource& sky,MapExtras::MapCache& map);
    void doOcupancyAndLights();
    lightCell propogateSun(MapExtras::Block* b, int x,int y,const lightCell& in,bool lastLevel);
    void doRay(lightCell power,int cx,int cy,int tx,int ty);
    void doFovs();
    bool lightUpCell(lightCell& power,int dx,int dy,int tx,int ty);
    bool addLight(int tileId,const lightSource& light);

    matLightDef* getMaterial(int matType,int matIndex);
    //apply material to cell
    void applyMaterial(int tileId,const matLightDef& mat,float size=1, float thickness = 1);
    //try to find and apply material, if failed return false, and if def!=null then apply def.
    bool applyMaterial(int tileId,int matType,int matIndex,float size=1,const matLightDef* def=NULL);
    size_t inline getIndex(int x,int y)
    {
        return x*h+y;
    }
    //maps
    std::vector<lightCell> lightMap;
    std::vector<lightCell> ocupancy;
    std::vector<lightSource> lights;
    //settings

    ///set up sane settings if setting file does not exist.
    void defaultSettings(); 

    static int parseMaterials(lua_State* L);
    static int parseSpecial(lua_State* L);
    //special stuff
    matLightDef matLava;
    matLightDef matIce;
    matLightDef matAmbience;
    matLightDef matCursor;
    matLightDef matWall;
    matLightDef matWater;
    matLightDef matCitizen;
    float levelDim;
    //materials
    std::map<std::pair<int,int>,matLightDef> matDefs;

    int w,h;
    DFHack::rect2d mapPort;
};
#endif
