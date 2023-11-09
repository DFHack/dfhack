#include <algorithm>
#include <atomic>
#include <mutex>
#include <numeric>
#include <unordered_map>

#include "Internal.h"

#include "modules/DFSDL.h"
#include "modules/Textures.h"

#include "Debug.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "df/enabler.h"
#include "df/viewscreen_adopt_regionst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_new_arenast.h"
#include "df/viewscreen_new_regionst.h"

#include <SDL_pixels.h>
#include <SDL_surface.h>

using df::global::enabler;
using namespace DFHack;
using namespace DFHack::DFSDL;

namespace DFHack {
DBG_DECLARE(core, textures, DebugCategory::LINFO);
}

struct ReservedRange {
    void init(int32_t start) {
        this->start = start;
        this->end = start + ReservedRange::size;
        this->current = start;
        this->is_installed = true;
    }
    long get_new_texpos() {
        if (this->current == this->end)
            return -1;
        return this->current++;
    }

    static const int32_t size = 10000; // size of reserved texpos buffer
    int32_t start = -1;
    int32_t end = -1;
    long current = -1;
    bool is_installed = false;
};

static ReservedRange reserved_range{};
static std::unordered_map<TexposHandle, long> g_handle_to_texpos;
static std::unordered_map<TexposHandle, long> g_handle_to_reserved_texpos;
static std::unordered_map<TexposHandle, SDL_Surface*> g_handle_to_surface;
static std::unordered_map<std::string, std::vector<TexposHandle>> g_tileset_to_handles;
static std::vector<TexposHandle> g_delayed_regs;
static std::mutex g_adding_mutex;
static std::atomic<bool> loading_state = false;
static SDL_Surface* dummy_surface = NULL;

// Converts an arbitrary Surface to something like the display format
// (32-bit RGBA), and converts magenta to transparency if convert_magenta is set
// and the source surface didn't already have an alpha channel.
// It also deletes the source surface.
//
// It uses the same pixel format (RGBA, R at lowest address) regardless of
// hardware.
SDL_Surface* canonicalize_format(SDL_Surface* src) {
    // even though we have null check after DFIMG_Load
    // in loadTileset() (the only consumer of this method)
    // it's better put nullcheck here as well
    if (!src)
        return src;

    auto fmt = DFSDL_AllocFormat(SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32);
    SDL_Surface* tgt = DFSDL_ConvertSurface(src, fmt, SDL_SWSURFACE);
    DFSDL_FreeSurface(src);
    for (int x = 0; x < tgt->w; ++x) {
        for (int y = 0; y < tgt->h; ++y) {
            Uint8* p = (Uint8*)tgt->pixels + y * tgt->pitch + x * 4;
            if (p[3] == 0) {
                for (int c = 0; c < 3; c++) {
                    p[c] = 0;
                }
            }
        }
    }

    return tgt;
}

// register surface in texture raws, get a texpos
static long add_texture(SDL_Surface* surface) {
    std::lock_guard<std::mutex> lg_add_texture(g_adding_mutex);
    auto texpos = enabler->textures.raws.size();
    enabler->textures.raws.push_back(surface);
    return texpos;
}

// register surface in texture raws to specific texpos
static void insert_texture(SDL_Surface* surface, long texpos) {
    std::lock_guard<std::mutex> lg_add_texture(g_adding_mutex);
    enabler->textures.raws[texpos] = surface;
}

// delete surface from texture raws
static void delete_texture(long texpos) {
    std::lock_guard<std::mutex> lg_add_texture(g_adding_mutex);
    auto pos = static_cast<size_t>(texpos);
    if (pos >= enabler->textures.raws.size())
        return;
    enabler->textures.raws[texpos] = NULL;
}

// create new surface with RGBA32 format and pixels as data
SDL_Surface* create_texture(std::vector<uint32_t>& pixels, int texture_px_w, int texture_px_h) {
    auto surface = DFSDL_CreateRGBSurfaceWithFormat(0, texture_px_w, texture_px_h, 32,
                                                    SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32);
    auto canvas_length = static_cast<size_t>(texture_px_w * texture_px_h);
    for (size_t i = 0; i < pixels.size() && i < canvas_length; i++) {
        uint32_t* p = (uint32_t*)surface->pixels + i;
        *p = pixels[i];
    }
    return surface;
}

