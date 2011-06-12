#pragma once
#ifndef CL_MOD_CONSTRUCTIONS
#define CL_MOD_CONSTRUCTIONS
/*
* DF constructions
*/
#include "dfhack/Export.h"
#include "dfhack/Module.h"

/**
 * \defgroup grp_constructions Construction module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    /**
     * type of item the construction is made of
     * \ingroup grp_constructions
     */
    enum e_construction_base
    {
        constr_bar = 0, /*!< Bars */
        constr_block = 2, /*!< Blocks */
        constr_boulder = 4, /*!< Rough stones or boulders */
        constr_logs = 5 /*!< Wooden logs */
    };
    #pragma pack(push, 1)
    /**
     * structure for holding a DF construction
     * \ingroup grp_constructions
     */
    struct t_construction
    {
        //0
        uint16_t x; /*!< X coordinate */
        uint16_t y; /*!< Y coordinate */
        // 4
        uint16_t z; /*!< Z coordinate */
        uint16_t form; /*!< type of item the construction is made of */
        // 8
        uint16_t unk_8; // = -1 in many cases
        uint16_t mat_type;
        // C
        uint32_t mat_idx;
        uint16_t unk3;
        // 10
        uint16_t unk4;
        uint16_t unk5;
        // 14
        uint32_t unk6;

        /// Address of the read object in DF memory. Added by DFHack.
        uint32_t origin;
    };
    #pragma pack (pop)
    class DFContextShared;
    /**
     * The Constructions module - allows reading constructed tiles (walls, floors, stairs, etc.)
     * \ingroup grp_modules
     * \ingroup grp_constructions
     */
    class DFHACK_EXPORT Constructions : public Module
    {
        public:
        Constructions(DFContextShared * d);
        ~Constructions();
        bool Start(uint32_t & numConstructions);
        bool Read (const uint32_t index, t_construction & constr);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
