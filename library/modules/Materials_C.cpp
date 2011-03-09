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

#include "dfhack-c/modules/Materials_C.h"

#ifdef __cplusplus
extern "C" {
#endif

int Materials_ReadInorganicMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadInorganicMaterials() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadOrganicMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadOrganicMaterials() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadWoodMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadWoodMaterials() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadPlantMaterials(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadPlantMaterials() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadCreatureTypes(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadCreatureTypes() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadCreatureTypesEx(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadCreatureTypesEx() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadDescriptorColors(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadDescriptorColors() == true)
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Materials_ReadOthers(DFHackObject* mat)
{
	if(mat != NULL)
	{
		if(((DFHack::Materials*)mat)->ReadOthers() == true)
			return 1;
		else
			return 0;
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

const char* Materials_getType(DFHackObject* mat, t_material* material)
{
	if(mat != NULL)
	{
		std::string type = ((DFHack::Materials*)mat)->getType(*material);
		
		return type.c_str();
	}
	
	return "\0";
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

t_matgloss* Materials_getInorganic(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->inorganic.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			if(alloc_matgloss_buffer_callback == NULL)
				return NULL;

			((*alloc_matgloss_buffer_callback)(&buf, materials->inorganic.size()));
			
			if(buf != NULL)
			{
				copy(materials->inorganic.begin(), materials->inorganic.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matgloss* Materials_getOrganic(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->organic.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			if(alloc_matgloss_buffer_callback == NULL)
				return NULL;
			
			((*alloc_matgloss_buffer_callback)(&buf, materials->organic.size()));
			
			if(buf != NULL)
			{
				copy(materials->organic.begin(), materials->organic.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matgloss* Materials_getTree(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->tree.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			if(alloc_matgloss_buffer_callback == NULL)
				return NULL;
			
			((*alloc_matgloss_buffer_callback)(&buf, materials->tree.size()));
			
			if(buf != NULL)
			{
				copy(materials->tree.begin(), materials->tree.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matgloss* Materials_getPlant(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->plant.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			if(alloc_matgloss_buffer_callback == NULL)
				return NULL;
			
			((*alloc_matgloss_buffer_callback)(&buf, materials->plant.size()));
			
			if(buf != NULL)
			{
				copy(materials->plant.begin(), materials->plant.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matgloss* Materials_getRace(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->race.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			if(alloc_matgloss_buffer_callback == NULL)
				return NULL;
			
			((*alloc_matgloss_buffer_callback)(&buf, materials->race.size()));
			
			if(buf != NULL)
			{
				copy(materials->race.begin(), materials->race.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

//race_ex getter goes here...
c_creaturetype* Materials_getRaceEx(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		int matSize = materials->raceEx.size();
		
		if(matSize > 0)
		{
			c_creaturetype* buf = NULL;
			
			if(alloc_creaturetype_buffer_callback == NULL)
				return NULL;
			
			((*alloc_creaturetype_buffer_callback)(&buf, matSize));
			
			if(buf != NULL)
			{
				for(int i = 0; i < matSize; i++)
					CreatureTypeConvert(&materials->raceEx[i], &(buf[i]));
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_descriptor_color* Materials_getColor(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->color.size() > 0)
		{
			t_descriptor_color* buf = NULL;
			
			((*alloc_descriptor_buffer_callback)(&buf, materials->color.size()));
			
			if(buf != NULL)
			{
				copy(materials->color.begin(), materials->color.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matglossOther* Materials_getOther(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->other.size() > 0)
		{
			t_matglossOther* buf = NULL;
			
			((*alloc_matgloss_other_buffer_callback)(&buf, materials->other.size()));
			
			if(buf != NULL)
			{
				copy(materials->other.begin(), materials->other.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

t_matgloss* Materials_getAllDesc(DFHackObject* mat)
{
	if(mat != NULL)
	{
		DFHack::Materials* materials = (DFHack::Materials*)mat;
		
		if(materials->alldesc.size() > 0)
		{
			t_matgloss* buf = NULL;
			
			((*alloc_matgloss_buffer_callback)(&buf, materials->alldesc.size()));
			
			if(buf != NULL)
			{
				copy(materials->race.begin(), materials->alldesc.end(), buf);
				
				return buf;
			}
		}
	}
	
	return NULL;
}

#ifdef __cplusplus
}
#endif
