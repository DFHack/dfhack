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

#include "dfhack-c/Common.h"
#include "DFProcess_C.h"
#include "dfhack/DFTypes.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Gui.h"

#define HBUILD(a) a ## BufferCallback
#define HREG_MACRO(type_name, type) DFHACK_EXPORT void HBUILD(Register ## type_name) (int (*funcptr)(type, uint32_t));

#define HUNREG_MACRO(type_name) DFHACK_EXPORT void HBUILD(Unregister ## type_name) ();

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT extern int (*alloc_byte_buffer_callback)(int8_t**, uint32_t);
DFHACK_EXPORT extern int (*alloc_short_buffer_callback)(int16_t**, uint32_t);
DFHACK_EXPORT extern int (*alloc_int_buffer_callback)(int32_t**, uint32_t);

DFHACK_EXPORT extern int (*alloc_ubyte_buffer_callback)(uint8_t**, uint32_t);
DFHACK_EXPORT extern int (*alloc_ushort_buffer_callback)(uint16_t**, uint32_t);
DFHACK_EXPORT extern int (*alloc_uint_buffer_callback)(uint32_t**, uint32_t);

DFHACK_EXPORT extern int (*alloc_char_buffer_callback)(char**, uint32_t);

DFHACK_EXPORT extern int (*alloc_matgloss_buffer_callback)(t_matgloss**, uint32_t);
DFHACK_EXPORT extern int (*alloc_descriptor_buffer_callback)(t_descriptor_color**, uint32_t);
DFHACK_EXPORT extern int (*alloc_matgloss_other_buffer_callback)(t_matglossOther**, uint32_t);

DFHACK_EXPORT extern int (*alloc_feature_buffer_callback)(t_feature**, uint32_t);
DFHACK_EXPORT extern int (*alloc_hotkey_buffer_callback)(t_hotkey**, uint32_t);
DFHACK_EXPORT extern int (*alloc_screen_buffer_callback)(t_screen**, uint32_t);

DFHACK_EXPORT extern int (*alloc_tree_buffer_callback)(dfh_plant**, uint32_t);

DFHACK_EXPORT extern int (*alloc_memrange_buffer_callback)(t_memrange**, uint32_t*, uint32_t);

DFHACK_EXPORT void RegisterByteBufferCallback(int (*funcptr)(int8_t**, uint32_t));
DFHACK_EXPORT void RegisterShortBufferCallback(int (*funcptr)(int16_t**, uint32_t));
DFHACK_EXPORT void RegisterIntBufferCallback(int (*funcptr)(int32_t**, uint32_t));

DFHACK_EXPORT void RegisterUByteBufferCallback(int (*funcptr)(uint8_t**, uint32_t));
DFHACK_EXPORT void RegisterUShortBufferCallback(int (*funcptr)(uint16_t**, uint32_t));
DFHACK_EXPORT void RegisterUIntBufferCallback(int (*funcptr)(uint32_t**, uint32_t));

DFHACK_EXPORT void RegisterCharBufferCallback(int (*funcptr)(char**, uint32_t));

DFHACK_EXPORT void RegisterMatglossBufferCallback(int (*funcptr)(t_matgloss**, uint32_t));
DFHACK_EXPORT void RegisterDescriptorColorBufferCallback(int (*funcptr)(t_descriptor_color**, uint32_t));
DFHACK_EXPORT void RegisterMatglossOtherBufferCallback(int (*funcptr)(t_matglossOther**, uint32_t));

DFHACK_EXPORT void RegisterFeatureBufferCallback(int (*funcptr)(t_feature**, uint32_t));
DFHACK_EXPORT void RegisterHotkeyBufferCallback(int (*funcptr)(t_hotkey**, uint32_t));
DFHACK_EXPORT void RegisterScreenBufferCallback(int (*funcptr)(t_screen**, uint32_t));

DFHACK_EXPORT void RegisterTreeBufferCallback(int (*funcptr)(dfh_plant**, uint32_t));

DFHACK_EXPORT void RegisterMemRangeBufferCallback(int (*funcptr)(t_memrange**, uint32_t*, uint32_t));

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

HUNREG_MACRO(Tree)
HUNREG_MACRO(MemRange)

struct t_customWorkshop
{
	uint32_t index;
	char name[256];
};

DFHACK_EXPORT extern int (*alloc_customWorkshop_buffer_callback)(t_customWorkshop**, uint32_t);
DFHACK_EXPORT extern int (*alloc_material_buffer_callback)(t_material**, uint32_t);

