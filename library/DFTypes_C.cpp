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

#include "dfhack/DFIntegers.h"
#include "dfhack/DFTileTypes.h"
#include <stdlib.h>
#include "string.h"
#include <vector>
#include <algorithm>

using namespace std;

#include "Internal.h"
#include "dfhack/DFTypes.h"
#include "dfhack/DFTileTypes.h"
#include "dfhack-c/DFTypes_C.h"
#include "dfhack/modules/Materials.h"

using namespace DFHack;

/*
I believe this is what they call "the bad kind of clever", but writing out registration functions for each callback just feels *so* wrong...

The output of this macro is something like this...

void RegisterByteBufferCallback(int(*fptr)(int8_t*, uint32_t))
{
	alloc_byte_buffer_callback = fptr;
}

*/
#define BUILD(a) a ## BufferCallback
#define REG_MACRO(type_name, type, callback) void BUILD(Register ## type_name) (int (*fptr)(type, uint32_t)) { callback = fptr; }

/*
The output of this macro is something like this...

void UnregisterByteBufferCallback()
{
	alloc_byte_buffer_callback = NULL;
}

*/
#define UNREG_MACRO(type_name, callback) void BUILD(Unregister ## type_name) () { callback = NULL; }

#ifdef __cplusplus
extern "C" {
#endif

int (*alloc_byte_buffer_callback)(int8_t**, uint32_t) = NULL;
int (*alloc_short_buffer_callback)(int16_t**, uint32_t) = NULL;
int (*alloc_int_buffer_callback)(int32_t**, uint32_t) = NULL;

int (*alloc_ubyte_buffer_callback)(uint8_t**, uint32_t) = NULL;
int (*alloc_ushort_buffer_callback)(uint16_t**, uint32_t) = NULL;
int (*alloc_uint_buffer_callback)(uint32_t**, uint32_t) = NULL;

int (*alloc_char_buffer_callback)(char**, uint32_t) = NULL;

int (*alloc_matgloss_buffer_callback)(t_matgloss**, uint32_t) = NULL;
int (*alloc_descriptor_buffer_callback)(t_descriptor_color**, uint32_t) = NULL;
int (*alloc_matgloss_other_buffer_callback)(t_matglossOther**, uint32_t) = NULL;

int (*alloc_feature_buffer_callback)(t_feature**, uint32_t) = NULL;
int (*alloc_hotkey_buffer_callback)(t_hotkey**, uint32_t) = NULL;
int (*alloc_screen_buffer_callback)(t_screen**, uint32_t) = NULL;

int (*alloc_tree_buffer_callback)(t_tree**, uint32_t) = NULL;

int (*alloc_memrange_buffer_callback)(t_memrange**, uint32_t*, uint32_t) = NULL;

int (*alloc_customWorkshop_buffer_callback)(t_customWorkshop**, uint32_t) = NULL;
int (*alloc_material_buffer_callback)(t_material**, uint32_t) = NULL;

int (*alloc_creaturetype_buffer_callback)(c_creaturetype**, c_creaturetype_descriptor*, uint32_t) = NULL;

int (*alloc_vein_buffer_callback)(t_vein**, uint32_t) = NULL;
int (*alloc_frozenliquidvein_buffer_callback)(t_frozenliquidvein**, uint32_t) = NULL;
int (*alloc_spattervein_buffer_callback)(t_spattervein**, uint32_t) = NULL;
int (*alloc_grassvein_buffer_callback)(t_grassvein**, uint32_t) = NULL;
int (*alloc_worldconstruction_buffer_callback)(t_worldconstruction**, uint32_t) = NULL;

int (*alloc_featuremap_buffer_callback)(c_featuremap_node**, uint32_t*, uint32_t) = NULL;

//int (*alloc_bodypart_buffer_callback)(t_bodypart**, uint32_t) = NULL;
REG_MACRO(Byte, int8_t**, alloc_byte_buffer_callback)
REG_MACRO(Short, int16_t**, alloc_short_buffer_callback)
REG_MACRO(Int, int32_t**, alloc_int_buffer_callback)
REG_MACRO(UByte, uint8_t**, alloc_ubyte_buffer_callback)
REG_MACRO(UShort, uint16_t**, alloc_ushort_buffer_callback)
REG_MACRO(UInt, uint32_t**, alloc_uint_buffer_callback)
REG_MACRO(Char, char**, alloc_char_buffer_callback)
REG_MACRO(Matgloss, t_matgloss**, alloc_matgloss_buffer_callback)
REG_MACRO(DescriptorColor, t_descriptor_color**, alloc_descriptor_buffer_callback)
REG_MACRO(MatglossOther, t_matglossOther**, alloc_matgloss_other_buffer_callback)
REG_MACRO(Feature, t_feature**, alloc_feature_buffer_callback)
REG_MACRO(Hotkey, t_hotkey**, alloc_hotkey_buffer_callback)
REG_MACRO(Screen, t_screen**, alloc_screen_buffer_callback)
REG_MACRO(Tree, t_tree**, alloc_tree_buffer_callback)
REG_MACRO(CustomWorkshop, t_customWorkshop**, alloc_customWorkshop_buffer_callback)
REG_MACRO(Material, t_material**, alloc_material_buffer_callback)

void RegisterMemRangeBufferCallback(int (*funcptr)(t_memrange**, uint32_t*, uint32_t))
{
	alloc_memrange_buffer_callback = funcptr;
}

UNREG_MACRO(Byte, alloc_byte_buffer_callback)
UNREG_MACRO(Short, alloc_short_buffer_callback)
UNREG_MACRO(Int, alloc_int_buffer_callback)
UNREG_MACRO(UByte, alloc_ubyte_buffer_callback)
UNREG_MACRO(UShort, alloc_ushort_buffer_callback)
UNREG_MACRO(UInt, alloc_uint_buffer_callback)
UNREG_MACRO(Char, alloc_char_buffer_callback)
UNREG_MACRO(Matgloss, alloc_matgloss_buffer_callback)
UNREG_MACRO(DescriptorColor, alloc_descriptor_buffer_callback)
UNREG_MACRO(MatglossOther, alloc_matgloss_other_buffer_callback)
UNREG_MACRO(Feature, alloc_feature_buffer_callback)
UNREG_MACRO(Hotkey, alloc_hotkey_buffer_callback)
UNREG_MACRO(Screen, alloc_screen_buffer_callback)
UNREG_MACRO(Tree, alloc_tree_buffer_callback)
UNREG_MACRO(MemRange, alloc_memrange_buffer_callback)
UNREG_MACRO(CustomWorkshop, alloc_customWorkshop_buffer_callback)
UNREG_MACRO(Material, alloc_material_buffer_callback)

void RegisterCreatureTypeBufferCallback(int (*funcptr)(c_creaturetype**, c_creaturetype_descriptor*, uint32_t))
{
	alloc_creaturetype_buffer_callback = funcptr;
}

UNREG_MACRO(CreatureType, alloc_creaturetype_buffer_callback)

REG_MACRO(Vein, t_vein**, alloc_vein_buffer_callback)
REG_MACRO(FrozenLiquidVein, t_frozenliquidvein**, alloc_frozenliquidvein_buffer_callback)
REG_MACRO(SpatterVein, t_spattervein**, alloc_spattervein_buffer_callback)
REG_MACRO(GrassVein, t_grassvein**, alloc_grassvein_buffer_callback)
REG_MACRO(WorldConstruction, t_worldconstruction**, alloc_worldconstruction_buffer_callback)

UNREG_MACRO(Vein, alloc_vein_buffer_callback)
UNREG_MACRO(FrozenLiquidVein, alloc_frozenliquidvein_buffer_callback)
UNREG_MACRO(SpatterVein, alloc_spattervein_buffer_callback)
UNREG_MACRO(GrassVein, alloc_grassvein_buffer_callback)
UNREG_MACRO(WorldConstruction, alloc_worldconstruction_buffer_callback)

void RegisterFeatureMapBufferCallback(int (*funcptr)(c_featuremap_node**, uint32_t*, uint32_t))
{
	alloc_featuremap_buffer_callback = funcptr;
}

UNREG_MACRO(FeatureMap, alloc_featuremap_buffer_callback)

int DFHack_isWallTerrain(int in)
{
	return DFHack::isWallTerrain(in);
}

int DFHack_isFloorTerrain(int in)
{
	return DFHack::isFloorTerrain(in);
}

int DFHack_isRampTerrain(int in)
{
	return DFHack::isRampTerrain(in);
}

int DFHack_isStairTerrain(int in)
{
	return DFHack::isStairTerrain(in);
}

int DFHack_isOpenTerrain(int in)
{
	return DFHack::isOpenTerrain(in);
}

int DFHack_getVegetationType(int in)
{
	return DFHack::getVegetationType(in);
}

int DFHack_getTileType(int index, TileRow* tPtr)
{
	if(index >= TILE_TYPE_ARRAY_LENGTH)
		return 0;
	
	*tPtr = tileTypeTable[index];
	
	return 1;
}

#ifdef __cplusplus
}
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
