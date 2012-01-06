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
#ifndef CL_MOD_CONSTRUCTIONS
#define CL_MOD_CONSTRUCTIONS
/*
* DF constructions
*/
#include "Export.h"
#include "Module.h"

/**
 * \defgroup grp_constructions Construction module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    /**
     * type of item the construction is made of
     * \ingroup grp_constructions
     */
    enum e_construction_base
    {
        constr_bar = 0, /*!< Bars */
        constr_block = 2, /*!< Blocks */
        constr_boulder = 4, /*!< Rough stones or boulders */
        constr_logs = 5 /*!< Wooden logs */
    };
    #pragma pack(push, 1)
    /**
     * structure for holding a DF construction
     * \ingroup grp_constructions
     */
    struct t_construction
    {
        //0
        uint16_t x; /*!< X coordinate */
        uint16_t y; /*!< Y coordinate */
        // 4
        uint16_t z; /*!< Z coordinate */
        uint16_t form; /*!< type of item the construction is made of */
        // 8
        uint16_t unk_8; // = -1 in many cases
        uint16_t mat_type;
        // C
        uint32_t mat_idx;
        uint16_t unk3;
        // 10
        uint16_t unk4;
        uint16_t unk5;
        // 14
        uint32_t unk6;

        /// Address of the read object in DF memory. Added by DFHack.
        t_construction * origin;
    };
    #pragma pack (pop)
    class DFContextShared;
    /**
     * The Constructions module - allows reading constructed tiles (walls, floors, stairs, etc.)
     * \ingroup grp_modules
     * \ingroup grp_constructions
     */
    class DFHACK_EXPORT Constructions : public Module
    {
        public:
        Constructions();
        ~Constructions();
        bool Start(uint32_t & numConstructions);
        bool Read (const uint32_t index, t_construction & constr);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
