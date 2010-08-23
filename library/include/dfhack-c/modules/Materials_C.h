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

#include "DFHack_C.h"
#include "dfhack-c/DFTypes_C.h"
#include "dfhack/modules/Materials.h"

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

DFHACK_EXPORT t_matgloss* Materials_getInorganic(DFHackObject* mat);
DFHACK_EXPORT t_matgloss* Materials_getOrganic(DFHackObject* mat);
DFHACK_EXPORT t_matgloss* Materials_getTree(DFHackObject* mat);
DFHACK_EXPORT t_matgloss* Materials_getPlant(DFHackObject* mat);
DFHACK_EXPORT t_matgloss* Materials_getRace(DFHackObject* mat);

DFHACK_EXPORT c_creaturetype* Materials_getRaceEx(DFHackObject* mat);
DFHACK_EXPORT t_descriptor_color* Materials_getColor(DFHackObject* mat);
DFHACK_EXPORT t_matglossOther* Materials_getOther(DFHackObject* mat);
DFHACK_EXPORT t_matgloss* Materials_getAllDesc(DFHackObject* mat);

#ifdef __cplusplus
}
#endif

#endif
