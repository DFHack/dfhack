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
#include <string>
#include <map>
#include <vector>
using namespace std;
#include "dfhack-c/modules/Constructions_C.h"

#ifdef __cplusplus
extern "C" {
#endif

int Constructions_Start(DFHackObject* c_Ptr, uint32_t* numConstructions)
{
	if(c_Ptr != NULL)
	{
		return ((DFHack::Constructions*)c_Ptr)->Start(*numConstructions);
	}
	
	return -1;
}

int Constructions_Finish(DFHackObject* c_Ptr)
{
	if(c_Ptr != NULL)
	{
		return ((DFHack::Constructions*)c_Ptr)->Finish();
	}
	
	return -1;
}

int Constructions_Read(DFHackObject* c_Ptr, const uint32_t index, t_construction* construction)
{
	if(c_Ptr != NULL)
	{
		return ((DFHack::Constructions*)c_Ptr)->Read(index, *construction);
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
