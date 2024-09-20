/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

// this makes everything that includes this file export symbols when using DFHACK_EXPORT (see DFExport.h)
#ifndef BUILD_DFHACK_LIB
    #define BUILD_DFHACK_LIB
#endif

// force large file support
#ifdef LINUX_BUILD
    #define _FILE_OFFSET_BITS 64
#endif

// C99 integer types
#include <stdint.h>
