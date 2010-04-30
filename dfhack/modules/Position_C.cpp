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

#include "modules/Position_C.h"
#include "integers.h"

#include "modules/Position.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

int Position_getViewCoords(DFHackObject* pos, int32_t* x, int32_t* y, int32_t* z)
{
	if(pos != 0)
	{
		int32_t tx, ty, tz;
		
		if(((DFHack::Position*)pos)->getViewCoords(tx, ty, tz))
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

int Position_setViewCoords(DFHackObject* pos, int32_t x, int32_t y, int32_t z)
{
	if(pos != 0)
	{
		if(((DFHack::Position*)pos)->setViewCoords(x, y, z))
			return 1;
		else
			return 0;
	}
	
	return -1;
}


int Position_getCursorCoords(DFHackObject* pos, int32_t* x, int32_t* y, int32_t* z)
{
	if(pos != 0)
	{
		int32_t tx, ty, tz;
		
		if(((DFHack::Position*)pos)->getCursorCoords(tx, ty, tz))
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

int Position_setCursorCoords(DFHackObject* pos, int32_t x, int32_t y, int32_t z)
{
	if(pos != 0)
	{
		if(((DFHack::Position*)pos)->setCursorCoords(x, y, z))
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int Position_getWindowSize(DFHackObject* pos, int32_t* width, int32_t* height)
{
	if(pos != 0)
	{
		int32_t tw, th;
		
		if(((DFHack::Position*)pos)->getWindowSize(tw, th))
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

#ifdef __cplusplus
}
#endif