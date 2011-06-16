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

#ifndef DFHACK_TRANQUILITY
#define DFHACK_TRANQUILITY

// This is here to keep MSVC from spamming the build output with nonsense
// Call it public domain.

#ifdef _MSC_VER
    // don't spew nonsense
    #pragma warning( disable: 4251 )
    // don't display bogus 'deprecation' and 'unsafe' warnings.
    // See the idiocy: http://msdn.microsoft.com/en-us/magazine/cc163794.aspx
    #define _CRT_SECURE_NO_DEPRECATE
    #define _SCL_SECURE_NO_DEPRECATE
    #pragma warning( disable: 4996 )
    // Let me demonstrate:
    /**
     * [peterix@peterix dfhack]$ man wcscpy_s
     * No manual entry for wcscpy_s
     * 
     * Proprietary extensions.
     */
    // disable stupid
    #pragma warning( disable: 4800 )
    // disable more stupid
    #pragma warning( disable: 4068 )
#endif

#endif
