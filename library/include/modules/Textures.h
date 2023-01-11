#pragma once

#include "Export.h"
#include "ColorText.h"

namespace DFHack {

/**
 * The Textures module - loads and provides access to DFHack textures
 * \ingroup grp_modules
 * \ingroup grp_textures
 */
namespace Textures {

/**
 * Call this on DFHack init and on every viewscreen change so we can reload
 * and reindex textures as needed.
 */
void init(DFHack::color_ostream &out);

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
 * Get the texpos for the UI pin tiles. Each are 2x2 grids.
 */
DFHACK_EXPORT long getGreenPinTexposStart();
DFHACK_EXPORT long getRedPinTexposStart();
}
}
