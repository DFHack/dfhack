#pragma once

#include "Error.h"
#include "Export.h"
#include "ColorText.h"

#include <vector>

struct SDL_Surface;
struct SDL_Rect;
struct SDL_PixelFormat;
struct SDL_Window;
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

DFHACK_EXPORT void * lookup_DFSDL_Symbol(const char *name);
DFHACK_EXPORT SDL_Surface * DFIMG_Load(const char *file);
DFHACK_EXPORT SDL_Surface * DFSDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT SDL_Surface * DFSDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT int DFSDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
DFHACK_EXPORT SDL_Surface * DFSDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, uint32_t flags);
DFHACK_EXPORT void DFSDL_FreeSurface(SDL_Surface *surface);
// DFHACK_EXPORT int DFSDL_SemWait(SDL_sem *sem);
// DFHACK_EXPORT int DFSDL_SemPost(SDL_sem *sem);
DFHACK_EXPORT int DFSDL_PushEvent(SDL_Event *event);
DFHACK_EXPORT void DFSDL_free(void *ptr);
DFHACK_EXPORT SDL_PixelFormat* DFSDL_AllocFormat(uint32_t pixel_format);
DFHACK_EXPORT SDL_Surface* DFSDL_CreateRGBSurfaceWithFormat(uint32_t flags, int width, int height, int depth, uint32_t format);
DFHACK_EXPORT int DFSDL_ShowSimpleMessageBox(uint32_t flags, const char *title, const char *message, SDL_Window *window);

// submitted and returned text is UTF-8
// see wrapper functions below for cp-437 variants
DFHACK_EXPORT char * DFSDL_GetClipboardText();
DFHACK_EXPORT int DFSDL_SetClipboardText(const char *text);

}

// System clipboard -- submitted and returned text must be in CP437
DFHACK_EXPORT std::string getClipboardTextCp437();
DFHACK_EXPORT bool setClipboardTextCp437(std::string text);

// interprets 0xa as newline instead of usual CP437 char
DFHACK_EXPORT bool getClipboardTextCp437Multiline(std::vector<std::string> * lines);
DFHACK_EXPORT bool setClipboardTextCp437Multiline(std::string text);

}
