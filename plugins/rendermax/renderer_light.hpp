#ifndef RENDERER_LIGHT_INCLUDED
#define RENDERER_LIGHT_INCLUDED
#include "renderer_opengl.hpp"
#include "Types.h"
#include <tuple>
#include <stack>
#include <memory>
#include <unordered_map>
// we are not using boost so let's cheat:
template <class T>
inline void hash_combine(std::size_t & seed, const T & v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template<typename S, typename T> struct hash<pair<S, T>>
    {
        inline size_t operator()(const pair<S, T> & v) const
        {
            size_t seed = 0;
            ::hash_combine(seed, v.first);
            ::hash_combine(seed, v.second);
            return seed;
        }
    };
    template<typename S, typename T,typename V> struct hash<tuple<S, T, V>>
    {
        inline size_t operator()(const tuple<S, T, V> & v) const
        {
            size_t seed = 0;
            ::hash_combine(seed,get<0>(v));
            ::hash_combine(seed,get<1>(v));
            ::hash_combine(seed,get<2>(v));
            return seed;
        }
    };
}
// now we can hash pairs and tuples

#include "modules/MapCache.h"
bool isInRect(const df::coord2d& pos,const DFHack::rect2d& rect);
struct renderer_light : public renderer_wrap {
private:
    void colorizeTile(int x,int y)
    {
        const int tile = x*(df::global::gps->dimy) + y;
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        float *tex = p->tex + tile * 2 * 6;
        rgbf light=lightGrid[tile];
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
    std::vector<rgbf> lightGrid;
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
    virtual ~lightingEngine(){}
    virtual void reinit()=0;
    virtual void calculate()=0;

    virtual void updateWindow()=0;

    virtual void loadSettings()=0;
    virtual void clear()=0;
    
    virtual void setHour(float h)=0;
    virtual void debug(bool enable)=0;
protected:
    renderer_light* myRenderer;
};
struct lightSource
{
    rgbf power;
    int radius;
    bool flicker;
    lightSource():power(0,0,0),radius(0),flicker(false)
    {

    }
    lightSource(rgbf power,int radius);
    float powerSquared()const
    {
        return power.r*power.r+power.g*power.g+power.b*power.b;
    }
    void combine(const lightSource& other);

};
struct matLightDef
{
    bool isTransparent;
    rgbf transparency;
    bool isEmiting;
    bool flicker;
    rgbf emitColor;
    int radius;
    matLightDef():isTransparent(false),isEmiting(false),transparency(0,0,0),emitColor(0,0,0),radius(0){}
    matLightDef(rgbf transparency,rgbf emit,int rad):isTransparent(true),isEmiting(true),
        transparency(transparency),emitColor(emit),radius(rad){}
    matLightDef(rgbf emit,int rad):isTransparent(false),isEmiting(true),emitColor(emit),radius(rad),transparency(0,0,0){}
    matLightDef(rgbf transparency):isTransparent(true),isEmiting(false),transparency(transparency){}
    lightSource makeSource(float size=1) const
    {
        if(size>0.999 && size<1.001)
            return lightSource(emitColor,radius);
        else
            return lightSource(emitColor*size,radius*size);//todo check if this is sane
    }
};
struct buildingLightDef
{
    matLightDef light;
    bool poweredOnly;
    bool useMaterial;
    float thickness;
    float size;
    buildingLightDef():poweredOnly(false),useMaterial(true),thickness(1.0f),size(1.0f){}
};
class lightThread;
class lightingEngineViewscreen;
class lightThreadDispatch
{
    lightingEngineViewscreen *parent;
public:
    DFHack::rect2d viewPort;

    std::vector<std::unique_ptr<lightThread> > threadPool;
    std::vector<lightSource>& lights;

    tthread::mutex occlusionMutex;
    tthread::condition_variable occlusionDone; //all threads wait for occlusion to finish
    bool occlusionReady;
    tthread::mutex unprocessedMutex;
    std::stack<DFHack::rect2d> unprocessed; //stack of parts of map where lighting is not finished
    std::vector<rgbf>& occlusion;
    