// convert single surface into tiles according w/h
// register tiles in texture raws and return handles
std::vector<TexposHandle> slice_tileset(SDL_Surface* surface, int tile_px_w, int tile_px_h,
                                        bool reserved) {
    std::vector<TexposHandle> handles{};
    if (!surface)
        return handles;

    int dimx = surface->w / tile_px_w;
    int dimy = surface->h / tile_px_h;

    if (reserved && (dimx * dimy > reserved_range.end - reserved_range.current)) {
        WARN(textures).print(
            "there is not enough space in reserved range for whole tileset, using dynamic range\n");
        reserved = false;
    }

    for (int y = 0; y < dimy; y++) {
        for (int x = 0; x < dimx; x++) {
            SDL_Surface* tile = DFSDL_CreateRGBSurface(
                0, tile_px_w, tile_px_h, 32, surface->format->Rmask, surface->format->Gmask,
                surface->format->Bmask, surface->format->Amask);
            SDL_Rect vp{tile_px_w * x, tile_px_h * y, tile_px_w, tile_px_h};
            DFSDL_UpperBlit(surface, &vp, tile, NULL);
            auto handle = Textures::loadTexture(tile, reserved);
            handles.push_back(handle);
        }
    }

    DFSDL_FreeSurface(surface);
    return handles;
}

TexposHandle Textures::loadTexture(SDL_Surface* surface, bool reserved) {
    if (!surface || !enabler)
        return 0; // should be some error, i guess
    if (loading_state)
        reserved = true; // use reserved range during loading for all textures

    auto handle = reinterpret_cast<uintptr_t>(surface);
    g_handle_to_surface.emplace(handle, surface);
    surface->refcount++; // prevent destruct on next FreeSurface by game

    if (reserved && reserved_range.is_installed) {
        auto texpos = reserved_range.get_new_texpos();
        if (texpos != -1) {
            insert_texture(surface, texpos);
            g_handle_to_reserved_texpos.emplace(handle, texpos);
            dummy_surface->refcount--;
            return handle;
        }

        if (loading_state) { // if we in loading state and reserved range is full -> error
            ERR(textures).printerr("reserved range limit has been reached, use dynamic range\n");
            return 0;
        }
    }

    // if we here in loading state = true, then it should be dynamic range -> delay reg
    if (loading_state) {
        g_delayed_regs.push_back(handle);
    } else {
        auto texpos = add_texture(surface);
        g_handle_to_texpos.emplace(handle, texpos);
    }

    return handle;
}

std::vector<TexposHandle> Textures::loadTileset(const std::string& file, int tile_px_w,
                                                int tile_px_h, bool reserved) {
    if (g_tileset_to_handles.contains(file))
        return g_tileset_to_handles[file];
    if (!enabler)
        return std::vector<TexposHandle>{};

    SDL_Surface* surface = DFIMG_Load(file.c_str());
    if (!surface) {
        ERR(textures).printerr("unable to load textures from '%s'\n", file.c_str());
        return std::vector<TexposHandle>{};
    }

    surface = canonicalize_format(surface);
    auto handles = slice_tileset(surface, tile_px_w, tile_px_h, reserved);

    DEBUG(textures).print("loaded %zd textures from '%s'\n", handles.size(), file.c_str());
    g_tileset_to_handles[file] = handles;

    return handles;
}

long Textures::getTexposByHandle(TexposHandle handle) {
    if (!handle || !enabler)
        return -1;

    if (g_handle_to_reserved_texpos.contains(handle))
        return g_handle_to_reserved_texpos[handle];
    if (g_handle_to_texpos.contains(handle))
        return g_handle_to_texpos[handle];
    if (std::find(g_delayed_regs.begin(), g_delayed_regs.end(), handle) != g_delayed_regs.end())
        return 0;
    if (g_handle_to_surface.contains(handle)) {
        g_handle_to_surface[handle]->refcount++; // prevent destruct on next FreeSurface by game
        if (loading_state) { // reinit dor dynamic range during loading -> delayed
            g_delayed_regs.push_back(handle);
            return 0;
        }
        auto texpos = add_texture(g_handle_to_surface[handle]);
        g_handle_to_texpos.emplace(handle, texpos);
        return texpos;
    }

    return -1;
}

