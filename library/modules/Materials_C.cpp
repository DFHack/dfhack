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
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
using namespace std;
#include "dfhack-c/modules/Materials_C.h"

#ifdef __cplusplus
extern "C" {
#endif

void BuildDescriptorList(std::vector<t_creaturetype> & src, c_creaturetype_descriptor** dest)
{
    c_creaturetype_descriptor* descriptor = NULL;
        
    descriptor = (c_creaturetype_descriptor*)calloc(src.size(), sizeof(c_creaturetype_descriptor));
    
    for(uint32_t i = 0; i < src.size(); i++)
    {
        uint32_t castes_size = src[i].castes.size();
        c_creaturetype_descriptor* current = &descriptor[i];
        
        current->castesCount = castes_size;
        current->caste_descriptors = (c_creaturecaste_descriptor*)calloc(castes_size, sizeof(c_creaturecaste_descriptor));
        
        for(uint32_t j = 0; j < castes_size; j++)
        {
            uint32_t color_size = src[i].castes[j].ColorModifier.size();
            c_creaturecaste_descriptor* current_caste = &current->caste_descriptors[j];
            
            current_caste->colorModifierLength = color_size;
            current_caste->color_descriptors = (c_colormodifier_descriptor*)calloc(color_size, sizeof(c_colormodifier_descriptor));
            
            for(uint32_t k = 0; k < color_size; k++)
            {
                c_colormodifier_descriptor* current_color = &current_caste->color_descriptors[k];
                
                current_color->colorlistLength = src[i].castes[j].ColorModifier[k].colorlist.size();
            }
            
            current_caste->bodypartLength = src[i].castes[j].bodypart.size();
        }
        
        current->extractCount = src[i].extract.size();
    }
    
    *dest = descriptor;
}

void FreeDescriptorList(c_creaturetype_descriptor* d, uint32_t length)
{
    for(uint32_t i = 0; i < length; i++)
    {
        c_creaturetype_descriptor* desc = &d[i];
        
        for(uint32_t j = 0; j < desc->castesCount; j++)
            free(desc->caste_descriptors[j].color_descriptors);
        
        free(desc->caste_descriptors);
    }
    
    free(d);
}

int CreatureTypeConvert(std::vector<t_creaturetype> & src, c_creaturetype** out_buf)
{
    if(src.size() <= 0)
        return 0;
    else if(alloc_creaturetype_buffer_callback == NULL)
        return -1;
    else
    {
        c_creaturetype_descriptor* descriptor;
        c_creaturetype* buf;
        
        BuildDescriptorList(src, &descriptor);
        
        ((*alloc_creaturetype_buffer_callback)(out_buf, descriptor, src.size()));
        
        FreeDescriptorList(descriptor, src.size());
        
        if(out_buf == NULL)
            return -1;
        
        buf = out_buf[0];
        
        for(uint32_t i = 0; i < src.size(); i++)
        {
            c_creaturetype* current = &(buf[i]);
            
            memset(current->rawname, '\0', 128);
            strncpy(current->rawname, src[i].rawname, 128);
            
            for(uint32_t j = 0; j < current->castesCount; j++)
            {
                c_creaturecaste* current_caste = &current->castes[j];
                t_creaturecaste* src_caste = &src[i].castes[j];
                
                memset(current_caste->rawname, '\0', 128);
                memset(current_caste->singular, '\0', 128);
                memset(current_caste->plural, '\0', 128);
                memset(current_caste->adjective, '\0', 128);
                
                strncpy(current_caste->rawname, src_caste->rawname, 128);
                strncpy(current_caste->singular, src_caste->singular, 128);
                strncpy(current_caste->plural, src_caste->plural, 128);
                strncpy(current_caste->adjective, src_caste->adjective, 128);
                
                for(uint32_t k = 0; k < src[i].castes[j].ColorModifier.size(); k++)
                {
                    c_colormodifier* current_color = &current_caste->colorModifier[k];
                    
                    memset(current_color->part, '\0', 128);
                    strncpy(current_color->part, src_caste->ColorModifier[k].part, 128);
                    
                    copy(src_caste->ColorModifier[k].colorlist.begin(), src_caste->ColorModifier[k].colorlist.end(), current_color->colorlist);
                    current_color->colorlistLength = src_caste->ColorModifier[k].colorlist.size();
                    
                    current_color->startdate = src_caste->ColorModifier[k].startdate;
                    current_color->enddate = src_caste->ColorModifier[k].enddate;
                }
                
                current_caste->colorModifierLength = src_caste->ColorModifier.size();
                
                copy(src_caste->bodypart.begin(), src_caste->bodypart.end(), current_caste->bodypart);
                current_caste->bodypartLength = src_caste->bodypart.size();
                
                current_caste->strength = src_caste->strength;
                current_caste->agility = src_caste->agility;
                current_caste->toughness = src_caste->toughness;
                current_caste->endurance = src_caste->endurance;
                current_caste->recuperation = src_caste->recuperation;
                current_caste->disease_resistance = src_caste->disease_resistance;
                current_caste->analytical_ability = src_caste->analytical_ability;
                current_caste->focus = src_caste->focus;
                current_caste->willpower = src_caste->willpower;
                current_caste->creativity = src_caste->creativity;
                current_caste->intuition = src_caste->intuition;
                current_caste->patience = src_caste->patience;
                current_caste->memory = src_caste->memory;
                current_caste->linguistic_ability = src_caste->linguistic_ability;
                current_caste->spatial_sense = src_caste->spatial_sense;
                current_caste->musicality = src_caste->musicality;
                current_caste->kinesthetic_sense = src_caste->kinesthetic_sense;
            }
            
            copy(src[i].extract.begin(), src[i].extract.end(), current->extract);
            current->extractCount = src[i].extract.size();
            
            current->tile_character = src[i].tile_character;
            
            current->tilecolor.fore = src[i].tilecolor.fore;
            current->tilecolor.back = src[i].tilecolor.back;
            current->tilecolor.bright = src[i].tilecolor.bright;
        }
        
        return 1;
    }
}


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
			
			CreatureTypeConvert(((DFHack::Materials*)mat)->raceEx, &buf);
			
			return buf;
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