    tthread::mutex writeLock; //mutex for lightMap
    std::vector<rgbf>& lightMap;

    tthread::condition_variable writesDone;
    int writeCount;

    lightThreadDispatch(lightingEngineViewscreen* p);
    ~lightThreadDispatch();
    void signalDoneOcclusion();
    void shutdown();
    void waitForWrites();

    int getW();
    int getH();
    void start(int count);
};
class lightThread
{
    std::vector<rgbf> canvas;
    lightThreadDispatch& dispatch;
    DFHack::rect2d myRect;
    void work(); //main light calculation function
    void combine(); //combine existing canvas into global lightmap
public:
    tthread::thread *myThread;
    bool isDone; //no mutex, because bool is atomic
    lightThread(lightThreadDispatch& dispatch);
    ~lightThread();
    void run();
private:
    void doLight(int x,int y);
    void doRay(rgbf power,int cx,int cy,int tx,int ty);
    rgbf lightUpCell(rgbf power,int dx,int dy,int tx,int ty);
};
class lightingEngineViewscreen:public lightingEngine
{
public:
    lightingEngineViewscreen(renderer_light* target);
	~lightingEngineViewscreen();
    void reinit();
    void calculate();

    void updateWindow();

    void loadSettings();
    void clear();

    void debug(bool enable){doDebug=enable;};
private:

    df::coord2d worldToViewportCoord(const df::coord2d& in,const DFHack::rect2d& r,const df::coord2d& window2d) ;
    

    void doSun(const lightSource& sky,MapExtras::MapCache& map);
    void doOcupancyAndLights();
    rgbf propogateSun(MapExtras::Block* b, int x,int y,const rgbf& in,bool lastLevel);
    void doRay(std::vector<rgbf> & target, rgbf power,int cx,int cy,int tx,int ty);
    void doFovs();
	void doLight(std::vector<rgbf> & target, int index);
    rgbf lightUpCell(std::vector<rgbf> & target, rgbf power,int dx,int dy,int tx,int ty);
    bool addLight(int tileId,const lightSource& light);
    void addOclusion(int tileId,const rgbf& c,float thickness);

    matLightDef* getMaterial(int matType,int matIndex);
    buildingLightDef* getBuilding(df::building* bld);
    
    //apply material to cell
    void applyMaterial(int tileId,const matLightDef& mat,float size=1, float thickness = 1);
    //try to find and apply material, if failed return false, and if def!=null then apply def.
    bool applyMaterial(int tileId,int matType,int matIndex,float size=1,float thickness = 1,const matLightDef* def=NULL);
    
    size_t inline getIndex(int x,int y)
    {
        return x*h+y;
    }
    df::coord2d inline getCoords(int index)
    {
        return df::coord2d(index/h, index%h);
    }
    //maps
    std::vector<rgbf> lightMap;
    std::vector<rgbf> ocupancy;
    std::vector<lightSource> lights;

    //Threading stuff
    lightThreadDispatch threading;
    //misc
    void setHour(float h){dayHour=h;};

    int getW()const {return w;}
    int getH()const {return h;}
public:
	void lightWorkerThread(void * arg);
private:
    rgbf getSkyColor(float v);
    bool doDebug;

    //settings
    float daySpeed;
    float dayHour; //<0 to cycle
    std::vector<rgbf> dayColors; // a gradient of colors, first to 0, last to 24
    ///set up sane settings if setting file does not exist.
    void defaultSettings(); 

    static int parseMaterials(lua_State* L);
    static int parseSpecial(lua_State* L);
    static int parseBuildings(lua_State* L);
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
    std::unordered_map<std::pair<int,int>,matLightDef> matDefs;
    //buildings
    std::unordered_map<std::tuple<int,int,int>,buildingLightDef> buildingDefs;
    int w,h;
    DFHack::rect2d mapPort;
    friend lightThreadDispatch;
};
rgbf blend(rgbf a,rgbf b);
rgbf blendMax(rgbf a,rgbf b);
#endif
