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

#include "Export.h"
#include "integers.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include "DFTypes.h"
#include "modules/Materials.h"
#include "modules/Creatures.h"
#include "modules/Creatures_C.h"
#include "DFHackAPI_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

int Creatures_Start(DFHackObject* cPtr, uint32_t* numCreatures)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->Start(*numCreatures);
	}
	
	return -1;
}

int Creatures_Finish(DFHackObject* cPtr)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->Finish();
	}
	
	return -1;
}

int32_t Creatures_ReadCreatureInBox(DFHackObject* cPtr, const int32_t index, t_creature* furball, const uint16_t x1, const uint16_t y1, const uint16_t z1, const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->ReadCreatureInBox(index, *furball, x1, y1, z1, x2, y2, z2);
	}
	
	return -1;
}

int Creatures_ReadCreature(DFHackObject* cPtr, const int32_t index, t_creature* furball)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->ReadCreature(index, *furball);
	}
	
	return -1;
}

int Creatures_WriteLabors(DFHackObject* cPtr, const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteLabors(index, labors);
	}
	
	return -1;
}

uint32_t Creatures_GetDwarfRaceIndex(DFHackObject* cPtr)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->GetDwarfRaceIndex();
	}
	
	return 0;
}

int32_t Creatures_GetDwarfCivId(DFHackObject* cPtr)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->GetDwarfCivId();
	}
	
	return -1;
}

int Creatures_ReadJob(DFHackObject* cPtr, const t_creature* furball, t_material* (*t_material_buffer_create)(int))
{
	if(cPtr != NULL)
	{
		std::vector<t_material> mat;
		
		if(((DFHack::Creatures*)cPtr)->ReadJob(furball, mat))
		{
			t_material* buf = (*t_material_buffer_create)(mat.size());
			
			if(buf != NULL)
			{
				copy(mat.begin(), mat.end(), buf);
				
				return 1;
			}
			else
				return -1;
		}
		else
			return 0;
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
