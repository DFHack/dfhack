#ifndef DFHACK_API_H
#define DFHACK_API_H

// DFHack core classes and types
#include "dfhack/DFIntegers.h"
#include "dfhack/DFGlobal.h"
#include "dfhack/DFError.h"
#include "dfhack/DFContextManager.h"
#include "dfhack/DFContext.h"
#include "dfhack/DFMemInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFTypes.h"

// DFHack modules
#include "dfhack/modules/Buildings.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Position.h"
#include "dfhack/modules/Constructions.h"
#include "dfhack/modules/Creatures.h"
#include "dfhack/modules/Translation.h"
#include "dfhack/modules/World.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"

#ifdef DFHACK_WANT_MISCUTILS
    #include "dfhack/DFMiscUtils.h"
#endif
#ifdef DFHACK_WANT_TILETYPES
    #include "dfhack/DFTileTypes.h"
#endif
#endif