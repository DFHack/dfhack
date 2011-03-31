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

#include "dfhack/DFIntegers.h"
#include "dfhack-c/DFProcess_C.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTR_CHECK if(p_Ptr == NULL) return -1;

int ProcessID_C_Compare(ProcessID_C* left, ProcessID_C* right)
{
	if(left == NULL || right == NULL)
		return PROCESSID_C_NULL;
	else
	{
		if(left->time == right->time && left->pid == right->pid)
			return PROCESSID_C_EQ;
		else if(left->time < right->time || left->pid < right->pid)
			return PROCESSID_C_LT;
		else
			return PROCESSID_C_GT;
	}
}

int MemRange_C_IsInRange(MemRange_C* range, uint64_t address)
{
	if(range == NULL)
		return -1;
	else if(address >= range->start && address <= range->end)
		return 1;
	else
		return 0;
}

#define FUNCTION_FORWARD(func_name) \
int Process_##func_name (DFHackObject* p_Ptr) \
{ \
	if(p_Ptr == NULL) \
		return -1; \
	if(((DFHack::Process*)p_Ptr)->func_name()) \
		return 1; \
	else \
		return 0; \
}

FUNCTION_FORWARD(Attach)
FUNCTION_FORWARD(Detach)
FUNCTION_FORWARD(Suspend)
FUNCTION_FORWARD(AsyncSuspend)
FUNCTION_FORWARD(Resume)
FUNCTION_FORWARD(ForceResume)
FUNCTION_FORWARD(isSuspended)
FUNCTION_FORWARD(isAttached)
FUNCTION_FORWARD(isIdentified)
FUNCTION_FORWARD(isSnapshot)

#define MEMREAD_FUNC_FORWARD(func_name, type) \
int Process_##func_name(DFHackObject* p_Ptr, uint32_t address, type* value) \
{ \
	PTR_CHECK \
	if(((DFHack::Process*)p_Ptr)->func_name(address, *value)) \
		return 1; \
	else \
		return 0;\
}

#define MEMWRITE_FUNC_FORWARD(func_name, type) \
int Process_##func_name(DFHackObject* p_Ptr, uint32_t address, type value) \
{ \
	PTR_CHECK \
	((DFHack::Process*)p_Ptr)->func_name(address, value) \
	return 1; \
}

MEMREAD_FUNC_FORWARD(readQuad, uint64_t)
MEMWRITE_FUNC_FORWARD(writeQuad, uint64_t)

MEMREAD_FUNC_FORWARD(readDWord, uint32_t)
MEMWRITE_FUNC_FORWARD(writeDWord, uint32_t)

MEMREAD_FUNC_FORWARD(readWord, uint16_t)
MEMWRITE_FUNC_FORWARD(writeWord, uint16_t)

MEMREAD_FUNC_FORWARD(readFloat, float)
MEMWRITE_FUNC_FORWARD(writeFloat, float)

MEMREAD_FUNC_FORWARD(readByte, uint8_t)
MEMWRITE_FUNC_FORWARD(writeByte, uint8_t)

int Process_getPID(DFHackObject* p_Ptr, int32_t* pid)
{
	if(p_Ptr == NULL)
	{
		*pid = -1;
		return -1;
	}
	else
	{
		*pid = ((DFHack::Process*)p_Ptr)->getPID();
		
		return 1;
	}
}

int Process_getModuleIndex(DFHackObject* p_Ptr, char* name, uint32_t version, uint32_t* output)
{
	PTR_CHECK
	
	if(((DFHack::Process*)p_Ptr)->getModuleIndex(name, version, *output))
		return 1;
	else
		return 0;
}

int Process_SetAndWait(DFHackObject* p_Ptr, uint32_t state)
{
	PTR_CHECK
	
	if(((DFHack::Process*)p_Ptr)->SetAndWait(state))
		return 1;
	else
		return 0;
}

#ifdef __cplusplus
}
#endif