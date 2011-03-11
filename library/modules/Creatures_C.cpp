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

#include "dfhack-c/modules/Creatures_C.h"

#include <vector>
#include <algorithm>

using namespace std;

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

t_material* Creatures_ReadJob(DFHackObject* cPtr, const t_creature* furball)
{
	if(cPtr != NULL)
	{
		std::vector<t_material> mat;
		
		if(((DFHack::Creatures*)cPtr)->ReadJob(furball, mat))
		{
			if(mat.size() <= 0)
				return NULL;
			
			t_material* buf = NULL;
			
			(*alloc_t_material_buffer_callback)(&buf, mat.size());
			
			if(buf != NULL)
			{
				copy(mat.begin(), mat.end(), buf);
				
				return buf;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	
	return NULL;
}

uint32_t* Creatures_ReadInventoryIdx(DFHackObject* cPtr, const uint32_t index)
{
	if(cPtr != NULL)
	{
		std::vector<uint32_t> item;
		
		if(((DFHack::Creatures*)cPtr)->ReadInventoryIdx(index, item))
		{
			if(item.size() <= 0)
				return NULL;
			
			uint32_t* buf = NULL;
			
			(*alloc_uint_buffer_callback)(&buf, item.size());
			
			if(buf != NULL)
			{
				copy(item.begin(), item.end(), buf);
				
				return buf;
			}
			else
				return NULL;
		}
	}
	
	return NULL;
}

uint32_t* Creatures_ReadInventoryPtr(DFHackObject* cPtr, const uint32_t index)
{
	if(cPtr != NULL)
	{
		std::vector<uint32_t> item;
		
		if(((DFHack::Creatures*)cPtr)->ReadInventoryPtr(index, item))
		{
			if(item.size() <= 0)
				return NULL;
			
			uint32_t* buf = NULL;
			
			(*alloc_uint_buffer_callback)(&buf, item.size());
			
			if(buf != NULL)
			{
				copy(item.begin(), item.end(), buf);
				
				return buf;
			}
			else
				return NULL;
		}
	}
	
	return NULL;
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

int Creatures_WriteLabors(DFHackObject* cPtr, const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteLabors(index, labors);
	}
	
	return -1;
}

int Creatures_WriteHappiness(DFHackObject* cPtr, const uint32_t index, const uint32_t happiness_value)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteHappiness(index, happiness_value);
	}
	
	return -1;
}

int Creatures_WriteFlags(DFHackObject* cPtr, const uint32_t index, const uint32_t flags1, const uint32_t flags2)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteFlags(index, flags1, flags2);
	}
	
	return -1;
}

int Creatures_WriteSkills(DFHackObject* cPtr, const uint32_t index, const t_soul* soul)
{
	if(cPtr != NULL && soul != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteSkills(index, *soul);
	}
	
	return -1;
}

int Creatures_WriteAttributes(DFHackObject* cPtr, const uint32_t index, const t_creature* creature)
{
	if(cPtr != NULL && creature != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteAttributes(index, *creature);
	}
	
	return -1;
}

int Creatures_WriteSex(DFHackObject* cPtr, const uint32_t index, const uint8_t sex)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteSex(index, sex);
	}
	
	return -1;
}

int Creatures_WriteTraits(DFHackObject* cPtr, const uint32_t index, const t_soul* soul)
{
	if(cPtr != NULL && soul != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteTraits(index, *soul);
	}
	
	return -1;
}

int Creatures_WriteMood(DFHackObject* cPtr, const uint32_t index, const uint16_t mood)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteMood(index, mood);
	}
	
	return -1;
}

int Creatures_WriteMoodSkill(DFHackObject* cPtr, const uint32_t index, const uint16_t moodSkill)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteMoodSkill(index, moodSkill);
	}
	
	return -1;
}

int Creatures_WriteJob(DFHackObject* cPtr, const t_creature* furball, const t_material* mat, const uint32_t mat_count)
{
	if(cPtr != NULL && furball != NULL && mat != NULL)
	{
		if(mat_count == 0)
			return 0;
		
		std::vector<t_material> mat_vec;
		
		copy(mat, mat + mat_count, mat_vec.begin());
		
		return ((DFHack::Creatures*)cPtr)->WriteJob(furball, mat_vec);
	}
	
	return -1;
}

int Creatures_WritePos(DFHackObject* cPtr, const uint32_t index, const t_creature* creature)
{
	if(cPtr != NULL && creature != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WritePos(index, *creature);
	}
	
	return -1;
}

int Creatures_WriteCiv(DFHackObject* cPtr, const uint32_t index, const int32_t civ)
{
	if(cPtr != NULL)
	{
		return ((DFHack::Creatures*)cPtr)->WriteCiv(index, civ);
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
