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

#include "DFHack_C.h"
#include "dfhack/DFTypes.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Position.h"
#include "dfhack/DFTileTypes.h"

#define HBUILD(a) a ## BufferCallback
#define HREG_MACRO(type_name, type) DFHACK_EXPORT void HBUILD(Register ## type_name) (int (*funcptr)(type, uint32_t));

#define HUNREG_MACRO(type_name) DFHACK_EXPORT void HBUILD(Unregister ## type_name) ();

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT extern int (*alloc_byte_buffer_callback)(int8_t*, uint32_t);
DFHACK_EXPORT extern int (*alloc_short_buffer_callback)(int16_t*, uint32_t);
DFHACK_EXPORT extern int (*alloc_int_buffer_callback)(int32_t*, uint32_t);

DFHACK_EXPORT extern int (*alloc_ubyte_buffer_callback)(uint8_t*, uint32_t);
DFHACK_EXPORT extern int (*alloc_ushort_buffer_callback)(uint16_t*, uint32_t);
DFHACK_EXPORT extern int (*alloc_uint_buffer_callback)(uint32_t*, uint32_t);

DFHACK_EXPORT extern int (*alloc_char_buffer_callback)(char*, uint32_t);

DFHACK_EXPORT extern int (*alloc_matgloss_buffer_callback)(t_matgloss*, uint32_t);
DFHACK_EXPORT extern int (*alloc_descriptor_buffer_callback)(t_descriptor_color*, uint32_t);
DFHACK_EXPORT extern int (*alloc_matgloss_other_buffer_callback)(t_matglossOther*, uint32_t);

DFHACK_EXPORT extern int (*alloc_t_feature_buffer_callback)(t_feature*, uint32_t);
DFHACK_EXPORT extern int (*alloc_t_hotkey_buffer_callback)(t_hotkey*, uint32_t);
DFHACK_EXPORT extern int (*alloc_t_screen_buffer_callback)(t_screen*, uint32_t);

DFHACK_EXPORT void RegisterByteBufferCallback(int (*funcptr)(int8_t*, uint32_t));
DFHACK_EXPORT void RegisterShortBufferCallback(int (*funcptr)(int16_t*, uint32_t));
DFHACK_EXPORT void RegisterIntBufferCallback(int (*funcptr)(int32_t*, uint32_t));

DFHACK_EXPORT void RegisterUByteBufferCallback(int (*funcptr)(uint8_t*, uint32_t));
DFHACK_EXPORT void RegisterUShortBufferCallback(int (*funcptr)(uint16_t*, uint32_t));
DFHACK_EXPORT void RegisterUIntBufferCallback(int (*funcptr)(uint32_t*, uint32_t));

DFHACK_EXPORT void RegisterCharBufferCallback(int (*funcptr)(char*, uint32_t));

DFHACK_EXPORT void RegisterMatglossBufferCallback(int (*funcptr)(t_matgloss*, uint32_t));
DFHACK_EXPORT void RegisterDescriptorColorBufferCallback(int (*funcptr)(t_descriptor_color*, uint32_t));
DFHACK_EXPORT void RegisterMatglossOtherBufferCallback(int (*funcptr)(t_matglossOther*, uint32_t));

DFHACK_EXPORT void RegisterFeatureBufferCallback(int (*funcptr)(t_feature*, uint32_t));
DFHACK_EXPORT void RegisterHotkeyBufferCallback(int (*funcptr)(t_hotkey*, uint32_t));
DFHACK_EXPORT void RegisterScreenBufferCallback(int (*funcptr)(t_screen*, uint32_t));

HUNREG_MACRO(Byte)
HUNREG_MACRO(Short)
HUNREG_MACRO(Int)
HUNREG_MACRO(UByte)
HUNREG_MACRO(UShort)
HUNREG_MACRO(UInt)

HUNREG_MACRO(Char)

HUNREG_MACRO(Matgloss)
HUNREG_MACRO(DescriptorColor)
HUNREG_MACRO(MatglossOther)

HUNREG_MACRO(Feature)
HUNREG_MACRO(Hotkey)
HUNREG_MACRO(Screen)

struct t_customWorkshop
{
	uint32_t index;
	char name[256];
};

DFHACK_EXPORT extern int (*alloc_t_customWorkshop_buffer_callback)(t_customWorkshop*, uint32_t);
DFHACK_EXPORT extern int (*alloc_t_material_buffer_callback)(t_material*, uint32_t);

DFHACK_EXPORT void RegisterCustomWorkshopBufferCallback(int (*funcptr)(t_customWorkshop*, uint32_t));
DFHACK_EXPORT void RegisterMaterialBufferCallback(int (*funcptr)(t_material*, uint32_t));

HUNREG_MACRO(CustomWorkshop)
HUNREG_MACRO(Material)

struct c_colormodifier
{
	char part[128];
	uint32_t* colorlist;
	uint32_t colorlistLength;
};

DFHACK_EXPORT extern int (*alloc_empty_colormodifier_callback)(c_colormodifier*);
DFHACK_EXPORT extern int (*alloc_colormodifier_callback)(c_colormodifier*, const char*, uint32_t);
DFHACK_EXPORT extern int (*alloc_colormodifier_buffer_callback)(c_colormodifier*, uint32_t);

