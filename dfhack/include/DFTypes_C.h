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

#ifndef TYPES_C_API
#define TYPES_C_API

#include "Export.h"
#include "integers.h"
#include "DFTypes.h"
#include "modules/Materials.h"
#include "DFHackAPI_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

struct c_colormodifier
{
	char part[128];
	uint32_t* colorlist;
	uint32_t colorlistLength;
};

c_colormodifier* ColorModifier_New();
void ColorModifier_Free(c_colormodifier* src);

struct c_creaturecaste
{
	char rawname[128];
	char singular[128];
	char plural[128];
	char adjective[128];
	
	c_colormodifier* ColorModifier;
	uint32_t colorModifierLength;
	
	t_bodypart* bodypart;
	uint32_t bodypartLength;
};

c_creaturecaste* CreatureCaste_New();
void CreatureCaste_Free(c_creaturecaste* src);

struct c_creaturetype
{
	char rawname[128];
	
	c_creaturecaste* castes;
	uint32_t castesCount;
	
	t_creatureextract* extract;
	uint32_t extractCount;
	
	uint8_t tile_character;
	
	struct
	{
		uint16_t fore;
		uint16_t back;
		uint16_t bright;
	} tilecolor;
};

c_creaturetype* CreatureType_New();
void CreatureType_Free(c_creaturetype* src);

#ifdef __cplusplus
}
#endif

int CreatureTypeConvert(t_creaturetype* src, c_creaturetype* dest);
int CreatureCasteConvert(t_creaturecaste* src, c_creaturecaste* dest);
int ColorListConvert(t_colormodifier* src, c_colormodifier* dest);

#endif
