#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/*
* DF vegetation - stuff that grows and gets cut down or trampled by dwarves
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
namespace DFHack
{
    /*
    types
    0: sapling?, dead sapling?, grown maple tree
    1: willow sapling?
    2: shrub
    3: shrub near water!
    */
    struct t_tree
    {
        uint16_t type; // +0x6C
        uint16_t material; // +0x6E
        uint16_t x; // +0x70
        uint16_t y; // +0x72
        uint16_t z; // +0x74
        /*
        junk_fill<0xA> junk;
        uint32_t flags; // +0x80 maybe?
        */
        uint32_t address;
    };
    
    class DFContextShared;
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
