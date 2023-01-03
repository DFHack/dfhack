#include "Internal.h"

#include "modules/Textures.h"

#include "Debug.h"
#include "PluginManager.h"

#include "df/enabler.h"

using df::global::enabler;
using namespace DFHack;

namespace DFHack {
    DBG_DECLARE(core, textures, DebugCategory::LINFO);
}

static bool g_loaded = false;
static long g_num_dfhack_textures = 0;
static long g_dfhack_logo_texpos_start = -1;

static DFLibrary *g_sdl_handle = nullptr;
static DFLibrary *g_sdl_image_handle = nullptr;
static const std::vector<std::string> SDL_LIBS {
    "SDLreal.dll",
    "SDL.framework/Versions/A/SDL",
    "SDL.framework/SDL",
    "libSDL-1.2.so.0"
};
static const std::vector<std::string> SDL_IMAGE_LIBS {
    "SDL_image.dll",
    "SDL_image.framework/Versions/A/SDL_image",
    "SDL_image.framework/SDL_image",
    "libSDL_image-1.2.so.0"
};

DFSDL_Surface * (*g_IMG_Load)(const char *) = nullptr;
int (*g_SDL_SetAlpha)(DFSDL_Surface *, uint32_t, uint8_t) = nullptr;
DFSDL_Surface * (*g_SDL_CreateRGBSurface)(uint32_t, int, int, int, uint32_t, uint32_t, uint32_t, uint32_t);
int (*g_SDL_UpperBlit)(DFSDL_Surface *, DFSDL_Rect *, DFSDL_Surface *, DFSDL_Rect *);
DFSDL_Surface * (*g_SDL_ConvertSurface)(DFSDL_Surface *, const DFSDL_PixelFormat *, uint32_t);

void (*g_SDL_FreeSurface)(DFSDL_Surface *);

// Converts an arbitrary Surface to something like the display format
// (32-bit RGBA), and converts magenta to transparency if convert_magenta is set
// and the source surface didn't already have an alpha channel.
// It also deletes the source surface.
//
// It uses the same pixel format (RGBA, R at lowest address) regardless of
// hardware.
DFSDL_Surface * canonicalize_format(DFSDL_Surface *src) {
  DFSDL_PixelFormat fmt;
  fmt.palette = NULL;
  fmt.BitsPerPixel = 32;
  fmt.BytesPerPixel = 4;
  fmt.Rloss = fmt.Gloss = fmt.Bloss = fmt.Aloss = 0;
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//  fmt.Rshift = 24; fmt.Gshift = 16; fmt.Bshift = 8; fmt.Ashift = 0;
//#else
  fmt.Rshift = 0; fmt.Gshift = 8; fmt.Bshift = 16; fmt.Ashift = 24;
//#endif
  fmt.Rmask = 255 << fmt.Rshift;
  fmt.Gmask = 255 << fmt.Gshift;
  fmt.Bmask = 255 << fmt.Bshift;
  fmt.Amask = 255 << fmt.Ashift;
  fmt.colorkey = 0;
  fmt.alpha = 255;

  DFSDL_Surface *tgt = g_SDL_ConvertSurface(src, &fmt, 0); // SDL_SWSURFACE
  g_SDL_FreeSurface(src);
  return tgt;
}

const uint32_t TILE_WIDTH_PX = 8;
const uint32_t TILE_HEIGHT_PX = 12;

