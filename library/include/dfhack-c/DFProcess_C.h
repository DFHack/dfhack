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

#ifndef PROCESS_C_API
#define PROCESS_C_API

#include "DFHack_C.h"
#include "dfhack/DFProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

struct c_processID
{
	uint64_t time;
	uint64_t pid;
};

#define PROCESSID_C_NULL	-2
#define PROCESSID_C_LT		-1
#define PROCESSID_C_EQ		0
#define PROCESSID_C_GT		1

DFHACK_EXPORT extern int C_ProcessID_Compare(c_processID* left, c_processID* right);

DFHACK_EXPORT int Process_attach(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_detach(DFHackObject* p_Ptr);

DFHACK_EXPORT int Process_suspend(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_asyncSuspend(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_resume(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_forceresume(DFHackObject* p_Ptr);

DFHACK_EXPORT int Process_readQuad(DFHackObject* p_Ptr, uint32_t address, uint64_t* value);
DFHACK_EXPORT int Process_writeQuad(DFHackObject* p_Ptr, uint32_t address, uint64_t value);

DFHACK_EXPORT int Process_readDWord(DFHackObject* p_Ptr, uint32_t address, uint32_t* value);
DFHACK_EXPORT int Process_writeDWord(DFHackObject* p_Ptr, uint32_t address, uint32_t value);

DFHACK_EXPORT int Process_readWord(DFHackObject* p_Ptr, uint32_t address, uint16_t* value);
DFHACK_EXPORT int Process_writeWord(DFHackObject* p_Ptr, uint32_t address, uint16_t value);

DFHACK_EXPORT int Process_readFloat(DFHackObject* p_Ptr, uint32_t address, float* value);

DFHACK_EXPORT int Process_readByte(DFHackObject* p_Ptr, uint32_t address, uint8_t* value);
DFHACK_EXPORT int Process_writeByte(DFHackObject* p_Ptr, uint32_t address, uint8_t value);

DFHACK_EXPORT uint8_t* Process_read(DFHackObject* p_Ptr, uint32_t address, uint32_t length);
DFHACK_EXPORT void Process_write(DFHackObject* p_Ptr, uint32_t address, uint32_t length, uint8_t* buffer);

DFHACK_EXPORT int Process_readSTLVector(DFHackObject* p_Ptr, uint32_t address, t_vecTriplet* vector);

DFHACK_EXPORT char* Process_readString(DFHackObject* p_Ptr, uint32_t offset);
DFHACK_EXPORT char* Process_getPath(DFHackObject* p_Ptr);
DFHACK_EXPORT char* Process_readClassName(DFHackObject* p_Ptr, uint32_t vptr);

DFHACK_EXPORT int Process_isSuspended(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isAttached(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isIdentified(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isSnapshot(DFHackObject* p_Ptr);

DFHACK_EXPORT uint32_t* Process_getThreadIDs(DFHackObject* p_Ptr);
DFHACK_EXPORT t_memrange* Process_getMemRanges(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_getPID(DFHackObject* p_Ptr, int32_t* pid);

DFHACK_EXPORT int Process_getModuleIndex(DFHackObject* p_Ptr, char* name, uint32_t version, uint32_t* output);

DFHACK_EXPORT int Process_SetAndWait(DFHackObject* p_Ptr, uint32_t state);

#ifdef __cplusplus
}
#endif

#endif
