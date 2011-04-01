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

#include "DFHack_C.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
Allocates a new ContextManager.
@param path_to_xml A const string pointer containing the path to the Memory.xml file.
@return A DFHackObject pointer that points to the allocated ContextManager.
*/
DFHACK_EXPORT DFHackObject* ContextManager_Alloc(const char* path_to_xml);

/** 
Frees a previously allocated ContextManager.
@param contextMgr A DFHackObject pointer that points to a previously allocated ContextManager.
@return None.
*/
DFHACK_EXPORT void ContextManager_Free(DFHackObject* contextMgr);

DFHACK_EXPORT int ContextManager_Refresh(DFHackObject* contextMgr);

/**
Gets the number of active DF processes.
@param contextMgr A pointer to an active ContextManager.
@param size A pointer to an unsigned 32-bit integer that will contain the count of active DF processes.
@return 
	- 0:  Failure.
	- 1:  Success.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int ContextManager_size(DFHackObject* contextMgr, uint32_t* size);

DFHACK_EXPORT int ContextManager_purge(DFHackObject* contextMgr);
DFHACK_EXPORT DFHackObject* ContextManager_getContext(DFHackObject* contextMgr, uint32_t index);
DFHACK_EXPORT DFHackObject* ContextManager_getSingleContext(DFHackObject* contextMgr);

/** 
Frees a previously allocated Context.
@param context A DFHackObject pointer that points to a previously allocated Context.
@return None.
*/
DFHACK_EXPORT void Context_Free(DFHackObject* context);

/**
Attaches to a running DF process.
@param context A pointer to an unattached Context.
@return 
	- 0:  Failure.
	- 1:  Success.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_Attach(DFHackObject* context);

/**
Detaches from a tracked DF process.
@param context A pointer to an attached Context.
@return 
	- 0:  Failure.
	- 1:  Success.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_Detach(DFHackObject* context);

/**
Determines whether or not the given Context is attached to a running DF process.
@param context A pointer to an attached Context.
@return 
	- 0:  The supplied Context is not attached.
	- 1:  The supplied Context is attached.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_isAttached(DFHackObject* context);

/**
Suspends a running DF process.
@param context A pointer to an attached Context.
@return 
	- 0:  The tracked process was not suspended.
	- 1:  The tracked process was suspended.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_Suspend(DFHackObject* context);

/**
Resume a running DF process.
@param context A pointer to an attached Context.
@return 
	- 0:  The tracked process was not resumed.
	- 1:  The tracked process was resumed.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_Resume(DFHackObject* context);

/**
Determines whether or not the given Context's tracked process is suspended.
@param context A pointer to an attached Context.
@return 
	- 0:  The tracked process is not suspended.
	- 1:  The tracked process is suspended.
	- -1:  An invalid pointer was supplied.
*/
DFHACK_EXPORT int Context_isSuspended(DFHackObject* context);
DFHACK_EXPORT int Context_ForceResume(DFHackObject* context);
DFHACK_EXPORT int Context_AsyncSuspend(DFHackObject* context);

DFHACK_EXPORT DFHackObject* Context_getMemoryInfo(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getProcess(DFHackObject* context);

DFHACK_EXPORT DFHackObject* Context_getCreatures(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getMaps(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getGui(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getMaterials(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getTranslation(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getVegetation(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getBuildings(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getConstructions(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getItems(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getWorld(DFHackObject* context);
DFHACK_EXPORT DFHackObject* Context_getWindowIO(DFHackObject* context);

//these are DANGEROUS...can crash/segfault DF, turn the seas to blood, call up the Antichrist, etc
DFHACK_EXPORT void Context_ReadRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* target);
DFHACK_EXPORT void Context_WriteRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* source);

#ifdef __cplusplus
}
#endif

#endif
