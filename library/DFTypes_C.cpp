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
#include <stdlib.h>
#include "string.h"
#include <vector>
#include <algorithm>

using namespace std;

#include "dfhack/DFCommonInternal.h"
#include "dfhack/DFTypes.h"
#include "dfhack-c/DFTypes_C.h"
#include "dfhack/modules/Materials.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

int8_t* (*alloc_byte_buffer_callback)(uint32_t) = NULL;
int16_t* (*alloc_short_buffer_callback)(uint32_t) = NULL;
int32_t* (*alloc_int_buffer_callback)(uint32_t) = NULL;

uint8_t* (*alloc_ubyte_buffer_callback)(uint32_t) = NULL;
uint16_t* (*alloc_ushort_buffer_callback)(uint32_t) = NULL;
uint32_t* (*alloc_uint_buffer_callback)(uint32_t) = NULL;

char* (*alloc_char_buffer_callback)(uint32_t) = NULL;

t_matgloss* (*alloc_matgloss_buffer_callback)(int) = NULL;
t_descriptor_color* (*alloc_descriptor_buffer_callback)(int) = NULL;
t_matglossOther* (*alloc_matgloss_other_buffer_callback)(int) = NULL;

c_colormodifier* (*alloc_empty_colormodifier_callback)(void) = NULL;
c_colormodifier* (*alloc_colormodifier_callback)(const char*, uint32_t) = NULL;
c_colormodifier* (*alloc_colormodifier_buffer_callback)(uint32_t) = NULL;

c_creaturecaste* (*alloc_empty_creaturecaste_callback)(void) = NULL;
c_creaturecaste* (*alloc_creaturecaste_callback)(const char*, const char*, const char*, const char*, uint32_t, uint32_t) = NULL;
c_creaturecaste* (*alloc_creaturecaste_buffer_callback)(uint32_t) = NULL;

c_creaturetype* (*alloc_empty_creaturetype_callback)(void) = NULL;
c_creaturetype* (*alloc_creaturetype_callback)(const char*, uint32_t, uint32_t, uint8_t, uint16_t, uint16_t, uint16_t) = NULL;
c_creaturetype* (*alloc_creaturetype_buffer_callback)(uint32_t) = NULL;

#ifdef __cplusplus
}
#endif

int ColorListConvert(t_colormodifier* src, c_colormodifier* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_colormodifier_callback)(src->part, src->colorlist.size()));
	
	copy(src->colorlist.begin(), src->colorlist.end(), dest->colorlist);
	
	return 1;
}

int CreatureCasteConvert(t_creaturecaste* src, c_creaturecaste* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_creaturecaste_callback)(src->rawname, src->singular, src->plural, src->adjective, src->ColorModifier.size(), src->bodypart.size()));
	
	for(int i = 0; i < dest->colorModifierLength; i++)
		ColorListConvert(&src->ColorModifier[i], &dest->ColorModifier[i]);
	
	copy(src->bodypart.begin(), src->bodypart.end(), dest->bodypart);
	
	return 1;
}

int CreatureTypeConvert(t_creaturetype* src, c_creaturetype* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_creaturetype_callback)(src->rawname, src->castes.size(), src->extract.size(), src->tile_character, src->tilecolor.fore, src->tilecolor.back, src->tilecolor.bright));
	
	for(int i = 0; i < dest->castesCount; i++)
		CreatureCasteConvert(&src->castes[i], &dest->castes[i]);
	
	copy(src->extract.begin(), src->extract.end(), dest->extract);
	
	return 1;
}
