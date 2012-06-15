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

#include "Pragma.h"
#include "Export.h"

#include "DataDefs.h"
#include "df/general_ref.h"
#include "df/specific_ref.h"

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

    DFHACK_EXPORT int getdir(std::string dir, std::vector<std::string> &files);
    DFHACK_EXPORT bool hasEnding (std::string const &fullString, std::string const &ending);

    DFHACK_EXPORT df::general_ref *findRef(std::vector<df::general_ref*> &vec, df::general_ref_type type);
    DFHACK_EXPORT bool removeRef(std::vector<df::general_ref*> &vec, df::general_ref_type type, int id);

    DFHACK_EXPORT df::specific_ref *findRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type);
    DFHACK_EXPORT bool removeRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type, void *ptr);
}// namespace DFHack