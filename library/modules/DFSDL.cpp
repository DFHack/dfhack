#include "modules/DFSDL.h"

#include "Debug.h"
#include "PluginManager.h"

#include <SDL_clipboard.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_messagebox.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#ifdef WIN32
# include <regex>
#endif

namespace DFHack {
    DBG_DECLARE(core, dfsdl, DebugCategory::LINFO);
}

using namespace DFHack;
using std::string;
using std::vector;

static DFLibrary *g_sdl_handle = nullptr;
static DFLibrary *g_sdl_image_handle = nullptr;
static const vector<string> SDL_LIBS {
    "SDL2.dll",
    "SDL.framework/Versions/A/SDL",
    "SDL.framework/SDL",
    "libSDL2-2.0.so.0"
};
static const vector<string> SDL_IMAGE_LIBS {
    "SDL2_image.dll",
    "SDL_image.framework/Versions/A/SDL_image",
    "SDL_image.framework/SDL_image",
    "libSDL2_image-2.0.so.0"
};

SDL_Surface * (*g_IMG_Load)(const char *) = nullptr;
// int (*g_SDL_SemWait)(DFSDL_sem *) = nullptr;
// int (*g_SDL_SemPost)(DFSDL_sem *) = nullptr;

// These two pull in SDL.h
int SDL_InitSubSystem(Uint32 flags);
void SDL_QuitSubSystem(Uint32 flags);

#define DFSDL_DECLARE_SYMBOL(sym) decltype(sym)* g_##sym = nullptr

DFSDL_DECLARE_SYMBOL(SDL_AllocFormat);
DFSDL_DECLARE_SYMBOL(SDL_ConvertSurface);
DFSDL_DECLARE_SYMBOL(SDL_ConvertSurfaceFormat);
DFSDL_DECLARE_SYMBOL(SDL_CreateRGBSurface);
DFSDL_DECLARE_SYMBOL(SDL_CreateRGBSurfaceFrom);
DFSDL_DECLARE_SYMBOL(SDL_CreateRGBSurfaceWithFormat);
DFSDL_DECLARE_SYMBOL(SDL_CreateRenderer);
DFSDL_DECLARE_SYMBOL(SDL_CreateTexture);
DFSDL_DECLARE_SYMBOL(SDL_CreateTextureFromSurface);
DFSDL_DECLARE_SYMBOL(SDL_CreateWindow);
DFSDL_DECLARE_SYMBOL(SDL_DestroyRenderer);
DFSDL_DECLARE_SYMBOL(SDL_DestroyTexture);
DFSDL_DECLARE_SYMBOL(SDL_DestroyWindow);
DFSDL_DECLARE_SYMBOL(SDL_free);
DFSDL_DECLARE_SYMBOL(SDL_FreeSurface);
DFSDL_DECLARE_SYMBOL(SDL_GetClipboardText);
DFSDL_DECLARE_SYMBOL(SDL_GetError);
DFSDL_DECLARE_SYMBOL(SDL_GetEventFilter);
DFSDL_DECLARE_SYMBOL(SDL_GetModState);
DFSDL_DECLARE_SYMBOL(SDL_GetRendererOutputSize);
DFSDL_DECLARE_SYMBOL(SDL_GetWindowFlags);
DFSDL_DECLARE_SYMBOL(SDL_GetWindowID);
DFSDL_DECLARE_SYMBOL(SDL_HasClipboardText);
DFSDL_DECLARE_SYMBOL(SDL_HideWindow);
DFSDL_DECLARE_SYMBOL(SDL_iconv_string);
DFSDL_DECLARE_SYMBOL(SDL_InitSubSystem);
DFSDL_DECLARE_SYMBOL(SDL_MapRGB);
DFSDL_DECLARE_SYMBOL(SDL_memset);
DFSDL_DECLARE_SYMBOL(SDL_RenderClear);
DFSDL_DECLARE_SYMBOL(SDL_RenderCopy);
DFSDL_DECLARE_SYMBOL(SDL_RenderDrawRect);
DFSDL_DECLARE_SYMBOL(SDL_RenderFillRect);
DFSDL_DECLARE_SYMBOL(SDL_RenderPresent);
DFSDL_DECLARE_SYMBOL(SDL_RenderSetIntegerScale);
DFSDL_DECLARE_SYMBOL(SDL_RenderSetViewport);
//DFSDL_DECLARE_SYMBOL(SDL_PointInRect); defined in SDL_rect.h
DFSDL_DECLARE_SYMBOL(SDL_PushEvent);
DFSDL_DECLARE_SYMBOL(SDL_SetClipboardText);
DFSDL_DECLARE_SYMBOL(SDL_SetColorKey);
DFSDL_DECLARE_SYMBOL(SDL_SetEventFilter);
DFSDL_DECLARE_SYMBOL(SDL_SetHint);
DFSDL_DECLARE_SYMBOL(SDL_SetRenderDrawColor);
DFSDL_DECLARE_SYMBOL(SDL_SetTextureBlendMode);
DFSDL_DECLARE_SYMBOL(SDL_SetTextureColorMod);
DFSDL_DECLARE_SYMBOL(SDL_SetWindowMinimumSize);
DFSDL_DECLARE_SYMBOL(SDL_ShowSimpleMessageBox);
DFSDL_DECLARE_SYMBOL(SDL_ShowWindow);
DFSDL_DECLARE_SYMBOL(SDL_StartTextInput);
DFSDL_DECLARE_SYMBOL(SDL_StopTextInput);
DFSDL_DECLARE_SYMBOL(SDL_UpperBlit);
DFSDL_DECLARE_SYMBOL(SDL_UpdateTexture);
DFSDL_DECLARE_SYMBOL(SDL_QuitSubSystem);

