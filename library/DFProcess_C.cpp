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

#include <vector>
#include <algorithm>
#include <string>

using namespace std;

#include "dfhack/DFIntegers.h"
#include "dfhack-c/DFTypes_C.h"
#include "dfhack-c/DFProcess_C.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTR_CHECK if(p_Ptr == NULL) return -1;

int C_ProcessID_Compare(c_processID* left, c_processID* right)
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

FUNCTION_FORWARD(attach)
FUNCTION_FORWARD(detach)
FUNCTION_FORWARD(suspend)
FUNCTION_FORWARD(asyncSuspend)
FUNCTION_FORWARD(resume)
FUNCTION_FORWARD(forceresume)
FUNCTION_FORWARD(isSuspended)
FUNCTION_FORWARD(isAttached)
FUNCTION_FORWARD(isIdentified)
FUNCTION_FORWARD(isSnapshot)

#define MEMREAD_FUNC_FORWARD(func_name, type) \
int Process_##func_name(DFHackObject* p_Ptr, uint32_t address, type* value) \
{ \
	PTR_CHECK \
	((DFHack::Process*)p_Ptr)->func_name(address, *value); \
	return 1; \
}

#define MEMWRITE_FUNC_FORWARD(func_name, type) \
int Process_##func_name(DFHackObject* p_Ptr, uint32_t address, type value) \
{ \
	PTR_CHECK \
	((DFHack::Process*)p_Ptr)->func_name(address, value); \
	return 1; \
}

MEMREAD_FUNC_FORWARD(readQuad, uint64_t)
MEMWRITE_FUNC_FORWARD(writeQuad, uint64_t)

MEMREAD_FUNC_FORWARD(readDWord, uint32_t)
MEMWRITE_FUNC_FORWARD(writeDWord, uint32_t)

MEMREAD_FUNC_FORWARD(readWord, uint16_t)
MEMWRITE_FUNC_FORWARD(writeWord, uint16_t)

MEMREAD_FUNC_FORWARD(readFloat, float)

MEMREAD_FUNC_FORWARD(readByte, uint8_t)
MEMWRITE_FUNC_FORWARD(writeByte, uint8_t)

MEMREAD_FUNC_FORWARD(readSTLVector, t_vecTriplet);

uint8_t* Process_read(DFHackObject* p_Ptr, uint32_t address, uint32_t length)
{
	if(p_Ptr == NULL || alloc_ubyte_buffer_callback == NULL)
		return NULL;
	
	uint8_t* buf;
	
	((*alloc_ubyte_buffer_callback)(&buf, length));
	
	if(buf == NULL)
		return NULL;
	
	((DFHack::Process*)p_Ptr)->read(address, length, buf);
	
	return buf;
}

void Process_write(DFHackObject* p_Ptr, uint32_t address, uint32_t length, uint8_t* buffer)
{
	if(p_Ptr == NULL || buffer == NULL)
		return;
	
	((DFHack::Process*)p_Ptr)->write(address, length, buffer);
}

char* Process_readString(DFHackObject* p_Ptr, uint32_t offset)
{
	if(p_Ptr == NULL || alloc_char_buffer_callback == NULL)
		return NULL;
	
	std::string pString = ((DFHack::Process*)p_Ptr)->readSTLString(offset);
	
	if(pString.length() > 0)
	{
		size_t length = pString.length();
		
		char* buf;
		
		//add 1 for the null terminator
		((*alloc_char_buffer_callback)(&buf, length + 1));
		
		if(buf == NULL)
			return NULL;
		
		memset(buf, '\n', length + 1);
		
		pString.copy(buf, length);
		
		return buf;
	}
	else
		return "";
}

char* Process_getPath(DFHackObject* p_Ptr)
{
	if(p_Ptr == NULL || alloc_char_buffer_callback == NULL)
		return NULL;
	
	std::string pString = ((DFHack::Process*)p_Ptr)->getPath();
	
	if(pString.length() > 0)
	{
		size_t length = pString.length();
		
		char* buf;
		
		//add 1 for the null terminator
		((*alloc_char_buffer_callback)(&buf, length + 1));
		
		if(buf == NULL)
			return NULL;
		
		memset(buf, '\0', length + 1);
		
		pString.copy(buf, length);
		
		return buf;
	}
	else
		return "";
}

char* Process_readClassName(DFHackObject* p_Ptr, uint32_t vptr)
{
	if(p_Ptr == NULL || alloc_char_buffer_callback == NULL)
		return NULL;
	
	std::string cString = ((DFHack::Process*)p_Ptr)->readClassName(vptr);
	
	if(cString.length() > 0)
	{
		size_t length = cString.length();
		
		char* buf;
		
		//add 1 for the null terminator
		((*alloc_char_buffer_callback)(&buf, length + 1));
		
		if(buf == NULL)
			return NULL;
		
		memset(buf, '\0', length + 1);
		
		cString.copy(buf, length);
		
		return buf;
	}
	else
		return "";
}

uint32_t* Process_getThreadIDs(DFHackObject* p_Ptr)
{
	if(p_Ptr == NULL || alloc_uint_buffer_callback == NULL)
		return NULL;
	
	std::vector<uint32_t> threads;
	
	if(((DFHack::Process*)p_Ptr)->getThreadIDs(threads))
	{
		uint32_t* buf;
		
		if(threads.size() > 0)
		{
			((*alloc_uint_buffer_callback)(&buf, threads.size()));
			
			if(buf == NULL)
				return NULL;
			
			copy(threads.begin(), threads.end(), buf);
			
			return buf;
		}
	}
	else
		return NULL;
}

t_memrange* Process_getMemRanges(DFHackObject* p_Ptr)
{
	if(p_Ptr == NULL || alloc_memrange_buffer_callback == NULL)
		return NULL;
	
	std::vector<t_memrange> ranges;
	
	((DFHack::Process*)p_Ptr)->getMemRanges(ranges);
	
	if(ranges.size() > 0)
	{
		t_memrange* buf;
		uint32_t* rangeDescriptorBuf = (uint32_t*)calloc(ranges.size(), sizeof(uint32_t));
		
		for(uint32_t i = 0; i < ranges.size(); i++)
		{
			t_memrange* r = &ranges[i];
			
			rangeDescriptorBuf[i] = (uint32_t)(r->end - r->start);
		}
		
		((*alloc_memrange_buffer_callback)(&buf, rangeDescriptorBuf, ranges.size()));
		
		free(rangeDescriptorBuf);
		
		if(buf == NULL)
			return NULL;
		
		for(uint32_t i = 0; i < ranges.size(); i++)
		{
			t_memrange* r = &ranges[i];
			
			buf[i].start = r->start;
			buf[i].end = r->end;
			
			memset(buf[i].name, '\0', 1024);
			strncpy(buf[i].name, r->name, 1024);
			
			buf[i].read = r->read;
			buf[i].write = r->write;
			buf[i].execute = r->execute;
			buf[i].shared = r->shared;
			buf[i].valid = r->valid;
			
			memcpy(buf[i].buffer, r->buffer, r->end - r->start);
		}
		
		return buf;
	}
	else
		return NULL;
}

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
