#include "Internal.h"

#include "modules/DFSDL.h"
#include "modules/Textures.h"

#include "Debug.h"
#include "PluginManager.h"

#include "df/enabler.h"

using df::global::enabler;
using namespace DFHack;
using namespace DFHack::DFSDL;

namespace DFHack {
    DBG_DECLARE(core, textures, DebugCategory::LINFO);
}

static bool g_loaded = false;
static long g_num_dfhack_textures = 0;
static long g_dfhack_logo_texpos_start = -1;
static long g_green_pin_texpos_start = -1;
static long g_red_pin_texpos_start = -1;
static long g_icons_texpos_start = -1;
static long g_on_off_texpos_start = -1;
static long g_control_panel_texpos_start = -1;
static long g_thin_borders_texpos_start = -1;
static long g_medium_borders_texpos_start = -1;
static long g_panel_borders_texpos_start = -1;
static long g_window_borders_texpos_start = -1;

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

  DFSDL_Surface *tgt = DFSDL_ConvertSurface(src, &fmt, 0); // SDL_SWSURFACE
  DFSDL_FreeSurface(src);
  return tgt;
}

const uint32_t TILE_WIDTH_PX = 8;
const uint32_t TILE_HEIGHT_PX = 12;

static size_t load_textures(color_ostream & out, const char * fname,
                            long *texpos_start) {
    DFSDL_Surface *s = DFIMG_Load(fname);
    if (!s) {
        out.printerr("unable to load textures from '%s'\n", fname);
        return 0;
    }

    s = canonicalize_format(s);
    DFSDL_SetAlpha(s, 0, 255);
    int dimx = s->w / TILE_WIDTH_PX;
    int dimy = s->h / TILE_HEIGHT_PX;
    long count = 0;
    for (int y = 0; y < dimy; y++) {
        for (int x = 0; x < dimx; x++) {
            DFSDL_Surface *tile = DFSDL_CreateRGBSurface(0, // SDL_SWSURFACE
                    TILE_WIDTH_PX, TILE_HEIGHT_PX, 32,
                    s->format->Rmask, s->format->Gmask, s->format->Bmask,
                    s->format->Amask);
            DFSDL_SetAlpha(tile, 0,255);
            DFSDL_Rect vp;
            vp.x = TILE_WIDTH_PX * x;
            vp.y = TILE_HEIGHT_PX * y;
            vp.w = TILE_WIDTH_PX;
            vp.h = TILE_HEIGHT_PX;
            DFSDL_UpperBlit(s, &vp, tile, NULL);
            if (!count++)
                *texpos_start = enabler->textures.raws.size();
            enabler->textures.raws.push_back(tile);
        }
    }
    DFSDL_FreeSurface(s);

    DEBUG(textures,out).print("loaded %ld textures from '%s'\n", count, fname);
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

    bool is_pre_world = num_textures == textures.init_texture_size;

    g_num_dfhack_textures = load_textures(out, "hack/data/art/dfhack.png",
                                          &g_dfhack_logo_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/green-pin.png",
                                          &g_green_pin_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/red-pin.png",
                                          &g_red_pin_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/icons.png",
                                          &g_icons_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/on-off.png",
                                          &g_on_off_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/control-panel.png",
                                          &g_control_panel_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/border-thin.png",
                                          &g_thin_borders_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/border-medium.png",
                                          &g_medium_borders_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/border-panel.png",
                                          &g_panel_borders_texpos_start);
    g_num_dfhack_textures += load_textures(out, "hack/data/art/border-window.png",
                                          &g_window_borders_texpos_start);

    DEBUG(textures,out).print("loaded %ld textures\n", g_num_dfhack_textures);

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
        DFSDL_FreeSurface((DFSDL_Surface *)raws[idx]);
        raws[idx] = NULL;
    }

    if (g_dfhack_logo_texpos_start == textures.init_texture_size - g_num_dfhack_textures)
        textures.init_texture_size -= g_num_dfhack_textures;

    g_loaded = false;
    g_num_dfhack_textures = 0;
    g_dfhack_logo_texpos_start = -1;
}

long Textures::getDfhackLogoTexposStart() {
    return g_dfhack_logo_texpos_start;
}

long Textures::getGreenPinTexposStart() {
    return g_green_pin_texpos_start;
}

long Textures::getRedPinTexposStart() {
    return g_red_pin_texpos_start;
}

long Textures::getIconsTexposStart() {
    return g_icons_texpos_start;
}

long Textures::getOnOffTexposStart() {
    return g_on_off_texpos_start;
}

long Textures::getControlPanelTexposStart() {
    return g_control_panel_texpos_start;
}

long Textures::getThinBordersTexposStart() {
    return g_thin_borders_texpos_start;
}

long Textures::getMediumBordersTexposStart() {
    return g_medium_borders_texpos_start;
}

long Textures::getPanelBordersTexposStart() {
    return g_panel_borders_texpos_start;
}

long Textures::getWindowBordersTexposStart() {
    return g_window_borders_texpos_start;
}