bool DFSDL::init(color_ostream &out) {
    for (auto &lib_str : SDL_LIBS) {
        if ((g_sdl_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_sdl_handle) {
        out.printerr("DFHack could not find SDL\n");
        return false;
    }

    for (auto &lib_str : SDL_IMAGE_LIBS) {
        if ((g_sdl_image_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_sdl_image_handle) {
        out.printerr("DFHack could not find SDL_image\n");
        return false;
    }

    #define bind(handle, name) \
        g_##name = (decltype(g_##name))LookupPlugin(handle, #name); \
        if (!g_##name) { \
            out.printerr("DFHack could not find: " #name "\n"); \
            return false; \
        }

    bind(g_sdl_image_handle, IMG_Load);
    // bind(g_sdl_handle, SDL_SemWait);
    // bind(g_sdl_handle, SDL_SemPost);

    bind(g_sdl_handle, SDL_AllocFormat);
    bind(g_sdl_handle, SDL_ConvertSurface);
    bind(g_sdl_handle, SDL_ConvertSurfaceFormat);
    bind(g_sdl_handle, SDL_CreateRenderer);
    bind(g_sdl_handle, SDL_CreateRGBSurface);
    bind(g_sdl_handle, SDL_CreateRGBSurfaceFrom);
    bind(g_sdl_handle, SDL_CreateRGBSurfaceWithFormat);
    bind(g_sdl_handle, SDL_CreateTexture);
    bind(g_sdl_handle, SDL_CreateTextureFromSurface);
    bind(g_sdl_handle, SDL_CreateWindow);
    bind(g_sdl_handle, SDL_DestroyRenderer);
    bind(g_sdl_handle, SDL_DestroyTexture);
    bind(g_sdl_handle, SDL_DestroyWindow);
    bind(g_sdl_handle, SDL_free);
    bind(g_sdl_handle, SDL_FreeSurface);
    bind(g_sdl_handle, SDL_GetClipboardText);
    bind(g_sdl_handle, SDL_GetError);
    bind(g_sdl_handle, SDL_GetEventFilter);
    bind(g_sdl_handle, SDL_GetModState);
    bind(g_sdl_handle, SDL_GetRendererOutputSize);
    bind(g_sdl_handle, SDL_GetWindowFlags);
    bind(g_sdl_handle, SDL_GetWindowID);
    bind(g_sdl_handle, SDL_HasClipboardText);
    bind(g_sdl_handle, SDL_HideWindow);
    bind(g_sdl_handle, SDL_iconv_string);
    bind(g_sdl_handle, SDL_InitSubSystem);
    bind(g_sdl_handle, SDL_MapRGB);
    bind(g_sdl_handle, SDL_memset);
    //bind(g_sdl_handle, SDL_PointInRect); defined in SDL_rect.h
    bind(g_sdl_handle, SDL_PushEvent);
    bind(g_sdl_handle, SDL_QuitSubSystem);
    bind(g_sdl_handle, SDL_RenderClear);
    bind(g_sdl_handle, SDL_RenderCopy);
    bind(g_sdl_handle, SDL_RenderDrawRect);
    bind(g_sdl_handle, SDL_RenderFillRect);
    bind(g_sdl_handle, SDL_RenderPresent);
    bind(g_sdl_handle, SDL_RenderSetIntegerScale);
    bind(g_sdl_handle, SDL_RenderSetViewport);
    bind(g_sdl_handle, SDL_SetClipboardText);
    bind(g_sdl_handle, SDL_SetColorKey);
    bind(g_sdl_handle, SDL_SetEventFilter);
    bind(g_sdl_handle, SDL_SetHint);
    bind(g_sdl_handle, SDL_SetRenderDrawColor);
    bind(g_sdl_handle, SDL_SetTextureBlendMode);
    bind(g_sdl_handle, SDL_SetTextureColorMod);
    bind(g_sdl_handle, SDL_SetWindowMinimumSize);
    bind(g_sdl_handle, SDL_ShowSimpleMessageBox);
    bind(g_sdl_handle, SDL_ShowWindow);
    bind(g_sdl_handle, SDL_StartTextInput);
    bind(g_sdl_handle, SDL_StopTextInput);
    bind(g_sdl_handle, SDL_UpperBlit);
    bind(g_sdl_handle, SDL_UpdateTexture);

#undef bind

    DEBUG(dfsdl,out).print("sdl successfully loaded\n");
    return true;
}

// It's ok to leave NULLs in the raws list (according to usage in g_src)
void DFSDL::cleanup() {
    if (g_sdl_handle) {
        ClosePlugin(g_sdl_handle);
        g_sdl_handle = nullptr;
    }
    if (g_sdl_image_handle) {
        ClosePlugin(g_sdl_image_handle);
        g_sdl_image_handle = nullptr;
    }
}

void * DFSDL::lookup_DFSDL_Symbol(const char *name) {
    if (!g_sdl_handle)
        return nullptr;
    return LookupPlugin(g_sdl_handle, name);
}

void * DFSDL::lookup_DFIMG_Symbol(const char *name) {
    if (!g_sdl_image_handle)
        return nullptr;
    return LookupPlugin(g_sdl_image_handle, name);
}

SDL_Surface * DFSDL::DFIMG_Load(const char *file) {
    return g_IMG_Load(file);
}

SDL_Surface * DFSDL::DFSDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
    return g_SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

SDL_Surface * DFSDL::DFSDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
    return g_SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);
}

int DFSDL::DFSDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
    return g_SDL_UpperBlit(src, srcrect, dst, dstrect);
}

SDL_Surface * DFSDL::DFSDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, uint32_t flags) {
    return g_SDL_ConvertSurface(src, fmt, flags);
}

void DFSDL::DFSDL_FreeSurface(SDL_Surface *surface) {
    g_SDL_FreeSurface(surface);
}

// int DFSDL::DFSDL_SemWait(DFSDL_sem *sem) {
//     return g_SDL_SemWait(sem);
// }

// int DFSDL::DFSDL_SemPost(DFSDL_sem *sem) {
//     return g_SDL_SemPost(sem);
// }

int DFSDL::DFSDL_PushEvent(SDL_Event *event) {
    return g_SDL_PushEvent(event);
}

void DFSDL::DFSDL_free(void *ptr) {
    g_SDL_free(ptr);
}

char * DFSDL::DFSDL_GetClipboardText() {
    return g_SDL_GetClipboardText();
}

int DFSDL::DFSDL_SetClipboardText(const char *text) {
    return g_SDL_SetClipboardText(text);
}

SDL_PixelFormat* DFSDL::DFSDL_AllocFormat(uint32_t pixel_format) {
    return g_SDL_AllocFormat(pixel_format);
}

SDL_Surface* DFSDL::DFSDL_CreateRGBSurfaceWithFormat(uint32_t flags, int width, int height, int depth, uint32_t format) {
    return g_SDL_CreateRGBSurfaceWithFormat(flags, width, height, depth, format);
}

int DFSDL::DFSDL_ShowSimpleMessageBox(uint32_t flags, const char *title, const char *message, SDL_Window *window) {
    if (!g_SDL_ShowSimpleMessageBox)
        return -1;
    return g_SDL_ShowSimpleMessageBox(flags, title, message, window);
}

// convert tabs to spaces so they don't get converted to '?'
static char * tabs_to_spaces(char *str) {
    for (char *c = str; *c; ++c) {
        if (*c == '\t')
            *c = ' ';
    }
    return str;
}

static string normalize_newlines(const string & str, bool to_spaces = false) {
    string normalized_str = str;
#ifdef WIN32
    static const std::regex CRLF("\r\n");
    normalized_str = std::regex_replace(normalized_str, CRLF, "\n");
#endif
    if (to_spaces)
        std::replace(normalized_str.begin(), normalized_str.end(), '\n', ' ');

    return normalized_str;
}

DFHACK_EXPORT string DFHack::getClipboardTextCp437() {
    if (!g_sdl_handle || g_SDL_HasClipboardText() != SDL_TRUE)
        return "";
    char *text = tabs_to_spaces(g_SDL_GetClipboardText());
    string textcp437 = UTF2DF(normalize_newlines(text, true));
    DFHack::DFSDL::DFSDL_free(text);
    return textcp437;
}

DFHACK_EXPORT bool DFHack::getClipboardTextCp437Multiline(vector<string> * lines) {
    CHECK_NULL_POINTER(lines);

    if (!g_sdl_handle || g_SDL_HasClipboardText() != SDL_TRUE)
        return false;
    char *text = tabs_to_spaces(g_SDL_GetClipboardText());
    vector<string> utf8_lines;
    split_string(&utf8_lines, normalize_newlines(text), "\n");
    DFHack::DFSDL::DFSDL_free(text);

    for (auto utf8_line : utf8_lines)
        lines->emplace_back(UTF2DF(utf8_line));

    return true;
}

DFHACK_EXPORT bool DFHack::setClipboardTextCp437(string text) {
    if (!g_sdl_handle)
        return false;
    return 0 == DFHack::DFSDL::DFSDL_SetClipboardText(DF2UTF(text).c_str());
}

DFHACK_EXPORT bool DFHack::setClipboardTextCp437Multiline(string text) {
    if (!g_sdl_handle)
        return false;
    vector<string> lines;
    split_string(&lines, text, "\n");
    std::ostringstream str;
    for (size_t idx = 0; idx < lines.size(); ++idx) {
        str << DF2UTF(lines[idx]);
        if (idx < lines.size() - 1) {
#ifdef WIN32
            str << "\r\n";
#else
            str << "\n";
#endif
        }
    }
    return 0 == DFHack::DFSDL::DFSDL_SetClipboardText(str.str().c_str());
}

SDL_Surface* DFSDL::DFSDL_ConvertSurfaceFormat(SDL_Surface* surface, uint32_t format, uint32_t flags) {
    return g_SDL_ConvertSurfaceFormat(surface, format, flags);
}

SDL_Renderer* DFSDL::DFSDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags) {
    return g_SDL_CreateRenderer(window, index, flags);
}

