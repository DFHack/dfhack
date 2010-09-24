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

int (*alloc_byte_buffer_callback)(int8_t*, uint32_t) = NULL;
int (*alloc_short_buffer_callback)(int16_t*, uint32_t) = NULL;
int (*alloc_int_buffer_callback)(int32_t*, uint32_t) = NULL;

int (*alloc_ubyte_buffer_callback)(uint8_t*, uint32_t) = NULL;
int (*alloc_ushort_buffer_callback)(uint16_t*, uint32_t) = NULL;
int (*alloc_uint_buffer_callback)(uint32_t*, uint32_t) = NULL;

int (*alloc_char_buffer_callback)(char*, uint32_t) = NULL;

int (*alloc_matgloss_buffer_callback)(t_matgloss*, uint32_t) = NULL;
int (*alloc_descriptor_buffer_callback)(t_descriptor_color*, uint32_t) = NULL;
int (*alloc_matgloss_other_buffer_callback)(t_matglossOther*, uint32_t) = NULL;

int (*alloc_t_feature_buffer_callback)(t_feature*, uint32_t) = NULL;
int (*alloc_t_hotkey_buffer_callback)(t_hotkey*, uint32_t) = NULL;
int (*alloc_t_screen_buffer_callback)(t_screen*, uint32_t) = NULL;

int (*alloc_t_customWorkshop_buffer_callback)(t_customWorkshop*, uint32_t) = NULL;
int (*alloc_t_material_buffer_callback)(t_material*, uint32_t) = NULL;

int (*alloc_empty_colormodifier_callback)(c_colormodifier*) = NULL;
int (*alloc_colormodifier_callback)(c_colormodifier*, const char*, uint32_t) = NULL;
int (*alloc_colormodifier_buffer_callback)(c_colormodifier*, uint32_t) = NULL;

int (*alloc_empty_creaturecaste_callback)(c_creaturecaste*)= NULL;
int (*alloc_creaturecaste_callback)(c_creaturecaste*, const char*, const char*, const char*, const char*, uint32_t, uint32_t) = NULL;
int (*alloc_creaturecaste_buffer_callback)(c_creaturecaste*, uint32_t) = NULL;

int (*alloc_empty_creaturetype_callback)(c_creaturetype*) = NULL;
int (*alloc_creaturetype_callback)(c_creaturetype*, const char*, uint32_t, uint32_t, uint8_t, uint16_t, uint16_t, uint16_t) = NULL;
int (*alloc_creaturetype_buffer_callback)(c_creaturetype*, uint32_t) = NULL;

int (*alloc_vein_buffer_callback)(t_vein*, uint32_t) = NULL;
int (*alloc_frozenliquidvein_buffer_callback)(t_frozenliquidvein*, uint32_t) = NULL;
int (*alloc_spattervein_buffer_callback)(t_spattervein*, uint32_t) = NULL;

REG_MACRO(Byte, int8_t*, alloc_byte_buffer_callback)
REG_MACRO(Short, int16_t*, alloc_short_buffer_callback)
REG_MACRO(Int, int32_t*, alloc_int_buffer_callback)
REG_MACRO(UByte, uint8_t*, alloc_ubyte_buffer_callback)
REG_MACRO(UShort, uint16_t*, alloc_ushort_buffer_callback)
REG_MACRO(UInt, uint32_t*, alloc_uint_buffer_callback)
REG_MACRO(Char, char*, alloc_char_buffer_callback)
REG_MACRO(Matgloss, t_matgloss*, alloc_matgloss_buffer_callback)
REG_MACRO(DescriptorColor, t_descriptor_color*, alloc_descriptor_buffer_callback)
REG_MACRO(MatglossOther, t_matglossOther*, alloc_matgloss_other_buffer_callback)
REG_MACRO(Feature, t_feature*, alloc_t_feature_buffer_callback)
REG_MACRO(Hotkey, t_hotkey*, alloc_t_hotkey_buffer_callback)
REG_MACRO(Screen, t_screen*, alloc_t_screen_buffer_callback)
REG_MACRO(CustomWorkshop, t_customWorkshop*, alloc_t_customWorkshop_buffer_callback)
REG_MACRO(Material, t_material*, alloc_t_material_buffer_callback)

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
UNREG_MACRO(Feature, alloc_t_feature_buffer_callback)
UNREG_MACRO(Hotkey, alloc_t_hotkey_buffer_callback)
UNREG_MACRO(Screen, alloc_t_screen_buffer_callback)
UNREG_MACRO(CustomWorkshop, alloc_t_customWorkshop_buffer_callback)
UNREG_MACRO(Material, alloc_t_material_buffer_callback)

