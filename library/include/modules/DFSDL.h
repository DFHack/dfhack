#pragma once

#include "Export.h"
#include "ColorText.h"

namespace DFHack
{
    // SDL stand-in type definitions
    typedef signed short SINT16;
    typedef void DFSDL_sem;

    typedef struct
    {
        int16_t x, y;
        uint16_t w, h;
    } DFSDL_Rect;
    typedef struct
    {
        void *palette; // SDL_Palette*
        uint8_t  BitsPerPixel;
        uint8_t  BytesPerPixel;
        uint8_t  Rloss;
        uint8_t  Gloss;
        uint8_t  Bloss;
        uint8_t  Aloss;
        uint8_t  Rshift;
        uint8_t  Gshift;
        uint8_t  Bshift;
        uint8_t  Ashift;
        uint32_t Rmask;
        uint32_t Gmask;
        uint32_t Bmask;
        uint32_t Amask;
        uint32_t colorkey;
        uint8_t  alpha;
    } DFSDL_PixelFormat;
    typedef struct
    {
        uint32_t flags;
        DFSDL_PixelFormat* format;
        int w, h;
        int pitch;
        void* pixels;
        void* userdata; // as far as i could see DF doesnt use this
        int locked;
        void* lock_data;
        DFSDL_Rect clip_rect;
        void* map;
        int refcount;
    } DFSDL_Surface;

    // =========
    struct DFTileSurface
    {
        bool paintOver; // draw over original tile?
        DFSDL_Surface* surface; // from where it should be drawn
        DFSDL_Rect* rect; // from which coords (NULL to draw whole surface)
        DFSDL_Rect* dstResize; // if not NULL dst rect will be resized (x/y/w/h will be added to original dst)
    };

/**
 * The DFSDL module - provides access to SDL functions without actually
 * requiring build-time linkage to SDL
 * \ingroup grp_modules
 * \ingroup grp_dfsdl
 */
namespace DFSDL
{

/**
 * Call this on DFHack init so we can load the SDL functions. Returns false on
 * failure.
 */
bool init(DFHack::color_ostream &out);

/**
 * Call this when DFHack is being unloaded.
 */
void cleanup();

DFHACK_EXPORT DFSDL_Surface * DFIMG_Load(const char *file);
DFHACK_EXPORT int DFSDL_SetAlpha(DFSDL_Surface *surface, uint32_t flag, uint8_t alpha);
DFHACK_EXPORT DFSDL_Surface * DFSDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT int DFSDL_UpperBlit(DFSDL_Surface *src, const DFSDL_Rect *srcrect, DFSDL_Surface *dst, DFSDL_Rect *dstrect);
DFHACK_EXPORT DFSDL_Surface * DFSDL_ConvertSurface(DFSDL_Surface *src, const DFSDL_PixelFormat *fmt, uint32_t flags);
DFHACK_EXPORT void DFSDL_FreeSurface(DFSDL_Surface *surface);
DFHACK_EXPORT int DFSDL_SemWait(DFSDL_sem *sem);
DFHACK_EXPORT int DFSDL_SemPost(DFSDL_sem *sem);

}

}