SDL_Texture* DFSDL::DFSDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h) {
    return g_SDL_CreateTexture(renderer, format, access, w, h);
}

SDL_Texture* DFSDL::DFSDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    return g_SDL_CreateTextureFromSurface(renderer, surface);
}

SDL_Window* DFSDL::DFSDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags) {
    return g_SDL_CreateWindow(title, x, y, w, h, flags);
}

void DFSDL::DFSDL_DestroyRenderer(SDL_Renderer* renderer) {
    g_SDL_DestroyRenderer(renderer);
}

void DFSDL::DFSDL_DestroyTexture(SDL_Texture* texture) {
    g_SDL_DestroyTexture(texture);
}

void DFSDL::DFSDL_DestroyWindow(SDL_Window* window) {
    g_SDL_DestroyWindow(window);
}

const char* DFSDL::DFSDL_GetError(void) {
    return g_SDL_GetError();
}

SDL_bool DFSDL::DFSDL_GetEventFilter(SDL_EventFilter* filter, void** userdata) {
    return g_SDL_GetEventFilter(filter, userdata);
}

SDL_Keymod DFSDL::DFSDL_GetModState(void) {
    return g_SDL_GetModState();
}

int DFSDL::DFSDL_GetRendererOutputSize(SDL_Renderer* renderer, int* w, int* h) {
    return g_SDL_GetRendererOutputSize(renderer, w, h);
}

