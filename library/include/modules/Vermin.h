#pragma once
/**
 * \defgroup grp_vermin Wild vermin (ants, bees, etc)
 */
#include "Export.h"
#include "DataDefs.h"
#include "df/vermin.h"

namespace DFHack
{
namespace Vermin
{
    /**
     * Structure for holding a read DF vermin spawn point object
     * \ingroup grp_vermin
     */
    struct t_vermin
    {
        df::vermin * origin;
        int16_t race;
        int16_t caste;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint32_t countdown;
        bool visible:1;
        bool is_colony:1; /// Is vermin object a colony?
    };

    static const uint16_t TYPE_WILD_COLONY = 0xFFFF;
    /**
     * Get number of vermin objects
     */
    DFHACK_EXPORT uint32_t getNumVermin();
    /**
     * Read from vermin object
     */
    DFHACK_EXPORT bool Read (const uint32_t index, t_vermin & point);
    /**
     * Write into vermin object
     */
    DFHACK_EXPORT bool Write (const uint32_t index, t_vermin & point);
}// end DFHack::Vermin
}