void RegisterEmptyColorModifierCallback(int (*funcptr)(c_colormodifier*))
{
	alloc_empty_colormodifier_callback = funcptr;
}

void RegisterNewColorModifierCallback(int (*funcptr)(c_colormodifier*, const char*, uint32_t))
{
	alloc_colormodifier_callback = funcptr;
}

REG_MACRO(ColorModifier, c_colormodifier*, alloc_colormodifier_buffer_callback)

void RegisterEmptyCreatureCasteCallback(int (*funcptr)(c_creaturecaste*))
{
	alloc_empty_creaturecaste_callback = funcptr;
}

void UnregisterEmptyColorModifierCallback()
{
	alloc_empty_colormodifier_callback = NULL;
}

void UnregisterNewColorModifierCallback()
{
	alloc_colormodifier_callback = NULL;
}

void RegisterNewCreatureCasteCallback(int (*funcptr)(c_creaturecaste*, const char*, const char*, const char*, const char*, uint32_t, uint32_t))
{
	alloc_creaturecaste_callback = funcptr;
}

REG_MACRO(CreatureCaste, c_creaturecaste*, alloc_creaturecaste_buffer_callback)
UNREG_MACRO(CreatureCaste, alloc_creaturecaste_buffer_callback)

void UnregisterEmptyCreatureCasteCallback()
{
	alloc_empty_creaturecaste_callback = NULL;
}

void UnregisterNewCreatureCasteCallback()
{
	alloc_creaturecaste_callback = NULL;
}

void RegisterEmptyCreatureTypeCallback(int (*funcptr)(c_creaturetype*))
{
	alloc_empty_creaturetype_callback = funcptr;
}

void RegisterNewCreatureTypeCallback(int (*funcptr)(c_creaturetype*, const char*, uint32_t, uint32_t, uint8_t, uint16_t, uint16_t, uint16_t))
{
	alloc_creaturetype_callback = funcptr;
}

REG_MACRO(CreatureType, c_creaturetype*, alloc_creaturetype_buffer_callback)
UNREG_MACRO(CreatureType, alloc_creaturetype_buffer_callback)

void UnregisterEmptyCreatureTypeCallback()
{
	alloc_empty_creaturetype_callback = NULL;
}

void UnregisterNewCreatureTypeCallback()
{
	alloc_creaturetype_callback = NULL;
}

REG_MACRO(Vein, t_vein*, alloc_vein_buffer_callback)
REG_MACRO(FrozenLiquidVein, t_frozenliquidvein*, alloc_frozenliquidvein_buffer_callback)
REG_MACRO(SpatterVein, t_spattervein*, alloc_spattervein_buffer_callback)

UNREG_MACRO(Vein, alloc_vein_buffer_callback)
UNREG_MACRO(FrozenLiquidVein, alloc_frozenliquidvein_buffer_callback)
UNREG_MACRO(SpatterVein, alloc_spattervein_buffer_callback)

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

int ColorListConvert(t_colormodifier* src, c_colormodifier* dest)
{
	if(src == NULL)
		return -1;
	
	((*alloc_colormodifier_callback)(dest, src->part, src->colorlist.size()));
	
	copy(src->colorlist.begin(), src->colorlist.end(), dest->colorlist);
	
	return 1;
}

int CreatureCasteConvert(t_creaturecaste* src, c_creaturecaste* dest)
{
	if(src == NULL)
		return -1;
	
	((*alloc_creaturecaste_callback)(dest, src->rawname, src->singular, src->plural, src->adjective, src->ColorModifier.size(), src->bodypart.size()));
	
	for(int i = 0; i < dest->colorModifierLength; i++)
		ColorListConvert(&src->ColorModifier[i], &dest->ColorModifier[i]);
	
	copy(src->bodypart.begin(), src->bodypart.end(), dest->bodypart);
	
	return 1;
}

int CreatureTypeConvert(t_creaturetype* src, c_creaturetype* dest)
{
	if(src == NULL)
		return -1;
	
	((*alloc_creaturetype_callback)(dest, src->rawname, src->castes.size(), src->extract.size(), src->tile_character, src->tilecolor.fore, src->tilecolor.back, src->tilecolor.bright));
	
	for(int i = 0; i < dest->castesCount; i++)
		CreatureCasteConvert(&src->castes[i], &dest->castes[i]);
	
	copy(src->extract.begin(), src->extract.end(), dest->extract);
	
	return 1;
}
