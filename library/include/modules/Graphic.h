/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mr�zek (peterix@gmail.com)

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

/*******************************************************************************
                                    GRAPHIC
                            Changing tile cache
*******************************************************************************/
#pragma once
#ifndef CL_MOD_GRAPHIC
#define CL_MOD_GRAPHIC

#include <stdint.h>
#include "Export.h"
#include "Module.h"
#include <vector>

namespace DFHack
{
    // forward declaration used here instead of including DFSDL.h to reduce inclusion loading
    struct DFTileSurface;

    class DFHACK_EXPORT Graphic : public Module
    {
        public:
            bool Finish()
            {
                return true;
            }
            bool Register(DFTileSurface* (*func)(int,int));
            bool Unregister(DFTileSurface* (*func)(int,int));
            DFTileSurface* Call(int x, int y);

        private:
            std::vector<DFTileSurface* (*)(int, int)> funcs;
    };
}

#endif
