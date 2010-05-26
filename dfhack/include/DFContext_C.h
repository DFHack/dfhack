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

#ifndef DFHACK_C_CONTEXT
#define DFHACK_C_CONTEXT

#include "Export.h"
#include "integers.h"

typedef void DFHackObject;

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT DFHackObject* ContextManager_Alloc(const char* path_to_xml);
DFHACK_EXPORT void ContextManager_Free(DFHackObject* contextMgr);

DFHACK_EXPORT int ContextManager_Refresh(DFHackObject* contextMgr);
DFHACK_EXPORT int ContextManager_size(DFHackObject* contextMgr, uint32_t* size);
DFHACK_EXPORT int ContextManager_purge(DFHackObject* contextMgr);

DFHACK_EXPORT DFHackObject* ContextManager_getContext(DFHackObject* contextMgr, uint32_t index);
DFHACK_EXPORT DFHackObject* ContextManager_getSingleContext(DFHackObject* contextMgr);

DFHACK_EXPORT void Context_Free(DFHackObject* context);

DFHACK_EXPORT int Context_Attach(DFHackObject* context);
DFHACK_EXPORT int Context_Detach(DFHackObject* context);
DFHACK_EXPORT int Context_isAttached(DFHackObject* context);

DFHACK_EXPORT int Context_Suspend(DFHackObject* context);
DFHACK_EXPORT int Context_Resume(DFHackObject* context);
DFHACK_EXPORT int Context_isSuspended(DFHackObject* context);
DFHACK_EXPORT int Context_ForceResume(DFHackObject* context);
DFHACK_EXPORT int Context_AsyncSuspend(DFHackObject* context);

DFHACK_EXPORT DFHackObject* Context_getMemoryInfo(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getProcess(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getWindow(DFHackObject* context);

DFHACK_EXPORT DFHackObject* Context_getCreatures(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getMaps(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getGui(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getPosition(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getMaterials(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getTranslation(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getVegetation(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getBuildings(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getConstructions(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getItems(DFHackObject* context);

//these are DANGEROUS...can crash/segfault DF, turn the seas to blood, call up the Antichrist, etc
DFHACK_EXPORT void Context_ReadRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* target);
DFHACK_EXPORT void Context_WriteRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* source);

#ifdef __cplusplus
}
#endif

#endif
