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

#include "integers.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include "DFCommonInternal.h"
#include "DFTypes.h"
#include "modules/Materials.h"
#include "DFTypes_C.h"
#include "modules/Materials_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

int Materials_ReadInorganicMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadInorganicMaterials();
	}
	
	return -1;
}

int Materials_ReadOrganicMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadOrganicMaterials();
	}
	
	return -1;
}

int Materials_ReadWoodMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadWoodMaterials();
	}
	
	return -1;
}

int Materials_ReadPlantMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadPlantMaterials();
	}
	
	return -1;
}

int Materials_ReadCreatureTypes(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadCreatureTypes();
	}
	
	return -1;
}

int Materials_ReadCreatureTypesEx(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadCreatureTypesEx();
	}
	
	return -1;
}

int Materials_ReadDescriptorColors(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadDescriptorColors();
	}
	
	return -1;
}

int Materials_ReadOthers(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->ReadOthers();
	}
	
	return -1;
}

void Materials_ReadAllMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		((DFHack::Materials*)mat)->ReadAllMaterials();
	}
}

const char* Materials_getDescription(DFHackObject* mat, t_material* material)
{
	if(mat != NULL)
	{
		std::string description = ((DFHack::Materials*)mat)->getDescription(*material);
		
		return description.c_str();
	}
	
	return "\0";
}

//vector size getters

int Materials_getInorganicSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->inorganic.size();
	}
	
	return -1;
}

int Materials_getOrganicSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->organic.size();
	}
	
	return -1;
}

int Materials_getTreeSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->tree.size();
	}
	
	return -1;
}

int Materials_getPlantSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->plant.size();
	}
	
	return -1;
}

int Materials_getRaceSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->race.size();
	}
	
	return -1;
}

int Materials_getRaceExSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->raceEx.size();
	}
	
	return -1;
}

int Materials_getColorSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->color.size();
	}
	
	return -1;
}

int Materials_getOtherSize(DFHackObject* mat)
{
	if(mat != NULL)
	{
		return ((DFHack::Materials*)mat)->other.size();
	}
	
	return -1;
}

//vector getters

int Materials_getInorganic(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->inorganic.size() > 0)
		{
			t_matgloss* buf = ((*alloc_matgloss_buffer_callback)(materials->inorganic.size()));
			
			if(buf != NULL)
			{
				copy(materials->inorganic.begin(), materials->inorganic.end(), buf);
				
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

int Materials_getOrganic(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->organic.size() > 0)
		{
			t_matgloss* buf = ((*alloc_matgloss_buffer_callback)(materials->organic.size()));
			
			if(buf != NULL)
			{
				copy(materials->organic.begin(), materials->organic.end(), buf);
				
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

int Materials_getTree(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->tree.size() > 0)
		{
			t_matgloss* buf = ((*alloc_matgloss_buffer_callback)(materials->tree.size()));
			
			if(buf != NULL)
			{
				copy(materials->tree.begin(), materials->tree.end(), buf);
				
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

int Materials_getPlant(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->plant.size() > 0)
		{
			t_matgloss* buf = ((*alloc_matgloss_buffer_callback)(materials->plant.size()));
			
			if(buf != NULL)
			{
				copy(materials->plant.begin(), materials->plant.end(), buf);
				
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

int Materials_getRace(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->race.size() > 0)
		{
			t_matgloss* buf = ((*alloc_matgloss_buffer_callback)(materials->race.size()));
			
			if(buf != NULL)
			{
				copy(materials->race.begin(), materials->race.end(), buf);
				
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

//race_ex getter goes here...
// int Materials_getRaceEx(DFHackObject* mat, c_creaturetype* (*c_creaturetype_buffer_create)(c_creaturetype_descriptor*, int))
// {
	// if(mat != NULL)
	// {
		// DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		// if(materials->raceEx.size() > 0)
		// {
			// std::vector<t_creaturetype> types = materials->raceEx;
			// int typessize = types.size();
			
			// c_creaturetype_descriptor* descriptors = (c_creaturetype_descriptor*)malloc(sizeof(c_creaturetype_descriptor) * typessize);
			
			// for(int i = 0; i < typessize; i++)
			// {
				// descriptors[i].castesCount = types[i].castes.size();
				// descriptors[i].extractCount = types[i].extract.size();
			// }
			
			// c_creaturetype* buf = ((*c_creaturetype_buffer_create)(descriptors, typessize));
			
			// for(int i = 0; i < typessize; i++)
			// {
				// t_creaturetype current = types[i];
				
				// strncpy(buf[i].rawname, current.rawname, 128);
				// buf[i].rawname[127] = '\0';
				
				// buf[i].tile_character = current.tile_character;
				// buf[i].tilecolor = current.tilecolor;
				
				// current.extract.copy(buf[i].extract, current.extract.size());
			// }
			
			// free(descriptors);
		// }
	// }
	
	// return -1;
// }

int Materials_getColor(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->color.size() > 0)
		{
			t_descriptor_color* buf = ((*alloc_descriptor_buffer_callback)(materials->color.size()));
			
			if(buf != NULL)
			{
				copy(materials->color.begin(), materials->color.end(), buf);
				
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

int Materials_getOther(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->other.size() > 0)
		{
			t_matglossOther* buf = ((*alloc_matgloss_other_buffer_callback)(materials->other.size()));
			
			if(buf != NULL)
			{
				copy(materials->other.begin(), materials->other.end(), buf);
				
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
