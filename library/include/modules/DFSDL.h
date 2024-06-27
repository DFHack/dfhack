#pragma once

#include "Export.h"
#include "ColorText.h"
// Fix Clang complaining of missing uint32_t type
#include <cstdint>
#include <vector>

#include <SDL_blendmode.h>
#include <SDL_keycode.h>

struct SDL_Surface;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_PixelFormat;
struct SDL_Window;
struct SDL_Texture;
struct SDL_Point;

union SDL_Event;
typedef int (*SDL_EventFilter)(void* userdata, SDL_Event* event);

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
DFHACK_EXPORT void * lookup_DFIMG_Symbol(const char *name);
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

DFHACK_EXPORT SDL_Surface* DFSDL_ConvertSurfaceFormat(SDL_Surface* surface, uint32_t format, uint32_t flags);
DFHACK_EXPORT SDL_Renderer* DFSDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags);
DFHACK_EXPORT SDL_Surface* DFSDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFHACK_EXPORT SDL_Texture* DFSDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h);
DFHACK_EXPORT SDL_Texture* DFSDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
DFHACK_EXPORT SDL_Window* DFSDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags);
DFHACK_EXPORT void DFSDL_DestroyRenderer(SDL_Renderer* renderer);
DFHACK_EXPORT void DFSDL_DestroyTexture(SDL_Texture* texture);
DFHACK_EXPORT void DFSDL_DestroyWindow(SDL_Window* window);
DFHACK_EXPORT void DFSDL_free(void* mem);
DFHACK_EXPORT void DFSDL_FreeSurface(SDL_Surface* surface);
DFHACK_EXPORT char* DFSDL_GetClipboardText(void);
DFHACK_EXPORT const char* DFSDL_GetError(void);
DFHACK_EXPORT SDL_bool DFSDL_GetEventFilter(SDL_EventFilter* filter, void** userdata);
DFHACK_EXPORT SDL_Keymod DFSDL_GetModState(void);
DFHACK_EXPORT int DFSDL_GetRendererOutputSize(SDL_Renderer* renderer, int* w, int* h);
DFHACK_EXPORT uint32_t DFSDL_GetWindowFlags(SDL_Window* window);
DFHACK_EXPORT uint32_t DFSDL_GetWindowID(SDL_Window* window);
DFHACK_EXPORT void DFSDL_HideWindow(SDL_Window* window);
DFHACK_EXPORT char* DFSDL_iconv_string(const char* tocode, const char* fromcode, const char* inbuf, size_t inbytesleft);
DFHACK_EXPORT int DFSDL_InitSubSystem(uint32_t flags);
DFHACK_EXPORT uint32_t DFSDL_MapRGB(const SDL_PixelFormat* format, uint8_t r, uint8_t g, uint8_t b);
DFHACK_EXPORT void* DFSDL_memset(void* dst, int c, size_t len);
DFHACK_EXPORT int DFSDL_RenderClear(SDL_Renderer* renderer);
DFHACK_EXPORT int DFSDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
DFHACK_EXPORT int DFSDL_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect);
DFHACK_EXPORT int DFSDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect);
DFHACK_EXPORT void DFSDL_RenderPresent(SDL_Renderer* renderer);
DFHACK_EXPORT int DFSDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable);
DFHACK_EXPORT int DFSDL_RenderSetViewport(SDL_Renderer* renderer, const SDL_Rect* rect);
DFHACK_EXPORT SDL_bool DFSDL_PointInRect(const SDL_Point* p, const SDL_Rect* r);
DFHACK_EXPORT int DFSDL_SetClipboardText(const char* text);
DFHACK_EXPORT int DFSDL_SetColorKey(SDL_Surface* surface, int flag, uint32_t key);
DFHACK_EXPORT void DFSDL_SetEventFilter(SDL_EventFilter filter, void* userdata);
DFHACK_EXPORT SDL_bool DFSDL_SetHint(const char* name, const char* value);
DFHACK_EXPORT int DFSDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
DFHACK_EXPORT int DFSDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode);
DFHACK_EXPORT int DFSDL_SetTextureColorMod(SDL_Texture* texture, uint8_t r, uint8_t g, uint8_t b);
DFHACK_EXPORT void DFSDL_SetWindowMinimumSize(SDL_Window* window, int min_w, int min_h);
DFHACK_EXPORT void DFSDL_ShowWindow(SDL_Window* window);
DFHACK_EXPORT void DFSDL_StartTextInput(void);
DFHACK_EXPORT void DFSDL_StopTextInput(void);
DFHACK_EXPORT int DFSDL_UpperBlit(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect);
DFHACK_EXPORT int DFSDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
DFHACK_EXPORT void DFSDL_QuitSubSystem(uint32_t flags);

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
