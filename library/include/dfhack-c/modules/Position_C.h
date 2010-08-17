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

#ifndef POSITION_C_API
#define POSITION_C_API

#include "DFHack_C.h"
#include "dfhack/modules/Position.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int Position_getViewCoords(DFHackObject* pos, int32_t* x, int32_t* y, int32_t* z);
DFHACK_EXPORT int Position_setViewCoords(DFHackObject* pos, const int32_t x, const int32_t y, const int32_t z);

DFHACK_EXPORT int Position_getCursorCoords(DFHackObject* pos, int32_t* x, int32_t* y, int32_t* z);
DFHACK_EXPORT int Position_setCursorCoords(DFHackObject* pos, const int32_t x, const int32_t y, const int32_t z);

DFHACK_EXPORT int Position_getWindowSize(DFHackObject* pos, int32_t* width, int32_t* height);

#ifdef __cplusplus
}
#endif

#endif
