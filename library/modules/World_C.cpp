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

#include <vector>
#include <map>
using namespace std;
#include "dfhack-c/Common.h"
#include "dfhack/modules/World.h"
#include "dfhack-c/modules/World_C.h"
#include "dfhack-c/DFTypes_C.h"

#ifdef __cplusplus
extern "C" {
#endif

int World_Start(DFHackObject* world)
{
	if(world != NULL)
	{
		if(((DFHack::World*)world)->Start())
			return 1;
		else
			return 0;
	}

	return -1;
}

int World_Finish(DFHackObject* world)
{
	if(world != NULL)
	{
		if(((DFHack::World*)world)->Finish())
			return 1;
		else
			return 0;
	}

	return -1;
}

int World_ReadCurrentTick(DFHackObject* world, uint32_t* tick)
{
	if(world != NULL)
	{
		*tick = ((DFHack::World*)world)->ReadCurrentTick();
		return 1;
	}

	return -1;
}

int World_ReadCurrentYear(DFHackObject* world, uint32_t* year)
{
	if(world != NULL)
	{
		*year = ((DFHack::World*)world)->ReadCurrentYear();
		return 1;
	}

	return -1;
}

int World_ReadCurrentMonth(DFHackObject* world, uint32_t* month)
{
	if(world != NULL)
	{
		*month = ((DFHack::World*)world)->ReadCurrentMonth();
		return 1;
	}

	return -1;
}

int World_ReadCurrentDay(DFHackObject* world, uint32_t* day)
{
	if(world != NULL)
	{
		*day = ((DFHack::World*)world)->ReadCurrentDay();
		return 1;
	}

	return -1;
}

int World_ReadCurrentWeather(DFHackObject* world, uint8_t* weather)
{
    if(world != NULL)
    {
        *weather = ((DFHack::World*)world)->ReadCurrentWeather();
        return 1;
    }

    return -1;
}

int World_WriteCurrentWeather(DFHackObject* world, uint8_t weather)
{
    if(world != NULL)
    {
        ((DFHack::World*)world)->SetCurrentWeather(weather);
        return 1;
    }

    return -1;
}

int World_ReadGameMode(DFHackObject* world, t_gamemodes* modes)
{
	if(world != NULL)
	{
		if(((DFHack::World*)world)->ReadGameMode(*modes))
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int World_WriteGameMode(DFHackObject* world, t_gamemodes modes)
{
	if(world != NULL)
	{
		if(((DFHack::World*)world)->WriteGameMode(modes))
			return 1;
		else
			return 0;
	}
	
	return -1;
}

int World_ReadPauseState(DFHackObject* gui)
{
    if(gui != NULL)
    {
        return ((DFHack::World*)gui)->ReadPauseState();
    }
    
    return -1;
}

int World_SetPauseState(DFHackObject* gui, int8_t paused)
{
    if(gui != NULL)
    {
        bool pauseState = false;
        
        if(paused > 0)
            pauseState = true;
        
        ((DFHack::World*)gui)->SetPauseState(pauseState);
        
        return 1;
    }
    
    return -1;
}

#ifdef __cplusplus
}
#endif
