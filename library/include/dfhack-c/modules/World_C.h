/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef WORLD_C_API
#define WORLD_C_API

#include "DFHack_C.h"
#include "dfhack/modules/World.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int World_Start(DFHackObject* world);
DFHACK_EXPORT int World_Finish(DFHackObject* world);

DFHACK_EXPORT int World_ReadCurrentTick(DFHackObject* world, uint32_t* tick);
DFHACK_EXPORT int World_ReadCurrentYear(DFHackObject* world, uint32_t* year);
DFHACK_EXPORT int World_ReadCurrentMonth(DFHackObject* world, uint32_t* month);
DFHACK_EXPORT int World_ReadCurrentDay(DFHackObject* world, uint32_t* day);

DFHACK_EXPORT int World_ReadCurrentWeather(DFHackObject* world, uint8_t* weather);
DFHACK_EXPORT int World_WriteCurrentWeather(DFHackObject* world, uint8_t weather);

DFHACK_EXPORT int World_ReadGameMode(DFHackObject* world, t_gamemodes*);
DFHACK_EXPORT int World_WriteGameMode(DFHackObject* world, t_gamemodes);

DFHACK_EXPORT int World_ReadPauseState(DFHackObject* gui);
DFHACK_EXPORT int World_SetPauseState(DFHackObject* gui, int8_t paused);
#ifdef __cplusplus
}
#endif

#endif
