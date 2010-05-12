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

#ifndef MATERIALS_C_API
#define MATERIALS_C_API

#include "Export.h"
#include "integers.h"
#include "DFTypes.h"
#include "modules/Materials.h"
#include "DFHackAPI_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int Materials_ReadInorganicMaterials(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadOrganicMaterials(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadWoodMaterials(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadPlantMaterials(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadCreatureTypes(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadCreatureTypesEx(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadDescriptorColors(DFHackObject* mat);
DFHACK_EXPORT int Materials_ReadOthers(DFHackObject* mat);

DFHACK_EXPORT void Materials_ReadAllMaterials(DFHackObject* mat);

DFHACK_EXPORT const char* Materials_getDescription(DFHackObject* mat, t_material* material);

DFHACK_EXPORT int Materials_getInorganicSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getOrganicSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getTreeSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getPlantSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getRaceSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getRaceExSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getColorSize(DFHackObject* mat);
DFHACK_EXPORT int Materials_getOtherSize(DFHackObject* mat);

DFHACK_EXPORT int Materials_getInorganic(DFHackObject* mat, t_matgloss* (*t_matgloss_buffer_create)(int));
DFHACK_EXPORT int Materials_getOrganic(DFHackObject* mat, t_matgloss* (*t_matgloss_buffer_create)( int));
DFHACK_EXPORT int Materials_getTree(DFHackObject* mat, t_matgloss* (*t_matgloss_buffer_create)(int));
DFHACK_EXPORT int Materials_getPlant(DFHackObject* mat, t_matgloss* (*t_matgloss_buffer_create)(int));
DFHACK_EXPORT int Materials_getRace(DFHackObject* mat, t_matgloss* (*t_matgloss_buffer_create)(int));

/*doomchild:  
	I haven't done getRaceEx yet, because I'm not sure about the best way to make the t_creaturetype struct
	accessible from C.
*/
// DFHACK_EXPORT int Materials_getRaceEx(DFHackObject* mat, t_creaturetype* (*t_creaturetype_buffer_create)(int));

DFHACK_EXPORT int Materials_getColor(DFHackObject* mat, t_descriptor_color* (*t_descriptor_color_buffer_create)(int));
DFHACK_EXPORT int Materials_getOther(DFHackObject* mat, t_matglossOther* (*t_matglossOther_buffer_create)(int));

#ifdef __cplusplus
}
#endif

#endif
