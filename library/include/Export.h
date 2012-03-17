/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#ifdef LINUX_BUILD
    #ifndef DFHACK_EXPORT
        #define DFHACK_EXPORT __attribute__ ((visibility("default")))
    #endif
#else
    #ifdef BUILD_DFHACK_LIB
        #ifndef DFHACK_EXPORT
            #define DFHACK_EXPORT __declspec(dllexport)
        #endif
    #else
        #ifndef DFHACK_EXPORT
            #define DFHACK_EXPORT __declspec(dllimport)
        #endif
    #endif
#endif

// C export macros for faking SDL and plugin exports
#ifdef LINUX_BUILD
    #define DFhackCExport extern "C" __attribute__ ((visibility("default")))
    #define DFhackDataExport __attribute__ ((visibility("default")))
#else
    #define DFhackCExport extern "C" __declspec(dllexport)
    #define DFhackDataExport extern "C" __declspec(dllexport)
#endif
