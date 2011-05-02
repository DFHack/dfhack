#pragma once
#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/**
 * \defgroup grp_vegetation Vegetation : stuff that grows and gets cut down or trampled by dwarves
 * @ingroup grp_modules
 */

#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
namespace DFHack
{

    /**
     * \ingroup grp_vegetation
     */
    struct t_tree
    {
        // +0x6C
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
        uint16_t material; // +0x6E
        uint16_t x; // +0x70
        uint16_t y; // +0x72
        uint16_t z; // +0x74
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
        bool Read (const uint32_t index, t_tree & shrubbery);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
