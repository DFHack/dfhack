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

#include "Internal.h"
#include "dfhack/DFTileTypes.h"
#include "dfhack-c/DFTileTypes_C.h"

int DFHack_isWallTerrain(int in)
{
    return DFHack::isWallTerrain(in);
}

int DFHack_isFloorTerrain(int in)
{
    return DFHack::isFloorTerrain(in);
}

int DFHack_isRampTerrain(int in)
{
    return DFHack::isRampTerrain(in);
}

int DFHack_isStairTerrain(int in)
{
    return DFHack::isStairTerrain(in);
}

int DFHack_isOpenTerrain(int in)
{
    return DFHack::isOpenTerrain(in);
}

int DFHack_getVegetationType(int in)
{
    return DFHack::tileShape(in);
}

int DFHack_getTileType(int index, TileRow* tPtr)
{
    if(index >= TILE_TYPE_ARRAY_LENGTH)
        return 0;

    *tPtr = tileTypeTable[index];
    return 1;
}
