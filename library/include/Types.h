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

#include <algorithm>

#include "Pragma.h"
#include "Export.h"

#include "DataDefs.h"

#include "df/general_ref_type.h"
#include "df/specific_ref_type.h"

namespace df {
    struct building;
    struct general_ref;
    struct specific_ref;
}

namespace DFHack
{
    struct t_matglossPair
    {
        int16_t type;
        int32_t index;
        bool operator<(const t_matglossPair &b) const
        {
            if (type != b.type) return (type < b.type);
            return (index < b.index);
        }
        bool operator==(const t_matglossPair &b) const
        {
            return (type == b.type) && (index == b.index);
        }
        bool operator!=(const t_matglossPair &b) const
        {
            return (type != b.type) || (index != b.index);
        }
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

    typedef std::pair<df::coord2d, df::coord2d> rect2d;

    inline rect2d intersect(rect2d a, rect2d b) {
        df::coord2d g1 = a.first, g2 = a.second;
        df::coord2d c1 = b.first, c2 = b.second;
        df::coord2d rc1 = df::coord2d(std::max(g1.x, c1.x), std::max(g1.y, c1.y));
        df::coord2d rc2 = df::coord2d(std::min(g2.x, c2.x), std::min(g2.y, c2.y));
        return rect2d(rc1, rc2);
    }

    inline rect2d mkrect_xy(int x1, int y1, int x2, int y2) {
        return rect2d(df::coord2d(x1, y1), df::coord2d(x2, y2));
    }

    inline rect2d mkrect_wh(int x, int y, int w, int h) {
        return rect2d(df::coord2d(x, y), df::coord2d(x+w-1, y+h-1));
    }

    inline df::coord2d rect_size(const rect2d &rect) {
        return rect.second - rect.first + df::coord2d(1,1);
    }

    DFHACK_EXPORT int getdir(std::string dir, std::vector<std::string> &files);
    DFHACK_EXPORT bool hasEnding (std::string const &fullString, std::string const &ending);

    DFHACK_EXPORT df::general_ref *findRef(std::vector<df::general_ref*> &vec, df::general_ref_type type);
    DFHACK_EXPORT bool removeRef(std::vector<df::general_ref*> &vec, df::general_ref_type type, int id);

    DFHACK_EXPORT df::item *findItemRef(std::vector<df::general_ref*> &vec, df::general_ref_type type);
    DFHACK_EXPORT df::building *findBuildingRef(std::vector<df::general_ref*> &vec, df::general_ref_type type);
    DFHACK_EXPORT df::unit *findUnitRef(std::vector<df::general_ref*> &vec, df::general_ref_type type);

    DFHACK_EXPORT df::specific_ref *findRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type);
    DFHACK_EXPORT bool removeRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type, void *ptr);
}// namespace DFHack