DFHACK_EXPORT void RegisterCustomWorkshopBufferCallback(int (*funcptr)(t_customWorkshop**, uint32_t));
DFHACK_EXPORT void RegisterMaterialBufferCallback(int (*funcptr)(t_material**, uint32_t));

HUNREG_MACRO(CustomWorkshop)
HUNREG_MACRO(Material)

struct c_colormodifier
{
	char part[128];
	uint32_t* colorlist;
	uint32_t colorlistLength;
	uint32_t startdate;
	uint32_t enddate;
};

struct c_colormodifier_descriptor
{
	uint32_t colorlistLength;
};

struct c_creaturecaste
{
	char rawname[128];
	char singular[128];
	char plural[128];
	char adjective[128];
	
	c_colormodifier* colorModifier;
	uint32_t colorModifierLength;
	
	t_bodypart* bodypart;
	uint32_t bodypartLength;
	
	t_attrib strength;
	t_attrib agility;
	t_attrib toughness;
	t_attrib endurance;
	t_attrib recuperation;
	t_attrib disease_resistance;
	t_attrib analytical_ability;
	t_attrib focus;
	t_attrib willpower;
	t_attrib creativity;
	t_attrib intuition;
	t_attrib patience;
	t_attrib memory;
	t_attrib linguistic_ability;
	t_attrib spatial_sense;
	t_attrib musicality;
	t_attrib kinesthetic_sense;
};

struct c_creaturecaste_descriptor
{
	c_colormodifier_descriptor* color_descriptors;
	uint32_t colorModifierLength;
	uint32_t bodypartLength;
};

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

struct c_creaturetype_descriptor
{
	c_creaturecaste_descriptor* caste_descriptors;
	uint32_t castesCount;
	uint32_t extractCount;
};

DFHACK_EXPORT extern int (*alloc_creaturetype_buffer_callback)(c_creaturetype**, c_creaturetype_descriptor*, uint32_t);

DFHACK_EXPORT extern int (*alloc_vein_buffer_callback)(t_vein**, uint32_t);
DFHACK_EXPORT extern int (*alloc_frozenliquidvein_buffer_callback)(t_frozenliquidvein**, uint32_t);
DFHACK_EXPORT extern int (*alloc_spattervein_buffer_callback)(t_spattervein**, uint32_t);
DFHACK_EXPORT extern int (*alloc_grassvein_buffer_callback)(t_grassvein**, uint32_t);
DFHACK_EXPORT extern int (*alloc_worldconstruction_buffer_callback)(t_worldconstruction**, uint32_t);

DFHACK_EXPORT void RegisterCreatureTypeBufferCallback(int (*funcptr)(c_creaturetype**, c_creaturetype_descriptor*, uint32_t));

DFHACK_EXPORT void RegisterVeinBufferCallback(int (*funcptr)(t_vein**, uint32_t));
DFHACK_EXPORT void RegisterFrozenLiquidVeinBufferCallback(int (*funcptr)(t_frozenliquidvein**, uint32_t));
DFHACK_EXPORT void RegisterSpatterVeinBufferCallback(int (*funcptr)(t_spattervein**, uint32_t));
DFHACK_EXPORT void RegisterGrassVeinBufferCallback(int (*funcptr)(t_grassvein**, uint32_t));
DFHACK_EXPORT void RegisterWorldConstructionBufferCallback(int (*funcptr)(t_worldconstruction**, uint32_t));

HUNREG_MACRO(CreatureType)

HUNREG_MACRO(Vein)
HUNREG_MACRO(FrozenLiquidVein)
HUNREG_MACRO(SpatterVein)
HUNREG_MACRO(GrassVein)
HUNREG_MACRO(WorldConstruction)

struct c_mapcoord
{
	union
	{
		struct
		{
			uint16_t x;
			uint16_t y;
			uint32_t z;
		};
		struct
		{
			uint16_t x;
			uint16_t y;
		} dim;
		
		uint64_t comparate;
	};
};

struct c_featuremap_node
{
	c_mapcoord coordinate;
	t_feature* features;
	uint32_t feature_length;
};

DFHACK_EXPORT extern int (*alloc_featuremap_buffer_callback)(c_featuremap_node**, uint32_t*, uint32_t);

DFHACK_EXPORT void RegisterFeatureMapBufferCallback(int (*funcptr)(c_featuremap_node**, uint32_t*, uint32_t));

#ifdef __cplusplus
} // extern "C"
#endif

#endif