uint32_t DFSDL::DFSDL_GetWindowFlags(SDL_Window* window) {
    return g_SDL_GetWindowFlags(window);
}

uint32_t DFSDL::DFSDL_GetWindowID(SDL_Window* window) {
    return g_SDL_GetWindowID(window);
}

void DFSDL::DFSDL_HideWindow(SDL_Window* window) {
    g_SDL_HideWindow(window);
}

char* DFSDL::DFSDL_iconv_string(const char* tocode, const char* fromcode, const char* inbuf, size_t inbytesleft) {
    return g_SDL_iconv_string(tocode, fromcode, inbuf, inbytesleft);
}

int DFSDL::DFSDL_InitSubSystem(uint32_t flags) {
    return g_SDL_InitSubSystem(flags);
}

uint32_t DFSDL::DFSDL_MapRGB(const SDL_PixelFormat* format, uint8_t r, uint8_t g, uint8_t b) {
    return g_SDL_MapRGB(format, r, g, b);
}

void* DFSDL::DFSDL_memset(void* dst, int c, size_t len) {
    return g_SDL_memset(dst, c, len);
}

int DFSDL::DFSDL_RenderClear(SDL_Renderer* renderer) {
    return g_SDL_RenderClear(renderer);
}

int DFSDL::DFSDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) {
    return g_SDL_RenderCopy(renderer, texture, srcrect, dstrect);
}

