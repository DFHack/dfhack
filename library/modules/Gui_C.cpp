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

#include "dfhack-c/modules/Gui_C.h"

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

int Gui_ReadPauseState(DFHackObject* gui)
{
	if(gui != NULL)
	{
		return ((DFHack::Gui*)gui)->ReadPauseState();
	}
	
	return -1;
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
