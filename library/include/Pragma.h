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

#ifndef DFHACK_TRANQUILITY
#define DFHACK_TRANQUILITY

#ifdef _MSC_VER
    // don't generate warnings because we're lazy about how we do exports
    #pragma warning( disable: 4251 )
    // do not issue warning for unknown pragmas (equivalent to gcc -Wno-unknown-pragmas)
    #pragma warning( disable: 4068 )
    // disable warnings regarding narrowing conversions (equivalent to gcc -Wno-conversion)
    #pragma warning( disable: 4244)
    // disable warnings regarding narrowing conversions of size_t
    #pragma warning( disable: 4267)
#endif

#endif
