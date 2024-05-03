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

#ifdef LINUX_BUILD
    #ifndef DFHACK_EXPORT
        #define DFHACK_EXPORT __attribute__ ((visibility("default")))
    #endif
    #ifndef DFHACK_IMPORT
        #define DFHACK_IMPORT DFHACK_EXPORT
    #endif
#else
    #ifdef BUILD_DFHACK_LIB
        #ifndef DFHACK_EXPORT
            #define DFHACK_EXPORT __declspec(dllexport)
        #endif
        #ifndef DFHACK_IMPORT
            #define DFHACK_IMPORT
        #endif
    #else
        #ifndef DFHACK_EXPORT
            #define DFHACK_EXPORT __declspec(dllimport)
        #endif
        #ifndef DFHACK_IMPORT
            #define DFHACK_IMPORT DFHACK_EXPORT
        #endif
    #endif
#endif

// C export macros
#ifdef LINUX_BUILD
    #define DFhackCExport extern "C" __attribute__ ((visibility("default")))
    #define DFhackDataExport __attribute__ ((visibility("default")))
#else
    #define DFhackCExport extern "C" __declspec(dllexport)
    #define DFhackDataExport extern "C" __declspec(dllexport)
#endif

// Make gcc warn if types and format string don't match for printf
#ifdef __GNUC__
    //! Tell GCC about format functions to allow parameter strict type checks
    //! \param type The type of function can be printf, scanf, strftime or strfmon
    //! \param fmtstr One based position index for format parameter
    //! \param vararg One based position index for the first checked parameter
    #define Wformat(type, fmtstr, vararg) __attribute__ ((format (type, fmtstr, vararg)))
#else
    #define Wformat(type, fmtstr, vararg)
#endif
