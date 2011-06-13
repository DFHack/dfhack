#pragma once
#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/**
 * \defgroup grp_vegetation Vegetation : stuff that grows and gets cut down or trampled by dwarves
 * @ingroup grp_modules
 */

#include "dfhack/Export.h"
#include "dfhack/Module.h"
#include "dfhack/Types.h"
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
    class DFContextShared;
    /**
     * The Vegetation module
     * \ingroup grp_vegetation
     * \ingroup grp_modules
     */
    class DFHACK_EXPORT Vegetation : public Module
    {
        public:
        Vegetation(DFContextShared * d);
        ~Vegetation();
        bool Finish(){return true;};
        std::vector <df_plant *> *all_plants;
    };
}
#endif
