#pragma once
#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/**
 * \defgroup grp_vegetation Vegetation : stuff that grows and gets cut down or trampled by dwarves
 * @ingroup grp_modules
 */

#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
#include "dfhack/DFTypes.h"
namespace DFHack
{
    /**
     * \ingroup grp_vegetation
     */
    struct t_plant
    {
        // +0x3C
        #pragma pack(push, 1)
        union
        {
            uint16_t type;
            struct
            {
                unsigned int watery : 1;
                unsigned int is_shrub : 1;
                unsigned int unknown : 14;
            };
        };
        #pragma pack(pop)
        uint16_t material; // +0x3E
        uint16_t x; // +0x40
        uint16_t y; // +0x42
        uint16_t z; // +0x44
        uint16_t padding; // +0x46
        uint32_t unknown_1; // +0x48
        uint16_t temperature_1; // +0x4C
        uint16_t temperature_2; // +0x4E - maybe fraction?
        uint32_t is_burning; // 0x50: yes, just one flag
        uint32_t hitpoints; // 0x54
        uint32_t unknown_3; // 0x58
        // a vector is here
    };
    /**
     * Plant object read from the game
     * \ingroup grp_vegetation
     */
    struct dfh_plant
    {
        /// name of the plant
        t_name name;
        /// data with static size/address
        t_plant sdata;
        /// address where the plant was read from
        uint32_t address;
    };
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
        bool Start(uint32_t & numTrees);
        bool Read (const uint32_t index, dfh_plant & shrubbery);
        bool Write (dfh_plant & shrubbery);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
