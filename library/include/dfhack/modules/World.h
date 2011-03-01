#ifndef CL_MOD_WORLD
#define CL_MOD_WORLD

/*
* World: all kind of stuff related to the current world state
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
#include <ostream>

namespace DFHack
{
    enum WeatherType
    {
        CLEAR,
        RAINING,
        SNOWING
    };
    enum ControlMode
    {
        CM_Managing = 0,
        CM_DirectControl = 1,
        CM_Kittens = 2, // not sure yet, but I think it will involve kittens
        CM_Menu = 3,
        CM_MAX = 3
    };
    enum GameMode
    {
        GM_Fort = 0,
        GM_Adventurer = 1,
        GM_Kittens = 2, // not sure yet, but I think it will involve kittens
        GM_Menu = 3,
        GM_Arena = 4,
        GM_MAX
    };
    struct t_gamemodes
    {
        ControlMode control_mode;
        GameMode game_mode;
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
        bool ReadGameMode(t_gamemodes& rd);
        bool WriteGameMode(const t_gamemodes & wr); // this is very dangerous
        private:
        struct Private;
        Private *d;
    };
}
#endif

