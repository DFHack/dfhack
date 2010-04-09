#ifndef CL_MOD_CONSTRUCTIONS
#define CL_MOD_CONSTRUCTIONS
/*
* DF constructions
*/
#include "Export.h"
namespace DFHack
{
    // type of item the construction is made of
    enum e_construction_base
    {
        constr_bar = 0,
        constr_block = 2,
        constr_boulder = 4,
        constr_logs = 5,
    };
    
    struct t_construction
    {
        //0
        uint16_t x;
        uint16_t y;
        // 4
        uint16_t z;
        e_construction_base type :16; // 4 = 'rough'
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
        
        // added later by dfhack
        uint32_t origin;
    };

    struct APIPrivate;
    class DFHACK_EXPORT Constructions
    {
        public:
        Constructions(APIPrivate * d);
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