int DFSDL::DFSDL_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect) {
    return g_SDL_RenderDrawRect(renderer, rect);
}

int DFSDL::DFSDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect) {
    return g_SDL_RenderFillRect(renderer, rect);
}

void DFSDL::DFSDL_RenderPresent(SDL_Renderer* renderer) {
    g_SDL_RenderPresent(renderer);
}

int DFSDL::DFSDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable) {
    return g_SDL_RenderSetIntegerScale(renderer, enable);
}

int DFSDL::DFSDL_RenderSetViewport(SDL_Renderer* renderer, const SDL_Rect* rect) {
    return g_SDL_RenderSetViewport(renderer, rect);
}

// Defined in SDL_rect.h
SDL_bool DFSDL::DFSDL_PointInRect(const SDL_Point* p, const SDL_Rect* r) {
    return SDL_PointInRect(p, r);
}

int DFSDL::DFSDL_SetColorKey(SDL_Surface* surface, int flag, uint32_t key) {
    return g_SDL_SetColorKey(surface, flag, key);
}

void DFSDL::DFSDL_SetEventFilter(SDL_EventFilter filter, void* userdata) {
    g_SDL_SetEventFilter(filter, userdata);
}

SDL_bool DFSDL::DFSDL_SetHint(const char* name, const char* value) {
    return g_SDL_SetHint(name, value);
}

int DFSDL::DFSDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return g_SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

int DFSDL::DFSDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode) {
    return g_SDL_SetTextureBlendMode(texture, blendMode);
}

int DFSDL::DFSDL_SetTextureColorMod(SDL_Texture* texture, uint8_t r, uint8_t g, uint8_t b) {
    return g_SDL_SetTextureColorMod(texture, r, g, b);
}

void DFSDL::DFSDL_SetWindowMinimumSize(SDL_Window* window, int min_w, int min_h) {
    g_SDL_SetWindowMinimumSize(window, min_w, min_h);
}

void DFSDL::DFSDL_ShowWindow(SDL_Window* window) {
    g_SDL_ShowWindow(window);
}

void DFSDL::DFSDL_StartTextInput(void) {
    g_SDL_StartTextInput();
}

void DFSDL::DFSDL_StopTextInput(void) {
    g_SDL_StopTextInput();
}

int DFSDL::DFSDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch) {
    return g_SDL_UpdateTexture(texture, rect, pixels, pitch);
}

void DFSDL::DFSDL_QuitSubSystem(uint32_t flags) {
    g_SDL_QuitSubSystem(flags);
}