static size_t load_textures(color_ostream & out, const char * fname,
                            long *texpos_start) {
    if (!g_sdl_handle || !g_sdl_image_handle || !g_IMG_Load || !g_SDL_SetAlpha
            || !g_SDL_CreateRGBSurface || !g_SDL_UpperBlit
            || !g_SDL_ConvertSurface || !g_SDL_FreeSurface)
        return 0;

    DFSDL_Surface *s = g_IMG_Load(fname);
    if (!s) {
        out.printerr("unable to load textures from '%s'\n", fname);
        return 0;
    }

    s = canonicalize_format(s);
    g_SDL_SetAlpha(s, 0, 255);
    int dimx = s->w / TILE_WIDTH_PX;
    int dimy = s->h / TILE_HEIGHT_PX;
    long count = 0;
    for (int y = 0; y < dimy; y++) {
        for (int x = 0; x < dimx; x++) {
            DFSDL_Surface *tile = g_SDL_CreateRGBSurface(0, // SDL_SWSURFACE
                    TILE_WIDTH_PX, TILE_HEIGHT_PX, 32,
                    s->format->Rmask, s->format->Gmask, s->format->Bmask,
                    s->format->Amask);
            g_SDL_SetAlpha(tile, 0,255);
            DFSDL_Rect vp;
            vp.x = TILE_WIDTH_PX * x;
            vp.y = TILE_HEIGHT_PX * y;
            vp.w = TILE_WIDTH_PX;
            vp.h = TILE_HEIGHT_PX;
            g_SDL_UpperBlit(s, &vp, tile, NULL);
            if (!count++)
                *texpos_start = enabler->textures.raws.size();
            enabler->textures.raws.push_back(tile);
        }
    }
    g_SDL_FreeSurface(s);

    DEBUG(textures,out).print("loaded %zd textures from '%s'\n", count, fname);
    return count;
}

// DFHack could conceivably be loaded at any time, so we need to be able to
// handle loading textures before or after a world is loaded.
// If a world is already loaded, then append our textures to the raws. they'll
// be freed when the world is unloaded and we'll reload when we get to the title
// screen. If it's pre-world, append our textures and then adjust the "init"
// texture count so our textures will no longer be freed when worlds are
// unloaded.
//
void Textures::init(color_ostream &out) {
    auto & textures = enabler->textures;
    long num_textures = textures.raws.size();
    if (num_textures <= g_dfhack_logo_texpos_start)
        g_loaded = false;

    if (g_loaded)
        return;

    for (auto &lib_str : SDL_LIBS) {
        if ((g_sdl_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    for (auto &lib_str : SDL_IMAGE_LIBS) {
        if ((g_sdl_image_handle = OpenPlugin(lib_str.c_str())))
            break;
    }

    if (!g_sdl_handle) {
        out.printerr("Could not find SDL; DFHack textures not loaded.\n");
    } else if (!g_sdl_image_handle) {
        out.printerr("Could not find SDL_image; DFHack textures not loaded.\n");
    } else {
        #define bind(handle, name) \
            g_##name = (decltype(g_##name))LookupPlugin(handle, #name); \
            if (!g_##name) { \
                out.printerr("Could not find: " #name "; DFHack textures not loaded\n"); \
            }

        bind(g_sdl_image_handle, IMG_Load);
        bind(g_sdl_handle, SDL_SetAlpha);
        bind(g_sdl_handle, SDL_CreateRGBSurface);
        bind(g_sdl_handle, SDL_UpperBlit);
        bind(g_sdl_handle, SDL_ConvertSurface);
        bind(g_sdl_handle, SDL_FreeSurface);
        #undef bind
    }

    bool is_pre_world = num_textures == textures.init_texture_size;

    g_num_dfhack_textures = load_textures(out, "hack/data/art/dfhack.png",
                                          &g_dfhack_logo_texpos_start);

    DEBUG(textures,out).print("loaded %zd textures\n", g_num_dfhack_textures);

    if (is_pre_world)
        textures.init_texture_size += g_num_dfhack_textures;

    // NOTE: when GL modes are supported, we'll have to re-upload textures here

    g_loaded = true;
}

// It's ok to leave NULLs in the raws list (according to usage in g_src)
void Textures::cleanup() {
    if (!g_loaded)
        return;

    auto & textures = enabler->textures;
    auto &raws = textures.raws;
    size_t texpos_end = g_dfhack_logo_texpos_start + g_num_dfhack_textures;
    for (size_t idx = g_dfhack_logo_texpos_start; idx <= texpos_end; ++idx) {
        SDL_FreeSurface(raws[idx]);
        raws[idx] = NULL;
    }

    if (g_dfhack_logo_texpos_start == textures.init_texture_size - g_num_dfhack_textures)
        textures.init_texture_size -= g_num_dfhack_textures;

    g_loaded = false;
    g_num_dfhack_textures = 0;
    g_dfhack_logo_texpos_start = -1;

    if (g_sdl_handle) {
        ClosePlugin(g_sdl_handle);
        g_sdl_handle = nullptr;
    }
}

long Textures::getDfhackLogoTexposStart() {
    return g_dfhack_logo_texpos_start;
}
