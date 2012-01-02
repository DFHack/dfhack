// Prints all the Tile Types known by DFHack.
// File is both fixed-field and CSV parsable.

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
using namespace std;

#include <DFHack.h>
#include <DFTileTypes.h>

using namespace DFHack;

int main (int argc, char **argv)
{
    FILE *f=stdout;
    const int Columns = 7;
    const char * Headings[Columns] = {"TileTypeID","Class","Material","V","Special","Direction","Description"};
    size_t Size[ Columns ] = {};
    int i;

    //First, figure out column widths.
    for(i=0;i<Columns;++i)
    {
        Size[i]=strlen(Headings[i])+1;
    }

    //Classes
    fprintf(f,"\nTile Type Classes:\n");
    for(i=0;i<tileshape_count;++i)
    {
        Size[1]=max<size_t>(Size[1],strlen(TileShapeString[i]));
        fprintf(f,"%4i ; %s\n", i, TileShapeString[i] ,0 );
    }

    //Materials
    fprintf(f,"\nTile Type Materials:\n");
    for(i=0;i<tilematerial_count;++i)
    {
        Size[2]=max<size_t>(Size[2],strlen(TileMaterialString[i]));
        fprintf(f,"%4i ; %s\n", i, TileMaterialString[i] ,0 );
    }

    //Specials
    fprintf(f,"\nTile Type Specials:\n");
    for(i=0;i<tilespecial_count;++i)
    {
        Size[4]=max<size_t>(Size[4],strlen(TileSpecialString[i]));
        fprintf(f,"%4i ; %s\n", i, TileSpecialString[i] ,0 );
    }

    /* - Not needed for now -
    //Direction is tricky
    for(i=0;i<TILE_TYPE_ARRAY_LENGTH;++i)
        Size[5]=max(Size[5], tileTypeTable[i].d.sum()+1 );
    */

    //Print the headings first.
    fprintf(f,"\nTile Types:\n");
    for(i=0;i<Columns;++i)
    {
        if(i) putc(';',f);
        fprintf(f," %-*s ",Size[i],Headings[i],0);
    }
    fprintf(f,"\n");



    //Process the whole array.
    //A macro should be used for making the strings safe, but they are left in naked ? blocks 
    //to illustrate the array references more clearly.
    for(i=0;i<TILE_TYPE_ARRAY_LENGTH;++i)
    {
        fprintf(f," %*i ; %-*s ; %-*s ; %*c ; %-*s ; %-*s ; %s\n",
            Size[0], i,
            Size[1], ( tileTypeTable[i].name ? TileShapeString[ tileTypeTable[i].shape ]    : "" ),
            Size[2], ( tileTypeTable[i].name ? TileMaterialString[ tileTypeTable[i].material ] : "" ),
            Size[3], ( tileTypeTable[i].variant ? '0'+tileTypeTable[i].variant : ' ' ),
            Size[4], ( tileTypeTable[i].special ? TileSpecialString[ tileTypeTable[i].special ]  : "" ),
            Size[5], ( tileTypeTable[i].direction.whole ? tileTypeTable[i].direction.getStr()        : "" ),
            ( tileTypeTable[i].name ? tileTypeTable[i].name : "" ),
            0
            );
    }
    fprintf(f,"\n");


    #ifndef LINUX_BUILD
    if( 1== argc)
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    #endif
    return 0;
}
