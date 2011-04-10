/*
 * www.sourceforge.net/projects/dfhack
 * Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must
 * not claim that you wrote the original software. If you use this
 * software in a product, an acknowledgment in the product documentation
 * would be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

#ifndef DFHACK_C_API
#define DFHACK_C_API

#define HBUILD(a) a ## BufferCallback
#define HREG_MACRO(type_name, type) DFHACK_EXPORT void HBUILD(Register ## type_name) (int (*funcptr)(type, uint32_t));

#define HUNREG_MACRO(type_name) DFHACK_EXPORT void HBUILD(Unregister ## type_name) ();

#include "dfhack/DFPragma.h"

#include "dfhack/DFExport.h"
#include "dfhack/DFIntegers.h"

typedef void DFHackObject;

#ifdef __cplusplus

namespace DFHack {};
using namespace DFHack;
extern "C" {
    #endif
    // some global stuff here
    #ifdef __cplusplus
}
#endif

#endif
