#pragma once
/**
 * \defgroup grp_vermin Wild vermin (ants, bees, etc)
 */
#include "Export.h"
namespace DFHack { namespace Simple { namespace Vermin
{
    /**
     * Structure for holding a read DF vermin spawn point object
     * \ingroup grp_vermin
     */
    struct t_vermin
    {
        void * origin;
        int16_t race;
        uint16_t type;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        bool     in_use;
        uint8_t  unknown;
        uint32_t countdown;
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
    /**
     * Is vermin object a colony?
     */
    DFHACK_EXPORT bool isWildColony(t_vermin & point);
} } } // end DFHack::Simple::Vermin
