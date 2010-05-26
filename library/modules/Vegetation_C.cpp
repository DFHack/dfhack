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

#include "dfhack/DFIntegers.h"
#include "dfhack/DFCommonInternal.h"
#include "dfhack/DFTypes.h"
#include "modules/Vegetation.h"
#include "modules/Vegetation_C.h"

using namespace DFHack;

#ifdef __cplusplus
extern "C" {
#endif

int Vegetation_Start(DFHackObject* veg, uint32_t* numTrees)
{
	if(veg != NULL)
	{
		return ((DFHack::Vegetation*)veg)->Start(*numTrees);
	}
	
	return -1;
}

int Vegetation_Finish(DFHackObject* veg)
{
	if(veg != NULL)
	{
		return ((DFHack::Vegetation*)veg)->Finish();
	}
	
	return -1;
}

int Vegetation_Read(DFHackObject* veg, const uint32_t index, t_tree* shrubbery)
{
	if(veg != NULL)
	{
		return ((DFHack::Vegetation*)veg)->Read(index, *shrubbery);
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
