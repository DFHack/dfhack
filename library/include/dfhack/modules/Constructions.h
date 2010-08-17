#ifndef CL_MOD_CONSTRUCTIONS
#define CL_MOD_CONSTRUCTIONS
/*
* DF constructions
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
namespace DFHack
{
    // type of item the construction is made of
    enum e_construction_base
    {
        constr_bar = 0,
        constr_block = 2,
        constr_boulder = 4,
        constr_logs = 5
    };
    #pragma pack(push, 1)
    struct t_construction
    {
        //0
        uint16_t x;
        uint16_t y;
        // 4
        uint16_t z;
        uint16_t form; // e_construction_base
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
    #pragma pack (pop)
    class DFContextShared;
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
