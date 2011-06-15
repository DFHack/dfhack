#ifndef DFHACK_API_H
#define DFHACK_API_H

/**
 * \defgroup grp_context Context and Process management
 * \defgroup grp_modules DFHack Module classes
 * \defgroup grp_cwrap C Wrapper
 */

#pragma once

// Defines
#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

// C99 integer types (used everywhere)
#include <stdint.h>

// DFHack core classes and types
#include "dfhack/Error.h"
#include "dfhack/Context.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Types.h"

// DFHack modules
#include "dfhack/modules/Buildings.h"
#include "dfhack/modules/Engravings.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Constructions.h"
#include "dfhack/modules/Creatures.h"
#include "dfhack/modules/Translation.h"
#include "dfhack/modules/World.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/Gui.h"

/*
 * This is a header full of ugly, volatile things.
 * Only for use of official DFHack tools!
 */
#ifdef DFHACK_WANT_MISCUTILS
    #include "dfhack/MiscUtils.h"
#endif

// define this to get the static tiletype->properties mapping
#ifdef DFHACK_WANT_TILETYPES
    #include "dfhack/TileTypes.h"
#endif
#endif
