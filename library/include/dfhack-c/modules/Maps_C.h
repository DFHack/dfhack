/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef MAPS_C_API
#define MAPS_C_API

#include "DFHack_C.h"
#include "dfhack/DFTypes.h"
#include "dfhack/modules/Maps.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int Maps_Start(DFHackObject* maps);
DFHACK_EXPORT int Maps_Finish(DFHackObject* maps);

DFHACK_EXPORT uint16_t* Maps_ReadGeology(DFHackObject* maps);

DFHACK_EXPORT t_feature* Maps_ReadGlobalFeatures(DFHackObject* maps);

DFHACK_EXPORT void Maps_getSize(DFHackObject* maps, uint32_t* x, uint32_t* y, uint32_t* z);
DFHACK_EXPORT int Maps_isValidBlock(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);

DFHACK_EXPORT uint32_t Maps_getBlockPtr(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);

DFHACK_EXPORT int Maps_ReadBlock40d(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, mapblock40d* buffer);

DFHACK_EXPORT int Maps_ReadTileTypes(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, tiletypes40d* buffer);
DFHACK_EXPORT int Maps_WriteTileTypes(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, tiletypes40d* buffer);

DFHACK_EXPORT int Maps_ReadDesignations(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, designations40d* buffer);
DFHACK_EXPORT int Maps_WriteDesignations(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, designations40d* buffer);

DFHACK_EXPORT int Maps_ReadTemperatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_temperatures* temp1, t_temperatures* temp2);
DFHACK_EXPORT int Maps_WriteTemperatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_temperatures* temp1, t_temperatures* temp2);

DFHACK_EXPORT int Maps_ReadOccupancy(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, occupancies40d* buffer);
DFHACK_EXPORT int Maps_WriteOccupancy(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, occupancies40d* buffer);

DFHACK_EXPORT int Maps_ReadDirtyBit(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int* dirtybit);
DFHACK_EXPORT int Maps_WriteDirtyBit(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int dirtybit);

DFHACK_EXPORT int Maps_ReadFeatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t* local, int16_t* global);
DFHACK_EXPORT int Maps_WriteLocalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t local);
DFHACK_EXPORT int Maps_WriteEmptyLocalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);
DFHACK_EXPORT int Maps_WriteGlobalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t local);
DFHACK_EXPORT int Maps_WriteEmptyGlobalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);

DFHACK_EXPORT int Maps_ReadBlockFlags(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_blockflags* blockflags);
DFHACK_EXPORT int Maps_WriteBlockFlags(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_blockflags blockflags);

DFHACK_EXPORT int Maps_ReadRegionOffsets(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, biome_indices40d* buffer);

DFHACK_EXPORT t_vein* Maps_ReadStandardVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);
DFHACK_EXPORT t_frozenliquidvein* Maps_ReadFrozenVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);
DFHACK_EXPORT t_spattervein* Maps_ReadSpatterVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);
DFHACK_EXPORT t_grassvein* Maps_ReadGrassVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);
DFHACK_EXPORT t_worldconstruction* Maps_ReadWorldConstructions(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z);

typedef struct
{
	t_vein* veins;
	t_frozenliquidvein* frozen_veins;
	t_spattervein* spatter_veins;
	t_grassvein* grass_veins;
	t_worldconstruction* world_constructions;
} c_allveins;

DFHACK_EXPORT int Maps_ReadAllVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, c_allveins* vein_struct);

#ifdef __cplusplus
}
#endif

#endif
