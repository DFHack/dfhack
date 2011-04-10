/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#pragma once

#ifndef DFCOMMONINTERNAL_H_INCLUDED
#define DFCOMMONINTERNAL_H_INCLUDED

// this makes everything that includes this file export symbols when using DFHACK_EXPORT (see DFExport.h)
#ifndef BUILD_DFHACK_LIB
    #define BUILD_DFHACK_LIB
#endif

// wizardry for adding quotes to macros
#define _QUOTEME(x) #x
#define QUOT(x) _QUOTEME(x)

// force large file support
#ifdef LINUX_BUILD
    #define _FILE_OFFSET_BITS 64
#endif

// one file for telling the MSVC compiler where it can shove its pointless warnings
#include "dfhack/DFPragma.h"

// C99 integer types
#include "dfhack/DFIntegers.h"

#endif // DFCOMMONINTERNAL_H_INCLUDED

