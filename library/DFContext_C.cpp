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

#include "dfhack/DFPragma.h"
#include "dfhack/DFExport.h"
#include <string>
#include <vector>
#include <map>
#include "dfhack/DFIntegers.h"
#include "dfhack/DFTileTypes.h"
#include "dfhack/DFTypes.h"
//#include "dfhack/modules/WindowIO.h"
#include "dfhack/DFContextManager.h"
#include "dfhack/DFContext.h"

using namespace std;
using namespace DFHack;

#include "dfhack-c/DFContext_C.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHackObject* ContextManager_Alloc(const char* path_to_xml)
{
    DFHack::ContextManager* contextMgr = new DFHack::ContextManager(std::string(path_to_xml));
    return (DFHackObject*)contextMgr;
}

//FIXME: X:\dfhack\DFHackContext_C.cpp:56: warning: deleting `DFHackObject* ' is undefined
//DC:  Yeah, I forgot that trying to delete a void pointer might be a bad idea.  This works now.
void ContextManager_Free(DFHackObject* contextMgr)
{
    if(contextMgr != NULL)
    {
		DFHack::ContextManager* a = (DFHack::ContextManager*)contextMgr;
        delete a;
		
        contextMgr = NULL;
    }
}

int ContextManager_Refresh(DFHackObject* contextMgr)
{
	if(contextMgr != NULL)
	{
		return ((DFHack::ContextManager*)contextMgr)->Refresh();
	}
	
	return -1;
}

int ContextManager_size(DFHackObject* contextMgr, uint32_t* size)
{
	if(contextMgr != NULL)
	{
		uint32_t result = ((DFHack::ContextManager*)contextMgr)->size();
		
		*size = result;
		
		return 1;
	}
	
	return -1;
}

int ContextManager_purge(DFHackObject* contextMgr)
{
	if(contextMgr != NULL)
	{
		((DFHack::ContextManager*)contextMgr)->purge();
		
		return 1;
	}
	
	return -1;
}

DFHackObject* ContextManager_getContext(DFHackObject* contextMgr, uint32_t index)
{
	if(contextMgr != NULL)
	{
		DFHack::ContextManager* mgr = ((DFHack::ContextManager*)contextMgr);
		
		if(index >= mgr->size())
			return NULL;
		
		return (DFHackObject*)((DFHack::Context*)((*mgr)[index]));
	}
	
	return NULL;
}

DFHackObject* ContextManager_getSingleContext(DFHackObject* contextMgr)
{
	if(contextMgr != NULL)
	{
		return (DFHackObject*)((DFHack::ContextManager*)contextMgr)->getSingleContext();
	}
	
	return NULL;
}

void Context_Free(DFHackObject* context)
{
	if(context != NULL)
	{
		DFHack::Context* c = (DFHack::Context*)context;		
		delete c;
		
		context = NULL;
	}
}

int Context_Attach(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->Attach();
    }
	
    return -1;
}

int Context_Detach(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->Detach();
    }

    return -1;
}

int Context_isAttached(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->isAttached();
    }

    return -1;
}

int Context_Suspend(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->Suspend();
    }

    return -1;
}

int Context_Resume(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->Resume();
    }

    return -1;
}

int Context_isSuspended(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->isSuspended();
    }

    return -1;
}

int Context_ForceResume(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->ForceResume();
    }

    return -1;
}

int Context_AsyncSuspend(DFHackObject* context)
{
    if(context != NULL)
    {
        return ((DFHack::Context*)context)->AsyncSuspend();
    }

    return -1;
}

//module getters

DFHackObject* Context_getMemoryInfo(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getMemoryInfo();
	}
	
	return NULL;
}

DFHackObject* Context_getProcess(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getProcess();
	}
	
	return NULL;
}

DFHackObject* Context_getWindow(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getWindowIO();
	}
	
	return NULL;
}

DFHackObject* Context_getCreatures(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getCreatures();
	}
	
	return NULL;
}

DFHackObject* Context_getMaps(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getMaps();
	}
	
	return NULL;
}

DFHackObject* Context_getGui(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getGui();
	}
	
	return NULL;
}

DFHackObject* Context_getPosition(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getPosition();
	}
	
	return NULL;
}

DFHackObject* Context_getMaterials(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getMaterials();
	}
	
	return NULL;
}

DFHackObject* Context_getTranslation(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getTranslation();
	}
	
	return NULL;
}

DFHackObject* Context_getVegetation(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getVegetation();
	}
	
	return NULL;
}

DFHackObject* Context_getBuildings(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getBuildings();
	}
	
	return NULL;
}

DFHackObject* Context_getConstructions(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getConstructions();
	}
	
	return NULL;
}

DFHackObject* Context_getItems(DFHackObject* context)
{
	if(context != NULL)
	{
		return (DFHackObject*)((DFHack::Context*)context)->getItems();
	}
	
	return NULL;
}

void Context_ReadRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* target)
{
    if(context != NULL)
    {
        ((DFHack::Context*)context)->ReadRaw(offset, size, target);
    }
}

void Context_WriteRaw(DFHackObject* context, const uint32_t offset, const uint32_t size, uint8_t* source)
{
    if(context != NULL)
    {
        ((DFHack::Context*)context)->WriteRaw(offset, size, source);
    }
}

#ifdef __cplusplus
}
#endif
