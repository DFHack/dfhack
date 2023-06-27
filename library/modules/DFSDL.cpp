#include "Internal.h"

#include "modules/DFSDL.h"

#include "Debug.h"
#include "PluginManager.h"

namespace DFHack {
    DBG_DECLARE(core, dfsdl, DebugCategory::LINFO);
}

using namespace DFHack;

static DFLibrary *g_sdl_handle = nullptr;
static DFLibrary *g_sdl_image_handle = nullptr;
static const std::vector<std::string> SDL_LIBS {
    "SDL2.dll",
    "SDL.framework/Versions/A/SDL",
    "SDL.framework/SDL",
    "libSDL2-2.0.so.0"
};
static const std::vector<std::string> SDL_IMAGE_LIBS {
    "SDL2_image.dll",
    "SDL_image.framework/Versions/A/SDL_image",
    "SDL_image.framework/SDL_image",
    "libSDL2_image-2.0.so.0"
};

SDL_Surface * (*g_IMG_Load)(const char *) = nullptr;
SDL_Surface * (*g_SDL_CreateRGBSurface)(uint32_t, int, int, int, uint32_t, uint32_t, uint32_t, uint32_t) = nullptr;
SDL_Surface * (*g_SDL_CreateRGBSurfaceFrom)(void *pixels, int width, int height, int depth, int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) = nullptr;
int (*g_SDL_UpperBlit)(SDL_Surface *, const SDL_Rect *, SDL_Surface *, SDL_Rect *) = nullptr;
SDL_Surface * (*g_SDL_ConvertSurface)(SDL_Surface *, const SDL_PixelFormat *, uint32_t) = nullptr;
void (*g_SDL_FreeSurface)(SDL_Surface *) = nullptr;
// int (*g_SDL_SemWait)(DFSDL_sem *) = nullptr;
// int (*g_SDL_SemPost)(DFSDL_sem *) = nullptr;
int (*g_SDL_PushEvent)(SDL_Event *) = nullptr;

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
    bind(g_sdl_handle, SDL_CreateRGBSurface);
    bind(g_sdl_handle, SDL_CreateRGBSurfaceFrom);
    bind(g_sdl_handle, SDL_UpperBlit);
    bind(g_sdl_handle, SDL_ConvertSurface);
    bind(g_sdl_handle, SDL_FreeSurface);
    // bind(g_sdl_handle, SDL_SemWait);
    // bind(g_sdl_handle, SDL_SemPost);
    bind(g_sdl_handle, SDL_PushEvent);
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
