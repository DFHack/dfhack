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
#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/**
 * \defgroup grp_vegetation Vegetation : stuff that grows and gets cut down or trampled by dwarves
 * @ingroup grp_modules
 */

#include "Export.h"
#include "Module.h"
#include "Types.h"
namespace DFHack
{
    /**
     * \ingroup grp_vegetation
     */
    const uint32_t sapling_to_tree_threshold = 0x1D880;
    /**
     * \ingroup grp_vegetation
     */
    #pragma pack(push, 2)
    struct df_plant
    {
        df_name name;
        union
        {
            uint16_t type;
            struct
            {
                bool watery : 1;
                bool is_shrub : 1;
            };
        };
        uint16_t material; // +0x3E
        uint16_t x; // +0x40
        uint16_t y; // +0x42
        uint16_t z; // +0x44
        uint16_t padding; // +0x46
        uint32_t grow_counter; // +0x48
        uint16_t temperature_1; // +0x4C
        uint16_t temperature_2; // +0x4E - maybe fraction?
        uint32_t is_burning; // 0x50: yes, just one flag
        uint32_t hitpoints; // 0x54
        /**
         * 0x58 - the updates are staggered into 9? groups. this seems to be what differentiates the plants.
         */
        uint16_t update_order;
        uint16_t padding2;
        // a vector is here
        // some more temperature stuff after that
    };
    #pragma pack(pop)
    /**
     * The Vegetation module
     * \ingroup grp_vegetation
     * \ingroup grp_modules
     */
    class DFHACK_EXPORT Vegetation : public Module
    {
        public:
        Vegetation();
        ~Vegetation();
        bool Finish(){return true;};
        std::vector <df_plant *> *all_plants;
    };
}
#endif