DFHACK_EXPORT void RegisterEmptyColorModifierCallback(int (*funcptr)(c_colormodifier*));
DFHACK_EXPORT void RegisterNewColorModifierCallback(int (*funcptr)(c_colormodifier*, const char*, uint32_t));
DFHACK_EXPORT void RegisterColorModifierBufferCallback(int (*funcptr)(c_colormodifier*, uint32_t));

DFHACK_EXPORT void UnregisterEmptyColorModifierCallback();
DFHACK_EXPORT void UnregisterNewColorModifierCallback();
HUNREG_MACRO(ColorModifier)

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

DFHACK_EXPORT extern int (*alloc_empty_creaturecaste_callback)(c_creaturecaste*);
DFHACK_EXPORT extern int (*alloc_creaturecaste_callback)(c_creaturecaste*, const char*, const char*, const char*, const char*, uint32_t, uint32_t);
DFHACK_EXPORT extern int (*alloc_creaturecaste_buffer_callback)(c_creaturecaste*, uint32_t);

DFHACK_EXPORT void RegisterEmptyCreatureCasteCallback(int (*funcptr)(c_creaturecaste*));
DFHACK_EXPORT void RegisterNewCreatureCasteCallback(int (*funcptr)(c_creaturecaste*, const char*, const char*, const char*, const char*, uint32_t, uint32_t));
DFHACK_EXPORT void RegisterCreatureCasteBufferCallback(int (*funcptr)(c_creaturecaste*, uint32_t));

DFHACK_EXPORT void UnregisterEmptyCreatureCasteCallback();
DFHACK_EXPORT void UnregisterNewCreatureCasteCallback();
HUNREG_MACRO(CreatureCaste)

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

DFHACK_EXPORT extern int (*alloc_empty_creaturetype_callback)(c_creaturetype*);
DFHACK_EXPORT extern int (*alloc_creaturetype_callback)(c_creaturetype*, const char*, uint32_t, uint32_t, uint8_t, uint16_t, uint16_t, uint16_t);
DFHACK_EXPORT extern int (*alloc_creaturetype_buffer_callback)(c_creaturetype*, uint32_t);

DFHACK_EXPORT extern int (*alloc_vein_buffer_callback)(t_vein*, uint32_t);
DFHACK_EXPORT extern int (*alloc_frozenliquidvein_buffer_callback)(t_frozenliquidvein*, uint32_t);
DFHACK_EXPORT extern int (*alloc_spattervein_buffer_callback)(t_spattervein*, uint32_t);
DFHACK_EXPORT extern int (*alloc_grassvein_buffer_callback)(t_grassvein*, uint32_t);
DFHACK_EXPORT extern int (*alloc_worldconstruction_buffer_callback)(t_worldconstruction*, uint32_t);

DFHACK_EXPORT void RegisterEmptyCreatureTypeCallback(int (*funcptr)(c_creaturetype*));
DFHACK_EXPORT void RegisterNewCreatureTypeCallback(int (*funcptr)(c_creaturetype*, const char*, uint32_t, uint32_t, uint8_t, uint16_t, uint16_t, uint16_t));
DFHACK_EXPORT void RegisterCreatureTypeBufferCallback(int (*funcptr)(c_creaturetype*, uint32_t));

DFHACK_EXPORT void RegisterVeinBufferCallback(int (*funcptr)(t_vein*, uint32_t));
DFHACK_EXPORT void RegisterFrozenLiquidVeinBufferCallback(int (*funcptr)(t_frozenliquidvein*, uint32_t));
DFHACK_EXPORT void RegisterSpatterVeinBufferCallback(int (*funcptr)(t_spattervein*, uint32_t));
DFHACK_EXPORT void RegisterGrassVeinBufferCallback(int (*funcptr)(t_grassvein*, uint32_t));
DFHACK_EXPORT void RegisterWorldConstructionBufferCallback(int (*funcptr)(t_worldconstruction*, uint32_t));

DFHACK_EXPORT void UnregisterEmptyCreatureTypeCallback();
DFHACK_EXPORT void UnregisterNewCreatureTypeCallback();
HUNREG_MACRO(CreatureType)

HUNREG_MACRO(Vein)
HUNREG_MACRO(FrozenLiquidVein)
HUNREG_MACRO(SpatterVein)
HUNREG_MACRO(GrassVein)
HUNREG_MACRO(WorldConstruction)

DFHACK_EXPORT int DFHack_isWallTerrain(int in);
DFHACK_EXPORT int DFHack_isFloorTerrain(int in);
DFHACK_EXPORT int DFHack_isRampTerrain(int in);
DFHACK_EXPORT int DFHack_isStairTerrain(int in);
DFHACK_EXPORT int DFHack_isOpenTerrain(int in);
DFHACK_EXPORT int DFHack_getVegetationType(int in);

DFHACK_EXPORT int DFHack_getTileType(int index, TileRow* tPtr);

#ifdef __cplusplus
}
#endif

int CreatureTypeConvert(t_creaturetype* src, c_creaturetype* dest);
int CreatureCasteConvert(t_creaturecaste* src, c_creaturecaste* dest);
int ColorListConvert(t_colormodifier* src, c_colormodifier* dest);

#endif