TexposHandle Textures::createTile(std::vector<uint32_t>& pixels, int tile_px_w, int tile_px_h,
                                  bool reserved) {
    if (!enabler)
        return 0;

    auto texture = create_texture(pixels, tile_px_w, tile_px_h);
    auto handle = Textures::loadTexture(texture, reserved);
    return handle;
}

std::vector<TexposHandle> Textures::createTileset(std::vector<uint32_t>& pixels, int texture_px_w,
                                                  int texture_px_h, int tile_px_w, int tile_px_h,
                                                  bool reserved) {
    if (!enabler)
        return std::vector<TexposHandle>{};

    auto texture = create_texture(pixels, texture_px_w, texture_px_h);
    auto handles = slice_tileset(texture, tile_px_w, tile_px_h, reserved);
    return handles;
}

void Textures::deleteHandle(TexposHandle handle) {
    if (!enabler)
        return;

    auto texpos = Textures::getTexposByHandle(handle);
    if (texpos > 0)
        delete_texture(texpos);
    if (g_handle_to_reserved_texpos.contains(handle))
        g_handle_to_reserved_texpos.erase(handle);
    if (g_handle_to_texpos.contains(handle))
        g_handle_to_texpos.erase(handle);
    if (auto it = std::find(g_delayed_regs.begin(), g_delayed_regs.end(), handle);
        it != g_delayed_regs.end())
        g_delayed_regs.erase(it);
    if (g_handle_to_surface.contains(handle)) {
        auto surface = g_handle_to_surface[handle];
        while (surface->refcount)
            DFSDL_FreeSurface(surface);
        g_handle_to_surface.erase(handle);
    }
}

static void reset_texpos() {
    DEBUG(textures).print("resetting texture mappings\n");
    g_handle_to_texpos.clear();
}

static void reset_reserved_texpos() {
    DEBUG(textures).print("resetting reserved texture mappings\n");
    g_handle_to_reserved_texpos.clear();
}

static void reset_tilesets() {
    DEBUG(textures).print("resetting tileset to handle mappings\n");
    g_tileset_to_handles.clear();
}

static void reset_surface() {
    DEBUG(textures).print("deleting cached surfaces\n");
    for (auto& entry : g_handle_to_surface) {
        DFSDL_FreeSurface(entry.second);
    }
    g_handle_to_surface.clear();
}

static void register_delayed_handles() {
    DEBUG(textures).print("register delayed handles, size %zd\n", g_delayed_regs.size());
    for (auto& handle : g_delayed_regs) {
        auto texpos = add_texture(g_handle_to_surface[handle]);
        g_handle_to_texpos.emplace(handle, texpos);
    }
    g_delayed_regs.clear();
}

// reset point on New Game
struct tracking_stage_new_region : df::viewscreen_new_regionst {
    typedef df::viewscreen_new_regionst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        if (this->m_raw_load_stage != this->raw_load_stage) {
            TRACE(textures).print("raw_load_stage %d -> %d\n", this->m_raw_load_stage,
                                  this->raw_load_stage);
            bool tmp_state = loading_state;
            loading_state = this->raw_load_stage >= 0 && this->raw_load_stage < 3 ? true : false;
            if (tmp_state != loading_state && !loading_state)
                register_delayed_handles();
            this->m_raw_load_stage = this->raw_load_stage;
            if (this->m_raw_load_stage == 1)
                reset_texpos();
        }
        INTERPOSE_NEXT(logic)();
    }

  private:
    inline static int m_raw_load_stage = -2; // not valid state at the start
};
IMPLEMENT_VMETHOD_INTERPOSE(tracking_stage_new_region, logic);

