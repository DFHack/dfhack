#ifndef CL_MOD_CREATURES
#define CL_MOD_CREATURES
/*
* Creatures
*/
#include "Export.h"
namespace DFHack
{
    class APIPrivate;
    struct t_creature;
    class DFHACK_EXPORT Creatures
    {
        public:
        Creatures(DFHack::APIPrivate * d);
        ~Creatures();
        bool Start( uint32_t & numCreatures);
        bool Finish();
        
        // Read creatures in a box, starting with index. Returns -1 if no more creatures
        // found. Call repeatedly do get all creatures in a specified box (uses tile coords)
        int32_t ReadCreatureInBox(const int32_t index, t_creature & furball,
                                  const uint16_t x1, const uint16_t y1,const uint16_t z1,
                                  const uint16_t x2, const uint16_t y2,const uint16_t z2);
        bool ReadCreature(const int32_t index, t_creature & furball);
        /// write labors of a creature (for Dwarf Therapist)
        //bool WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);
        
        private:
        struct Private;
        Private *d;
    };
}
#endif