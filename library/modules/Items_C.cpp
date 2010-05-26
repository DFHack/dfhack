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

#include "dfhack-c/modules/Items_C.h"

#ifdef __cplusplus
extern "C" {
#endif

char* Items_getItemDescription(DFHackObject* items, uint32_t itemptr, DFHackObject* mats)
{
	if(items != NULL && mats != NULL)
	{
		std::string desc = ((DFHack::Items*)items)->getItemDescription(itemptr, (DFHack::Materials*)mats);
		
		if(desc.size() > 0)
		{
			char* buf = (*alloc_char_buffer_callback)(desc.size());
			
			if(buf != NULL)
			{
				size_t len = desc.copy(buf, desc.size());
				
				if(len > 0)
					buf[len] = '\0';
				else
					buf[0] = '\0';
			}
			
			return buf;
		}
	}
	
	return NULL;
}

char* Items_getItemClass(DFHackObject* items, int32_t index)
{
	if(items != NULL)
	{
		std::string iclass = ((DFHack::Items*)items)->getItemClass(index);
		
		if(iclass.size() > 0)
		{
			char* buf = (*alloc_char_buffer_callback)(iclass.size());
			
			if(buf != NULL)
			{
				size_t len = iclass.copy(buf, iclass.size());
				
				if(len > 0)
					buf[len] = '\0';
				else
					buf[0] = '\0';
			}
			
			return buf;
		}
	}
	
	return NULL;
}

int Items_getItemData(DFHackObject* items, uint32_t itemptr, t_item* item)
{
	if(items != NULL)
	{
		return ((DFHack::Items*)items)->getItemData(itemptr, *item);
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
