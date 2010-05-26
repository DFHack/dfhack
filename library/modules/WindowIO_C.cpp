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

using namespace std;

#include "dfhack/DFIntegers.h"
#include "DFHack.h"
#include "DFHack_C.h"
#include "dfhack/modules/WindowIO.h"
#include "dfhack-c/modules/WindowIO_C.h"

using namespace DFHack;


#ifdef __cplusplus
extern "C" {
#endif

int WindowIO_TypeStr(DFHackObject* window, const char* input, int delay, bool useShift)
{
	if(window != NULL)
	{
		((DFHack::WindowIO*)window)->TypeStr(input, delay, useShift);
		return 1;
	}
	
	return -1;
}

int WindowIO_TypeSpecial(DFHackObject* window, t_special command, int count, int delay)
{
	if(window != NULL)
	{
		((DFHack::WindowIO*)window)->TypeSpecial(command, count, delay);
		return 1;
	}
	
	return -1;
}

#ifdef __cplusplus
}
#endif
