// vim: sts=4 sta et shiftwidth=4:
#include "dfhack/DFIntegers.h"
#include "dfhack/DFTileTypes.h"
#include "dfhack/DFExport.h"

namespace DFHack {
    //set tile class string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileClassString[tileclass_count+1] = {
        TILECLASS_MACRO
            0
    };
#undef X

    //string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileMaterialString[tilematerial_count+1] = {
        TILEMATERIAL_MACRO
            0
    };
#undef X

    //string lookup table (e.g. for printing to user)
#define X(name,comment) #name,
    const char * TileSpecialString[tilespecial_count+1] = {
        TILESPECIAL_MACRO
            0
    };
#undef X

}
