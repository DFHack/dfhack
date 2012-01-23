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
#ifndef CL_MOD_ENGRAVINGS
#define CL_MOD_ENGRAVINGS
/*
* DF engravings
*/
#include "Export.h"
#include "Module.h"

/**
 * \defgroup grp_engraving Engraving module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    /**
     * engraving flags
     * \ingroup grp_engraving
     */
    struct flg_engraving
    {
        // there are 9 directions an engraving can have.
        // unfortunately, a tile can't be engraved from more than one direction by the game
        unsigned int floor : 1; // engraved on a floor   0x1
        unsigned int west : 1; // engraved from west     0x2
        unsigned int east : 1; // engraved from east     0x4
        unsigned int north : 1; // engraved from north   0x8
        unsigned int south : 1; // engraved from south   0x10
        unsigned int hidden : 1; // hide the engraving   0x20
        unsigned int northwest : 1; // engraved from...  0x40
        unsigned int northeast : 1; // engraved from...  0x80
        unsigned int southwest : 1; // engraved from...  0x100
        unsigned int southeast : 1; // engraved from...  0x200
        unsigned int rest : 22; // probably unused
    };

    /**
     * type the engraving is made of
     * \ingroup grp_engraving
     */
    struct t_engraving
    {
        //0
        int32_t artistIdx; /*!< Index of the artist in some global vector */
        // 4
        int32_t unknownIdx; // likes to stay -1
        // 8
        uint32_t unknown1; // likes to stay 1
        // C
        uint16_t x; /*!< X coordinate */
        uint16_t y; /*!< Y coordinate */
        // 10
        uint16_t z; /*!< Z coordinate */
        uint16_t padding; /*!< Could be used for hiding values. */
        // 14
        flg_engraving flags; // 0x20 = hide symbol
        // 18
        uint8_t display_character; // really? 4 bytes for that?
        uint8_t padding2[3];
        // 1C
        uint32_t type; // possibly an enum, decides what vectors to use for imagery
        // 20
        int16_t subtype_idx; // index in a vector kind of deal related to previous value
        uint16_t quality; // from 0 to 5
        // 24
        uint32_t unknown2;
        // 28 = length
    };
    /**
     * structure for holding a DF engraving
     * \ingroup grp_engraving
     */
    struct dfh_engraving
    {
        t_engraving s;
        t_engraving * origin;
    };
    /**
     * The Engravings module - allows reading engravings :D
     * \ingroup grp_modules
     * \ingroup grp_engraving
     */
    class DFHACK_EXPORT Engravings : public Module
    {
        public:
        Engravings();
        ~Engravings();
        bool Start(uint32_t & numEngravings);
        bool Read (const uint32_t index, dfh_engraving & engr);
        bool Write (const dfh_engraving & engr);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
