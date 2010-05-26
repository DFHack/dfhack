#ifndef CL_MOD_WORLD
#define CL_MOD_WORLD

/*
* World: all kind of stuff related to the current world state
*/
#include "dfhack/DFExport.h"

namespace DFHack
{
    class DFContextPrivate;
    class DFHACK_EXPORT World
    {
        public:
        
        World(DFHack::DFContextPrivate * d);
        ~World();
        bool Start();
        bool Finish();
        
        uint32_t ReadCurrentTick();
        uint32_t ReadCurrentYear();
        uint32_t ReadCurrentMonth();
        uint32_t ReadCurrentDay();
        
        private:
        struct Private;
        Private *d;
    };
}
#endif

