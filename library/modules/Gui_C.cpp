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

#include "dfhack-c/modules/Gui_C.h"
#include "dfhack-c/DFTypes_C.h"
#ifdef __cplusplus
extern "C" {
#endif

int Gui_Start(DFHackObject* gui)
{
	if(gui != NULL)
	{
		return ((DFHack::Gui*)gui)->Start();
	}
	
	return -1;
}

int Gui_Finish(DFHackObject* gui)
{
	if(gui != NULL)
	{
		return ((DFHack::Gui*)gui)->Finish();
	}
	
	return -1;
}

int Gui_getViewCoords(DFHackObject* gui, int32_t* x, int32_t* y, int32_t* z)
{
	if(gui != NULL)
	{
		int32_t tx, ty, tz;
		
		if(((DFHack::Gui*)gui)->getViewCoords(tx, ty, tz))
		{
			*x = tx;
			*y = ty;
			*z = tz;
			
			return 1;
		}
		else
			return 0;
	}
	
	return -1;
}

int Gui_setViewCoords(DFHackObject* gui, int32_t x, int32_t y, int32_t z)
{
	if(gui != NULL)
	{
		if(((DFHack::Gui*)gui)->setViewCoords(x, y, z))
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Gui_getCursorCoords(DFHackObject* gui, int32_t* x, int32_t* y, int32_t* z)
{
	if(gui != NULL)
	{
		int32_t tx, ty, tz;
		
		if(((DFHack::Gui*)gui)->getCursorCoords(tx, ty, tz))
		{
			*x = tx;
			*y = ty;
			*z = tz;
			
			return 1;
		}
		else
			return 0;
	}
	
	return -1;
}

int Gui_setCursorCoords(DFHackObject* gui, int32_t x, int32_t y, int32_t z)
{
	if(gui != NULL)
	{
		if(((DFHack::Gui*)gui)->setCursorCoords(x, y, z))
			return 1;
		else
			return 0;
	}
	
	return -1;
}

t_hotkey* Gui_ReadHotkeys(DFHackObject* gui)
{
	if(gui != NULL)
	{
		t_hotkey* buf;
		
		(*alloc_hotkey_buffer_callback)(&buf, NUM_HOTKEYS);
		
		if(buf != NULL)
		{
			if(((DFHack::Gui*)gui)->ReadHotkeys(buf))
				return buf;
			else
				return NULL;
		}
		else
			return NULL;
	}
	
	return NULL;
}

int Gui_getWindowSize(DFHackObject* gui, int32_t* width, int32_t* height)
{
    if(gui != NULL)
    {
        int32_t tw, th;
        
        if(((DFHack::Gui*)gui)->getWindowSize(tw, th))
        {
            *width = tw;
            *height = th;
            
            return 1;
        }
        else
            return 0;
    }

    return -1;
}

t_screen* Gui_getScreenTiles(DFHackObject* gui, int32_t width, int32_t height)
{
	if(gui != NULL)
	{
		t_screen* buf;
		
		(*alloc_screen_buffer_callback)(&buf, width * height);
		
		if(buf == NULL)
			return NULL;
		
		if(((DFHack::Gui*)gui)->getScreenTiles(width, height, buf))
			return buf;
		else
			return NULL;
	}
	
	return NULL;
}

int Gui_ReadViewScreen(DFHackObject* gui, t_viewscreen* viewscreen)
{
	if(gui != NULL)
	{
		return ((DFHack::Gui*)gui)->ReadViewScreen(*viewscreen);
	}
	
	return -1;
}

int Gui_ReadMenuState(DFHackObject* gui, uint32_t* menuState)
{
	if(gui != NULL)
	{
		*menuState = ((DFHack::Gui*)gui)->ReadMenuState();
		
		return 1;
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
