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

#include "Internal.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

#include "dfhack/DFTypes.h"
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

int (*alloc_tree_buffer_callback)(dfh_plant**, uint32_t) = NULL;

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
REG_MACRO(Tree, dfh_plant**, alloc_tree_buffer_callback)
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

#ifdef __cplusplus
}
#endif