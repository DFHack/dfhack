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

#include "dfhack-c/modules/Buildings_C.h"
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int Buildings_Start(DFHackObject* b_Ptr, uint32_t* numBuildings)
{
	if(b_Ptr != NULL)
	{
		return ((DFHack::Buildings*)b_Ptr)->Start(*numBuildings);
	}
	
	return -1;
}

int Buildings_Finish(DFHackObject* b_Ptr)
{
	if(b_Ptr != NULL)
	{
		return ((DFHack::Buildings*)b_Ptr)->Finish();
	}
	
	return -1;
}

int Buildings_Read(DFHackObject* b_Ptr, const uint32_t index, t_building* building)
{
	if(b_Ptr != NULL)
	{
		return ((DFHack::Buildings*)b_Ptr)->Read(index, *building);
	}
	
	return -1;
}

int Buildings_GetCustomWorkshopType(DFHackObject* b_Ptr, t_building* building)
{
	if(b_Ptr != NULL)
	{
		return ((DFHack::Buildings*)b_Ptr)->GetCustomWorkshopType(*building);
	}
	
	return -1;
}

t_customWorkshop* Buildings_ReadCustomWorkshopTypes(DFHackObject* b_Ptr)
{
    if(b_Ptr != NULL)
    {
        int i;
        t_customWorkshop* cw_Ptr = NULL;
        std::map<uint32_t, string> bTypes;
        map<uint32_t, string>::iterator bIter;

        if(!((DFHack::Buildings*)b_Ptr)->ReadCustomWorkshopTypes(bTypes))
            return NULL;
		
		(*alloc_customWorkshop_buffer_callback)(&cw_Ptr, bTypes.size());
		
		if(cw_Ptr == NULL)
			return NULL;
		
        for(i = 0, bIter = bTypes.begin(); bIter != bTypes.end(); bIter++, i++)
        {
            cw_Ptr[i].index = (*bIter).first;
            size_t length = (*bIter).second.copy(cw_Ptr[i].name, 256);
            cw_Ptr[i].name[length] = '\0';
        }
		
        return cw_Ptr;
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif
