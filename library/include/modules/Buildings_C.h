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

#ifndef BUILDINGS_C_API
#define BUILDINGS_C_API

#include "dfhack/DFExport.h"
#include "dfhack/DFIntegers.h"
#include "dfhack/DFTypes.h"
#include "modules/Buildings.h"
#include "DFHack_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

struct t_customWorkshop
{
	uint32_t index;
	char name[256];
};

DFHACK_EXPORT int Buildings_Start(DFHackObject* b_Ptr, uint32_t* numBuildings);
DFHACK_EXPORT int Buildings_Finish(DFHackObject* b_Ptr);

DFHACK_EXPORT int Buildings_Read(DFHackObject* b_Ptr, const uint32_t index, t_building* building);

DFHACK_EXPORT int Buildings_ReadCustomWorkshopTypes(DFHackObject* b_Ptr, void* (*t_customWorkshop_buffer_create)(uint32_t));
DFHACK_EXPORT int Buildings_GetCustomWorkshopType(DFHackObject* b_Ptr, t_building* building);

#ifdef __cplusplus
}
#endif

#endif
