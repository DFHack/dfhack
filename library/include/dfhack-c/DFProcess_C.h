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

struct ProcessID_C
{
	uint64_t time;
	uint64_t pid;
};

#define PROCESSID_C_NULL	-2
#define PROCESSID_C_LT		-1
#define PROCESSID_C_EQ		0
#define PROCESSID_C_GT		1

DFHACK_EXPORT extern int ProcessID_C_Compare(ProcessID_C* left, ProcessID_C* right);

struct MemRange_C
{
	uint64_t start;
	uint64_t end;
	
	char name[1024];
	uint8_t flags;
	
	uint8_t* buffer;
};

#define MEMRANGE_CAN_READ(x) (x.flags & 0x01)
#define MEMRANGE_CAN_WRITE(x) (x.flags & 0x02)
#define MEMRANGE_CAN_EXECUTE(x) (x.flags & 0x04)
#define MEMRANGE_IS_SHARED(x) (x.flags & 0x08)
#define MEMRANGE_IS_VALID(x) (x.flags & 0x10)

#define MEMRANGE_SET_READ(x, i) (x.flags |= 0x01)
#define MEMRANGE_SET_WRITE(x, i) (x.flags |= 0x02)
#define MEMRANGE_SET_EXECUTE(x, i) (x.flags |= 0x04)
#define MEMRANGE_SET_SHARED(x, i) (x.flags |= 0x08)
#define MEMRANGE_SET_VALID(x, i) (x.flags |= 0x10)

DFHACK_EXPORT extern int MemRange_C_IsInRange(MemRance_C* range, uint64_t address);

DFHACK_EXPORT int Process_Attach(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_Detach(DFHackObject* p_Ptr);

DFHACK_EXPORT int Process_Suspend(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_AsyncSuspend(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_Resume(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_ForceResume(DFHackObject* p_Ptr);

DFHACK_EXPORT int Process_readQuad(DFHackObject* p_Ptr, uint32_t address, uint64_t* value);
DFHACK_EXPORT int Process_writeQuad(DFHackObject* p_Ptr, uint32_t address, uint64_t value);

DFHACK_EXPORT int Process_readDWord(DFHackObject* p_Ptr, uint32_t address, uint32_t* value);
DFHACK_EXPORT int Process_writeDWord(DFHackObject* p_Ptr, uint32_t address, uint32_t value);

DFHACK_EXPORT int Process_readWord(DFHackObject* p_Ptr, uint32_t address, uint16_t* value);
DFHACK_EXPORT int Process_writeWord(DFHackObject* p_Ptr, uint32_t address, uint16_t value);

DFHACK_EXPORT int Process_readFloat(DFHackObject* p_Ptr, uint32_t address, float* value);
DFHACK_EXPORT int Process_writeFloat(DFHackObject* p_Ptr, uint32_t address, float value);

DFHACK_EXPORT int Process_readByte(DFHackObject* p_Ptr, uint32_t address, uint8_t* value);
DFHACK_EXPORT int Process_writeByte(DFHackObject* p_Ptr, uint32_t address, uint8_t value);

DFHACK_EXPORT int Process_isSuspended(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isAttached(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isIdentified(DFHackObject* p_Ptr);
DFHACK_EXPORT int Process_isSnapshot(DFHackObject* p_Ptr);

DFHACK_EXPORT uint32_t* Process_getThreadIDs(DFHackObject* p_Ptr);
DFHACK_EXPORT int getPID(DFHackObject* p_Ptr, int32_t* pid);

DFHACK_EXPORT int Process_getModuleIndex(DFHackObject* p_Ptr, char* name, uint32_t version, uint32_t* output);

DFHACK_EXPORT int Process_SetAndWait(DFHackObject* p_Ptr, uint32_t state);

#ifdef __cplusplus
}
#endif

#endif
