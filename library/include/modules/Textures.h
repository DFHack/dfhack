#pragma once

#include <string>
#include <vector>

#include "ColorText.h"
#include "Export.h"

struct SDL_Surface;

typedef uintptr_t TexposHandle;

namespace DFHack {

/**
 * The Textures module - loads and provides access to DFHack textures
 * \ingroup grp_modules
 * \ingroup grp_textures
 */
namespace Textures {

const uint32_t TILE_WIDTH_PX = 8;
const uint32_t TILE_HEIGHT_PX = 12;

/**
 * Load texture and get handle.
 * Keep it to obtain valid texpos.
 */
DFHACK_EXPORT TexposHandle loadTexture(SDL_Surface* surface, bool reserved = false);

/**
 * Load tileset from image file.
 * Return vector of handles to obtain valid texposes.
 */
DFHACK_EXPORT std::vector<TexposHandle> loadTileset(const std::string& file,
                                                    int tile_px_w = TILE_WIDTH_PX,
                                                    int tile_px_h = TILE_HEIGHT_PX,
                                                    bool reserved = false);

/**
 * Get texpos by handle.
 * Always use this function, if you need to get valid texpos for your texture.
 * Texpos can change on game textures reset, but handle will be the same.
 */
DFHACK_EXPORT long getTexposByHandle(TexposHandle handle);

/**
 * Delete all info about texture by TexposHandle
 */
DFHACK_EXPORT void deleteHandle(TexposHandle handle);

/**
 * Create new texture with RGBA32 format and pixels as data in row major order.
 * Register this texture and return TexposHandle.
 */
DFHACK_EXPORT TexposHandle createTile(std::vector<uint32_t>& pixels, int tile_px_w = TILE_WIDTH_PX,
                                      int tile_px_h = TILE_HEIGHT_PX, bool reserved = false);

/**
 * Create new textures as tileset with RGBA32 format and pixels as data in row major order.
 * Register this textures and return vector of TexposHandle.
 */
DFHACK_EXPORT std::vector<TexposHandle> createTileset(std::vector<uint32_t>& pixels,
                                                      int texture_px_w, int texture_px_h,
                                                      int tile_px_w = TILE_WIDTH_PX,
                                                      int tile_px_h = TILE_HEIGHT_PX,
                                                      bool reserved = false);

/**
 * Call this on DFHack init just once to setup interposed handlers and
 * init static assets.
 */
void init(DFHack::color_ostream& out);

/**
 * Call this when DFHack is being unloaded.
 *
 */
void cleanup();

} // namespace Textures
} // namespace DFHack