// reset point on New Game in Existing World
struct tracking_stage_adopt_region : df::viewscreen_adopt_regionst {
    typedef df::viewscreen_adopt_regionst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        if (this->m_cur_step != this->cur_step) {
            TRACE(textures).print("step %d -> %d\n", this->m_cur_step, this->cur_step);
            bool tmp_state = loading_state;
            loading_state = this->cur_step >= 0 && this->cur_step < 3 ? true : false;
            if (tmp_state != loading_state && !loading_state)
                register_delayed_handles();
            this->m_cur_step = this->cur_step;
            if (this->m_cur_step == 1)
                reset_texpos();
        }
        INTERPOSE_NEXT(logic)();
    }

  private:
    inline static int m_cur_step = -2; // not valid state at the start
};
IMPLEMENT_VMETHOD_INTERPOSE(tracking_stage_adopt_region, logic);

// reset point on Load Game
struct tracking_stage_load_region : df::viewscreen_loadgamest {
    typedef df::viewscreen_loadgamest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        if (this->m_cur_step != this->cur_step) {
            TRACE(textures).print("step %d -> %d\n", this->m_cur_step, this->cur_step);
            bool tmp_state = loading_state;
            loading_state = this->cur_step >= 0 && this->cur_step < 3 ? true : false;
            if (tmp_state != loading_state && !loading_state)
                register_delayed_handles();
            this->m_cur_step = this->cur_step;
            if (this->m_cur_step == 1)
                reset_texpos();
        }
        INTERPOSE_NEXT(logic)();
    }

  private:
    inline static int m_cur_step = -2; // not valid state at the start
};
IMPLEMENT_VMETHOD_INTERPOSE(tracking_stage_load_region, logic);

// reset point on New Arena
struct tracking_stage_new_arena : df::viewscreen_new_arenast {
    typedef df::viewscreen_new_arenast interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        if (this->m_cur_step != this->cur_step) {
            TRACE(textures).print("step %d -> %d\n", this->m_cur_step, this->cur_step);
            bool tmp_state = loading_state;
            loading_state = this->cur_step >= 0 && this->cur_step < 3 ? true : false;
            if (tmp_state != loading_state && !loading_state)
                register_delayed_handles();
            this->m_cur_step = this->cur_step;
            if (this->m_cur_step == 0)
                reset_texpos();
        }
        INTERPOSE_NEXT(logic)();
    }

  private:
    inline static int m_cur_step = -2; // not valid state at the start
};
IMPLEMENT_VMETHOD_INTERPOSE(tracking_stage_new_arena, logic);

static void install_reset_point() {
    INTERPOSE_HOOK(tracking_stage_new_region, logic).apply();
    INTERPOSE_HOOK(tracking_stage_adopt_region, logic).apply();
    INTERPOSE_HOOK(tracking_stage_load_region, logic).apply();
    INTERPOSE_HOOK(tracking_stage_new_arena, logic).apply();
}

static void uninstall_reset_point() {
    INTERPOSE_HOOK(tracking_stage_new_region, logic).remove();
    INTERPOSE_HOOK(tracking_stage_adopt_region, logic).remove();
    INTERPOSE_HOOK(tracking_stage_load_region, logic).remove();
    INTERPOSE_HOOK(tracking_stage_new_arena, logic).remove();
}

static void reserve_static_range() {
    if (static_cast<size_t>(enabler->textures.init_texture_size) != enabler->textures.raws.size()) {
        WARN(textures).print(
            "reserved range can't be installed! all textures will be loaded to dynamic range!");
        return;
    }
    reserved_range.init(enabler->textures.init_texture_size);
    dummy_surface =
        DFSDL_CreateRGBSurfaceWithFormat(0, 0, 0, 32, SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32);
    dummy_surface->refcount += ReservedRange::size;
    for (int32_t i = 0; i < ReservedRange::size; i++) {
        add_texture(dummy_surface);
    }
    enabler->textures.init_texture_size += ReservedRange::size;
}

void Textures::init(color_ostream& out) {
    if (!enabler)
        return;

    reserve_static_range();
    install_reset_point();
    DEBUG(textures, out)
        .print("dynamic texture loading ready, reserved range %d-%d\n", reserved_range.start,
               reserved_range.end);
}

void Textures::cleanup() {
    if (!enabler)
        return;

    reset_texpos();
    reset_reserved_texpos();
    reset_tilesets();
    reset_surface();
    uninstall_reset_point();
}
