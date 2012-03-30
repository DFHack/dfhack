// This tool display dfhack version on df screen

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <string.h>
using namespace std;
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Graphic.h"
#include "modules/Gui.h"
using namespace DFHack;

command_result df_versionosd (color_ostream &out, vector <string> & parameters);
static DFSDL_Surface* (*_IMG_LoadPNG_RW)(void* src) = 0;
static vPtr (*_SDL_RWFromFile)(const char* file, const char *mode) = 0;
static int (*_SDL_SetAlpha)(vPtr surface, uint32_t flag, uint8_t alpha) = 0;
static int (*_SDL_SetColorKey)(vPtr surface, uint32_t flag, uint32_t key) = 0;
static uint32_t (*_SDL_MapRGB)(vPtr pixelformat, uint8_t r, uint8_t g, uint8_t b) = 0;
DFTileSurface* gettile(int x, int y);

bool On = true;
DFSDL_Surface* surface;
DFTileSurface* tiles[10];
char* file = "Cooz_curses_square_16x16.png";
Gui* gui;

DFHACK_PLUGIN("versionosd");

DFTileSurface* createTile(int x, int y)
{
    DFTileSurface* tile = new DFTileSurface;
    tile->paintOver = true;
    tile->rect = new DFSDL_Rect;
    tile->rect->x = x*16;
    tile->rect->y = y*16;
    tile->rect->w = 16;
    tile->rect->h = 16;
    tile->surface = surface;
    tile->dstResize = NULL;
    return tile;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("versionosd",
                                     "Toggles displaying version in DF window",
                                     df_versionosd));

    HMODULE SDLImageLib = LoadLibrary("SDL_image.dll");
    _IMG_LoadPNG_RW = (DFHack::DFSDL_Surface* (*)(void*))GetProcAddress(SDLImageLib, "IMG_LoadPNG_RW");

    HMODULE realSDLlib = LoadLibrary("SDLreal.dll");
    _SDL_RWFromFile = (void*(*)(const char*, const char*))GetProcAddress(realSDLlib,"SDL_RWFromFile");
    _SDL_SetAlpha = (int (*)(void*, uint32_t, uint8_t))GetProcAddress(realSDLlib,"SDL_SetAlpha");
    _SDL_SetColorKey = (int (*)(void*, uint32_t, uint32_t))GetProcAddress(realSDLlib,"SDL_SetColorKey");
    _SDL_MapRGB = (uint32_t (*)(void*, uint8_t, uint8_t, uint8_t))GetProcAddress(realSDLlib,"SDL_MapRGB");

    void* RWop = _SDL_RWFromFile(file, "rb");
    surface = _IMG_LoadPNG_RW(RWop);

    if ( !surface )
    {
        out.print("Couldnt load image from file %s", file);
        return CR_FAILURE;
    }

    UINT32 pink = _SDL_MapRGB(vPtr(surface->format), 0xff, 0x00, 0xff);

    _SDL_SetColorKey((vPtr)surface, 4096, pink);
    _SDL_SetAlpha((vPtr)surface, 65536, 255);


    // setup tiles
    tiles[0] = createTile(4, 4); // D
    tiles[1] = createTile(6, 4); // F
    tiles[2] = createTile(8, 4); // H
    tiles[3] = createTile(1, 6); // a
    tiles[4] = createTile(3, 6); // c
    tiles[5] = createTile(11, 6); // k
    tiles[6] = createTile(0, 0); // " "
    // FIXME: it should get REAL version not hardcoded one
    tiles[7] = createTile(2, 7); // r
    tiles[8] = createTile(8, 3); // 8

    tiles[9] = createTile(9, 0); // o

    gui = c->getGui();


    Graphic* g = c->getGraphic();
    g->Register(gettile);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    Graphic* g = c->getGraphic();
    g->Unregister(gettile);
    delete surface;

    for (int i=0; i<10; i++)
    {
        delete tiles[i];
    }
    delete [] tiles;

    return CR_OK;
}

command_result df_versionosd (color_ostream &out, vector <string> & parameters)
{
    On = !On;
    CoreSuspender suspend;
    out.print("Version OSD is %s\n", On ? "On" : "Off");
    return CR_OK;
}

DFTileSurface* gettile (int x, int y)
{
    if ( !On ) return NULL;

    if ( x == 0 && y-4 >= 0 && y-4 < 9 )
    {
        return tiles[y-4];
    }

    int32_t cx, cy, cz;
    int32_t vx, vy, vz;
    if ( !gui->getViewCoords(vx, vy, vz) ) return NULL;
    if ( !gui->getCursorCoords(cx, cy, cz) ) return NULL;
    if ( cx-vx+1 == x && cy-vy+1 == y )
    {
        return tiles[9];
    }

    return NULL;
}
