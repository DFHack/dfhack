#ifndef CL_MOD_WORLD
#define CL_MOD_WORLD

/*
* World: all kind of stuff related to the current world state
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"

namespace DFHack
{
    enum WeatherType
    {
        CLEAR,
        RAINING,
        SNOWING
    };
    class DFContextShared;
    class DFHACK_EXPORT World : public Module
    {
        public:

        World(DFHack::DFContextShared * d);
        ~World();
        bool Start();
        bool Finish();

        uint32_t ReadCurrentTick();
        uint32_t ReadCurrentYear();
        uint32_t ReadCurrentMonth();
        uint32_t ReadCurrentDay();
        uint8_t ReadCurrentWeather();
        void SetCurrentWeather(uint8_t weather);

        private:
        struct Private;
        Private *d;
    };
}
#endif

