#pragma once

#include "Export.h"
#include "ColorText.h"

struct SDL_Surface;
struct SDL_Rect;
struct SDL_PixelFormat;
union SDL_Event;

namespace DFHack
{
    struct DFTileSurface
    {
        bool paintOver; // draw over original tile?
        SDL_Surface* surface; // from where it should be drawn
        SDL_Rect* rect; // from which coords (NULL to draw whole surface)
        SDL_Rect* dstResize; // if not NULL dst rect will be resized (x/y/w/h will be added to original dst)
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

DFHACK_EXPORT SDL_Surface * DFIMG_Load(const char *file);
DFHACK_EXPORT SDL_Surface * DFSDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT SDL_Surface * DFSDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT int DFSDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
DFHACK_EXPORT SDL_Surface * DFSDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, uint32_t flags);
DFHACK_EXPORT void DFSDL_FreeSurface(SDL_Surface *surface);
// DFHACK_EXPORT int DFSDL_SemWait(SDL_sem *sem);
// DFHACK_EXPORT int DFSDL_SemPost(SDL_sem *sem);
DFHACK_EXPORT int DFSDL_PushEvent(SDL_Event *event);

// submitted and returned text is UTF-8
// see wrapper functions in MiscUtils.h for cp-437 variants
DFHACK_EXPORT std::string DFSDL_GetClipboardText();
DFHACK_EXPORT bool DFSDL_SetClipboardText(const char *text);

}

}
