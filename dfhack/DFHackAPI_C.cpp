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

/*

void API_Free(DFHackObject* api)
{
	if(api != NULL)
	{
		delete api;
		api = NULL;
	}
}

bool API_Attach(DFHackObject* api)
{
	if(api != NULL)
		return ((DFHack::API*)api)->Attach();
}

bool API_Detach(DFHackObject* api)
{
	if(api != NULL)
		return ((DFHack::API*)api)->Detach();
}

bool API_isAttached(DFHackObject* api)
{
	if(api != NULL)
		return ((DFHack::API*)api)->isAttached();
}
*/

#ifdef __cplusplus
}
#endif