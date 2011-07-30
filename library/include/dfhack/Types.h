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

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"

namespace DFHack
{
    struct t_matglossPair
    {
        int16_t type;
        int32_t index;
    };

    template <int SIZE>
    struct junk_fill
    {
        uint8_t data[SIZE];
    };

    struct df_name
    {
        std::string first_name;
        std::string nick_name;
        int32_t words[7];
        int16_t parts_of_speech[7];
        int32_t language;
        int16_t unknown;
        int16_t has_name;
    };

    // DF effects, by darius from the bay12 forum
    enum EFFECT_TYPE
    {
        EFF_MIASMA=0,
        EFF_WATER,
        EFF_WATER2,
        EFF_BLOOD,
        EFF_DUST,
        EFF_MAGMA,
        EFF_SMOKE,
        EFF_DRAGONFIRE,
        EFF_FIRE,
        EFF_WEBING,
        EFF_BOILING, // uses matgloss
        EFF_OCEANWAVE
    };

    struct t_effect_df40d //size 40
    {
        uint16_t type;
        t_matglossPair material;
        int16_t lifetime;
        uint16_t x;
        uint16_t y;
        uint16_t z; //14
        int16_t x_direction;
        int16_t y_direction;
        junk_fill <12> unk4;
        uint8_t canCreateNew;//??
        uint8_t isHidden;
    };

    struct t_name
    {
        char first_name[128];
        char nickname[128];
        int32_t words[7];
        uint16_t parts_of_speech[7];
        uint32_t language;
        bool has_name;
    };

    struct t_attrib
    {
        uint32_t level;
        uint32_t field_4; // offset from beginning, purpose unknown
        uint32_t field_8;
        uint32_t field_C;
        uint32_t leveldiff;
        uint32_t field_14;
        uint32_t field_18;
    };

    struct t_level
    {
        uint32_t level;
        std::string name;
        uint32_t xpNxtLvl;
    };
}// namespace DFHack