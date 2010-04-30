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

#include "Tranquility.h"
#include "Export.h"
#include <string>
#include <vector>
#include <map>
#include "integers.h"
#include "DFTileTypes.h"
#include "DFTypes.h"
#include "DFWindow.h"
#include "DFHackAPI.h"

using namespace std;
using namespace DFHack;

#include "DFHackAPI_C.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHackObject* API_Alloc(const char* path_to_xml)
{
	DFHack::API* api = new DFHack::API(std::string(path_to_xml));
	return (DFHackObject*)api;
}

void API_Free(DFHackObject* api)
{
	if(api != NULL)
	{
		delete api;
		api = NULL;
	}
}

int API_Attach(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->Attach())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_Detach(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->Detach())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_isAttached(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->isAttached())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_Suspend(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->Suspend())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_Resume(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->Resume())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_isSuspended(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->isSuspended())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_ForceResume(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->ForceResume())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int API_AsyncSuspend(DFHackObject* api)
{
	if(api != NULL)
	{
		if(((DFHack::API*)api)->AsyncSuspend())
			return 1;
		else
			return 0;
	}
	
	return -1;
}

void API_ReadRaw(DFHackObject* api, const uint32_t offset, const uint32_t size, uint8_t* target)
{
	if(api != NULL)
	{
		((DFHack::API*)api)->ReadRaw(offset, size, target);
	}
}

void API_WriteRaw(DFHackObject* api, const uint32_t offset, const uint32_t size, uint8_t* source)
{
	if(api != NULL)
	{
		((DFHack::API*)api)->WriteRaw(offset, size, source);
	}
}

#ifdef __cplusplus
}
#endif