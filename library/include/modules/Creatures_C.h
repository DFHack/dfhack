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

#ifndef CREATURES_C_API
#define CREATURES_C_API

#include "dfhack/DFExport.h"
#include "dfhack/DFIntegers.h"
#include "DFHack_C.h"
#include "dfhack/DFTypes.h"
#include "modules/Materials.h"
#include "modules/Creatures.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int Creatures_Start(DFHackObject* cPtr, uint32_t* numCreatures);
DFHACK_EXPORT int Creatures_Finish(DFHackObject* cPtr);

DFHACK_EXPORT int32_t Creatures_ReadCreatureInBox(DFHackObject* cPtr, const int32_t index, t_creature* furball, 
													const uint16_t x1, const uint16_t y1, const uint16_t z1, 
													const uint16_t x2, const uint16_t y2, const uint16_t z2);

DFHACK_EXPORT int Creatures_ReadCreature(DFHackObject* cPtr, const int32_t index, t_creature* furball);
DFHACK_EXPORT int Creatures_WriteLabors(DFHackObject* cPtr, const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);

DFHACK_EXPORT uint32_t Creatures_GetDwarfRaceIndex(DFHackObject* cPtr);
DFHACK_EXPORT int32_t Creatures_GetDwarfCivId(DFHackObject* cPtr);

DFHACK_EXPORT int Creatures_ReadJob(DFHackObject* cPtr, const t_creature* furball, t_material* (*t_material_buffer_create)(int));

#ifdef __cplusplus
}
#endif

#endif
