#pragma once

#include "ColorText.h"
#include "modules/Maps.h"

/**
 * Runs dig-now for the specified tile. Default options apply.
 */
bool dig_now_tile(DFHack::color_ostream &out, const DFHack::DFCoord &pos);
