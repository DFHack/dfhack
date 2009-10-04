/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

/**
 * DO NOT USE THIS FILE DIRECTLY! USE MemAccess.h INSTEAD!
 */

// let's hope this commented crap is never needed
    /*
    char buffer[256];
    DWORD oldProtect = 0;
    DWORD numRead = 0;
    VirtualProtectEx( hProc, (LPVOID)0x77810F34, 256, PAGE_EXECUTE_READWRITE, &oldProtect );
    ReadProcessMemory( hProc, (LPVOID)0x77810F34, buffer, 256, &numRead );
    VirtualProtectEx( hProc, (LPVOID)0x77810F34, 256, oldProtect, NULL ); //restore the original protection when you're done
    */

// it would be possible to replace all this by macros

inline
uint8_t MreadByte (const uint32_t &offset)
{
    uint8_t result;
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint8_t), NULL);
    return result;
}


inline
void MreadByte (const uint32_t &offset,uint8_t &result)
{
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint8_t), NULL);
}


inline
uint16_t MreadWord (const uint32_t &offset)
{
    uint16_t result;
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint16_t), NULL);
    return result;
}


inline
void MreadWord (const uint32_t &offset, uint16_t &result)
{
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint16_t), NULL);
}


inline
uint32_t MreadDWord (const uint32_t &offset)
{
    uint32_t result;
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint32_t), NULL);
    return result;
}


inline
void MreadDWord (const uint32_t &offset, uint32_t &result)
{
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint32_t), NULL);
}


inline
uint64_t MreadQuad (const uint32_t &offset)
{
    uint64_t result;
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint64_t), NULL);
    return result;
}


inline
void MreadQuad (const uint32_t &offset, uint64_t &result)
{
    ReadProcessMemory(g_ProcessHandle, (int*) offset, &result, sizeof(uint64_t), NULL);
}


inline
void Mread (const uint32_t &offset, uint32_t size, uint8_t *target)
{
    ReadProcessMemory(g_ProcessHandle, (int*) offset, target, size, NULL);
}

// WRITING
inline
void MwriteDWord (const uint32_t offset, uint32_t data)
{
    WriteProcessMemory(g_ProcessHandle, (int*) offset, &data, sizeof(uint32_t), NULL);
}

// using these is expensive.
inline
void MwriteWord (uint32_t offset, uint16_t data)
{
    WriteProcessMemory(g_ProcessHandle, (int*) offset, &data, sizeof(uint16_t), NULL);
}

inline
void MwriteByte (uint32_t offset, uint8_t data)
{
    WriteProcessMemory(g_ProcessHandle, (int*) offset, &data, sizeof(uint8_t), NULL);
}

inline
void Mwrite (uint32_t offset, uint32_t size, uint8_t *source)
{
    WriteProcessMemory(g_ProcessHandle, (int*) offset, source, size, NULL);
}



///FIXME: reduce use of temporary objects
inline
const string MreadCString (const uint32_t &offset)
{
	string temp;
	char temp_c[256];
	DWORD read;
	ReadProcessMemory(g_ProcessHandle, (int *) offset, temp_c, 255, &read);
	temp_c[read+1] = 0;
	temp = temp_c;
	return temp;

	// I'll let the original code go down in infamy, and burn. Burn. BURN. YES, BURN! DIE YOU HORRIBLE, HORRIBLE, CODE!
	// Problems:
	//  * Does not check counter < 255, so potential buffer overrun.
	//  * Reads a single byte at a time, which is extremely inefficient (iirc, user -> kernel mode switch).
	//  * It adds an unnecessary null termination.
#if 0
    string temp;
    char temp_c[256];
    int counter = 0;
    char r;
    do
    {
        r = MreadByte(offset+counter);
        temp_c[counter] = r;
        counter++;
    } while (r);
    temp_c[counter] = 0;
    temp = temp_c;
    return temp;
#endif
}
