#pragma once

#include <vector>
#include <string>

#include "Export.h"
#include "ColorText.h"

#include <SDL_surface.h>

typedef typename void* TexposHandle;

namespace DFHack {

/**
 * The Textures module - loads and provides access to DFHack textures
 * \ingroup grp_modules
 * \ingroup grp_textures
 */
namespace Textures {

/**
 * Load texture and get handle.
 * Keep it to obtain valid texpos.
 */
DFHACK_EXPORT TexposHandle loadTexture(SDL_Surface* surface);

/**
 * Load tileset from image file.
 * Return vector of handles to obtain valid texposes.
 */
DFHACK_EXPORT std::vector<TexposHandle> loadTileset(const std::string& file, int tile_px_w, int tile_px_h);

/**
 * Get texpos by handle.
 * Always use this function, if you need to get valid texpos for your texure.
 * Texpos can change on game textures reset, but handle will be the same.
 */
DFHACK_EXPORT long getTexposByHandle(TexposHandle handle);

/**
 * Call this on DFHack init and on every viewscreen change so we can reload
 * and reindex textures as needed.
 */
void init(DFHack::color_ostream& out);

/**
 * Call this on DFHack init just once to setup interposed handlers.
 */
void initDynamic(DFHack::color_ostream& out);

/**
 * Call this when DFHack is being unloaded.
 *
 */
void cleanup();

/**
 * Get first texpos for the DFHack logo. This texpos and the next 11 make up the
 * 4x3 grid of logo textures that can be displayed on the UI layer.
 */
DFHACK_EXPORT long getDfhackLogoTexposStart();

/**
 * Get the first texpos for the UI pin tiles. Each are 2x2 grids.
 */
DFHACK_EXPORT long getGreenPinTexposStart();
DFHACK_EXPORT long getRedPinTexposStart();

/**
 * Get the first texpos for the DFHack icons. It's a 5x2 grid.
 */
DFHACK_EXPORT long getIconsTexposStart();

/**
 * Get the first texpos for the on and off icons. It's a 2x1 grid.
 */
DFHACK_EXPORT long getOnOffTexposStart();

/**
 * Get the first texpos for the pathable 32x32 sprites. It's a 2x1 grid.
 */
DFHACK_EXPORT long getMapPathableTexposStart();

/**
 * Get the first texpos for the unsuspend 32x32 sprites. It's a 5x1 grid.
 */
DFHACK_EXPORT long getMapUnsuspendTexposStart();

/**
 * Get the first texpos for the control panel icons. 10x2 grid.
 */
DFHACK_EXPORT long getControlPanelTexposStart();

/**
 * Get the first texpos for the DFHack borders. Each is a 7x3 grid.
 */
DFHACK_EXPORT long getThinBordersTexposStart();
DFHACK_EXPORT long getMediumBordersTexposStart();
DFHACK_EXPORT long getBoldBordersTexposStart();
DFHACK_EXPORT long getPanelBordersTexposStart();
DFHACK_EXPORT long getWindowBordersTexposStart();

}
}
