/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#ifndef TILETYPES_H_INCLUDED
#define TILETYPES_H_INCLUDED

#ifdef LINUX_BUILD
#	ifndef DFHACKAPI
#		define DFHACKAPI
#	endif
#else
#	ifdef BUILD_DFHACK_LIB
#		ifndef DFHACKAPI
#			define DFHACKAPI extern "C" __declspec(dllexport)
#		endif
#	else
#		ifndef DFHACKAPI
#			define DFHACKAPI extern "C" __declspec(dllimport)
#		endif
#	endif
#endif

enum VegetationType{
    TREE_DEAD,
    TREE_OK,
    SAPLING_DEAD,
    SAPLING_OK,
    SHRUB_DEAD,
    SHRUB_OK
};

DFHACKAPI bool isWallTerrain(int in);
DFHACKAPI bool isFloorTerrain(int in);
DFHACKAPI bool isRampTerrain(int in);
DFHACKAPI bool isStairTerrain(int in);
DFHACKAPI bool isOpenTerrain(int in);
DFHACKAPI int getVegetationType(int in);
DFHACKAPI int picktexture(int in);

#endif // TILETYPES_H_INCLUDED
