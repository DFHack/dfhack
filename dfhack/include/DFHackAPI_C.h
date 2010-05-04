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

#ifndef DFHACK_C_API
#define DFHACK_C_API

#include "Export.h"
#include "integers.h"

typedef void DFHackObject;

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT DFHackObject* API_Alloc(const char* path_to_xml);
DFHACK_EXPORT void API_Free(DFHackObject* api);

DFHACK_EXPORT int API_Attach(DFHackObject* api);
DFHACK_EXPORT int API_Detach(DFHackObject* api);
DFHACK_EXPORT int API_isAttached(DFHackObject* api);

DFHACK_EXPORT int API_Suspend(DFHackObject* api);
DFHACK_EXPORT int API_Resume(DFHackObject* api);
DFHACK_EXPORT int API_isSuspended(DFHackObject* api);
DFHACK_EXPORT int API_ForceResume(DFHackObject* api);
DFHACK_EXPORT int API_AsyncSuspend(DFHackObject* api);

DFHACK_EXPORT DFHackObject* API_getMemoryInfo(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getProcess(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getWindow(DFHackObject* api);

DFHACK_EXPORT DFHackObject* API_getCreatures(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getMaps(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getGui(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getPosition(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getMaterials(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getTranslation(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getVegetation(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getBuildings(DFHackObject* api);
DFHACK_EXPORT DFHackObject* API_getConstructions(DFHackObject* api);

//these are DANGEROUS...can crash/segfault DF, turn the seas to blood, call up the Antichrist, etc
DFHACK_EXPORT void API_ReadRaw(DFHackObject* api, const uint32_t offset, const uint32_t size, uint8_t* target);
DFHACK_EXPORT void API_WriteRaw(DFHackObject* api, const uint32_t offset, const uint32_t size, uint8_t* source);

#ifdef __cplusplus
}
#endif

#endif
