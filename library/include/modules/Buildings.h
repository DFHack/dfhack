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
#ifndef CL_MOD_BUILDINGS
#define CL_MOD_BUILDINGS
/**
 * \defgroup grp_buildings Building module parts - also includes zones and stockpiles
 * @ingroup grp_modules
 */
#include "Export.h"
#include "Module.h"

#ifdef __cplusplus
namespace DFHack
{
#endif
    /**
     * Structure for holding a read DF building object
     * \ingroup grp_buildings
     */
    struct t_building
    {
        uint32_t x1;
        uint32_t y1;
        uint32_t x2;
        uint32_t y2;
        uint32_t z;
        t_matglossPair material;
        uint32_t type;
        int32_t custom_type;
        void * origin;
    };

#ifdef __cplusplus
    /**
     * The Buildings module - allows reading DF buildings
     * \ingroup grp_modules
     * \ingroup grp_buildings
     */
    class DFHACK_EXPORT Buildings : public Module
    {
        public:
        Buildings();
        ~Buildings();
        bool Start(uint32_t & numBuildings);
        // read one building at offset
        bool Read (const uint32_t index, t_building & building);
        bool Finish();

        // read mapping from custom_type value to building RAW name
        // custom_type of -1 implies ordinary building
        bool ReadCustomWorkshopTypes(std::map <uint32_t, std::string> & btypes);

        private:
        struct Private;
        Private *d;
    };
}
#endif // __cplusplus

#endif
