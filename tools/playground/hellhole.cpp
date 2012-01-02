// Burn a hole straight to hell!

#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
using namespace std;

#include <DFHack.h>
#include <DFTileTypes.h>
#include <modules/Gui.h>
using namespace DFHack;


#ifdef LINUX_BUILD
#include <unistd.h>
void waitmsec (int delay)
{
    usleep(delay);
}
#else
#include <windows.h>
void waitmsec (int delay)
{
    Sleep(delay);
}
#endif

#define minmax(MinV,V,MaxV) (max((MinV),min((MaxV),(V))))

//User interaction enums.
//Pit Type (these only have meaning within hellhole, btw)
#define PITTYPEMACRO \
    X(pitTypeChasm,"Bottomless Chasm" ) \
    X(pitTypeEerie,"Bottomless Eerie Pit" ) \
    X(pitTypeFloor,"Pit with floor" ) \
    X(pitTypeSolid,"Solid Pillar" ) \
    X(pitTypeOasis,"Oasis Pit (ends at magma, no hell access)" ) \
    X(pitTypeOPool,"Oasis Pool, with partial aquifer (default 5 z-levels)" ) \
    X(pitTypeMagma,"Magma Pit (similar to volcano, no hell access)" ) \
    X(pitTypeMPool,"Magma Pool (default 5 z-levels)" )
//end PITTYPEMACRO

#define X(name,desc) name,
enum e_pitType
{
    pitTypeInvalid=-1,
    PITTYPEMACRO
    pitTypeCount,
};
#undef X


#define X(name,desc) desc,
const char * pitTypeDesc[pitTypeCount+1] =
{
    PITTYPEMACRO
    ""
};
#undef X




int getyesno( const char * msg , int default_value )
{
    const int bufferlen=4;
    static char buf[bufferlen];
    memset(buf,0,bufferlen);
    while (-1)
    {
        if (msg) printf("\n%s (default=%s)\n:" , msg , (default_value?"yes":"no") );
        fflush(stdin);
        fgets(buf,bufferlen,stdin);
        switch (buf[0])
        {
            case 0:
            case 0x0d:
            case 0x0a:
                return default_value;
            case 'y':
            case 'Y':
            case 'T':
            case 't':
            case '1':
                return -1;
            case 'n':
            case 'N':
            case 'F':
            case 'f':
            case '0':
                return 0;
        }
    }
    return 0;
}

int getint( const char * msg , int min, int max, int default_value ) {
    const int bufferlen=16;
    static char buf[bufferlen];
    int n=0;
    memset(buf,0,bufferlen);
    while (-1)
    {
        if (msg) printf("\n%s (default=%d)\n:" , msg , default_value);
        fflush(stdin);
        fgets(buf,bufferlen,stdin);
        if ( !buf[0] || 0x0a==buf[0] || 0x0d==buf[0] )
        {
            return default_value;
        }
        if ( sscanf(buf,"%d", &n) )
        {
            if (n>=min && n<=max )
            {
                return n;
            }
        }
    }
}

int getint( const char * msg , int min, int max )
{
    const int bufferlen=16;
    static char buf[bufferlen];
    int n=0;
    memset(buf,0,bufferlen);
    while (-1)
    {
        if (msg)
        {
            printf("\n%s \n:" , msg );
        }
        fflush(stdin);
        fgets(buf,bufferlen,stdin);

        if ( !buf[0] || 0x0a==buf[0] || 0x0d==buf[0] )
        {
            continue;
        }
        if ( sscanf(buf,"%d", &n) )
        {
            if (n>=min && n<=max )
            {
                return n;
            }
        }
    }
}



//Interactive, get pit type from user
e_pitType selectPitType()
{
    while ( -1 )
    {
        printf("Enter the type of hole to dig:\n" );
        for (int n=0;n<pitTypeCount;++n)
        {
            printf("%2d) %s\n", n, pitTypeDesc[n] );
        }
        printf(":");
        return (e_pitType)getint(NULL, 0, pitTypeCount-1 );
    }
}


void drawcircle(const int radius, unsigned char pattern[16][16], unsigned char v )
{
    //Small circles get better randomness if handled manually
    if ( 1==radius )
    {
        pattern[7][7]=v;
        if ( (rand()&1) ) pattern[6][7]=v;
        if ( (rand()&1) ) pattern[8][7]=v;
        if ( (rand()&1) ) pattern[7][6]=v;
        if ( (rand()&1) ) pattern[7][8]=v;

    }
    else if ( 2==radius )
    {
        pattern[7][7]=v;
        pattern[7][5]=v;
        pattern[7][6]=v;
        pattern[7][8]=v;
        pattern[7][9]=v;
        pattern[6][7]=v;
        pattern[8][7]=v;
        pattern[9][7]=v;
        pattern[6][6]=v;
        pattern[6][8]=v;
        pattern[8][6]=v;
        pattern[8][8]=v;
        pattern[5][7]=v;

        if ( (rand()&1) ) pattern[6][5]=v;
        if ( (rand()&1) ) pattern[5][6]=v;
        if ( (rand()&1) ) pattern[8][5]=v;
        if ( (rand()&1) ) pattern[9][6]=v;
        if ( (rand()&1) ) pattern[6][9]=v;
        if ( (rand()&1) ) pattern[5][8]=v;
        if ( (rand()&1) ) pattern[8][9]=v;
        if ( (rand()&1) ) pattern[9][8]=v;
    }
    else
    {
        //radius 3 or larger, simple circle calculation.
        int x,y;
        for (y=0-radius; y<=radius; ++y)
        {
            for (x=0-radius; x<=radius; ++x)
            {
                if (x*x+y*y <= radius*radius + (rand()&31-8) )
                {
                    pattern[ minmax(0,7+x,15) ][ minmax(0,7+y,15) ]=v;
                }
            }
        }
        //Prevent boxy patterns with a quick modification on edges
        if (rand()&1) pattern[ 7 ][ minmax(0,7+radius+1,15) ] = v;
        if (rand()&1) pattern[ 7 ][ minmax(0,7-radius-1,15) ] = v;
        if (rand()&1) pattern[ minmax(0,7+radius+1,15) ][ 7 ] = v;
        if (rand()&1) pattern[ minmax(0,7-radius-1,15) ][ 7 ] = v;
    }
}

//Check all neighbors for a given value n.
//If found, and v>=0, replace with v.
//Returns number of neighbors found.
int checkneighbors(unsigned char pattern[16][16], int x, int y, unsigned char n , char v )
{
    int r=0;
    if ( x>0  && y>0  && n==pattern[x-1][y-1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if ( x>0          && n==pattern[x-1][y  ] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if (         y>0  && n==pattern[x  ][y-1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if ( x<15         && n==pattern[x+1][y  ] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if ( x<15 && y>0  && n==pattern[x+1][y-1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if ( x<15 && y<15 && n==pattern[x+1][y+1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if (         y<15 && n==pattern[x  ][y+1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    if ( x>0  && y<15 && n==pattern[x-1][y+1] )
    {
        ++r;
        if (v>-1) pattern[x][y]=v;
    }
    return r;
}
//convenience
int checkneighbors(unsigned char pattern[16][16], int x, int y, unsigned char n )
{
    return checkneighbors(pattern,x,y,n,-1);
}

void settileat(unsigned char pattern[16][16], const unsigned char needle, const unsigned char v, const int index )
{
    int ok=0;
    int safety=256*256;
    int y,x,i=0;
    //Scan for sequential index
    while ( !ok && --safety )
    {
        for (y=0 ; !ok && y<16 ; ++y )
        {
            for (x=0 ; !ok && x<16 ; ++x )
            {
                if ( needle==pattern[x][y] )
                {
                    ++i;
                    if ( index==i )
                    {
                        //Got it!
                        pattern[x][y]=v;
                        ok=-1;
                    }
                }
            }
        }
    }
}


//FIXME: good candidate for adding to dfhack. Maybe the Maps should have those cached so they can be queried?
//Is a given feature present at the given tile?
int isfeature(
    vector<DFHack::t_feature> global_features,
    std::map <DFHack::DFCoord, std::vector<DFHack::t_feature *> > local_features,
    const mapblock40d &block, const DFCoord &pc, const int x, const int y, const e_feature Feat
)
{
    //const TileRow * tp;
    //tp = getTileTypeP(block.tiletypes[x][y]);
    const t_designation * d;
    d = &block.designation[x][y];

    if ( block.local_feature > -1 && d->bits.feature_local ) {
        if ( Feat==local_features[pc][block.local_feature]->type ) return Feat;
    }
    if ( block.global_feature > -1 && d->bits.feature_global ) {
        if ( Feat==global_features[block.global_feature].type ) return Feat;
    }

    return 0;
}

// FIXME: use block cache, break into manageable bits
int main (void)
{
    srand ( (unsigned int)time(NULL) );

    //Message of intent
    cout <<
         "DF Hole" << endl <<
         "This tool will instantly dig a chasm, pit, pipe, etc through hell, wherever your cursor is." << endl <<
         "This can not be undone!  End program now if you don't want hellish fun." << endl
         ;

    //User selection of settings should have it own routine, a structure for settings, I know
    //sloppy mess, but this is just a demo utility.

    //Pit Types.
    e_pitType pittype = selectPitType();

    //Here are all the settings.
    //Default values are set here.
    int pitdepth=0;
    int roof=-1;
    int holeradius=6;
    int wallthickness=1;
    int wallpillar=1;
    int holepillar=1;
    int exposehell = 0;
    int fillmagma=0;
    int fillwater=0;
    int stopatmagma=0;
    int exposemagma=0;
    int aquify=0;

    //The Tile Type to use for the walls lining the hole
    //263 is semi-molten rock, 331 is obsidian
    uint32_t whell=263, wmolten=263, wmagma=331, wcave=331;
    //The Tile Type to use for the hole's floor at bottom of the map
    //35 is chasm, 42 is eerie pit , 340 is obsidian floor, 344 is featstone floor, 264 is 'magma flow' floor
    uint32_t floor=35, cap=340;
    int floorvar=0;


    //Modify default settings based on pit type.
    switch ( pittype )
    {
        case pitTypeChasm:
            floor=35;
            break;
        case pitTypeEerie:
            floor=42;
            break;
        case pitTypeFloor:
            floor=344;
            floorvar=3;
            break;
        case pitTypeSolid:
            holeradius=0;
            wallthickness=7;
            wallpillar=4;
            break;
        case pitTypeOasis:
            stopatmagma=-1;
            fillwater=-1;
            holeradius=5;
            wallthickness=2;
            //aquify=-1;
            floor=340;
            floorvar=3;
            break;
        case pitTypeOPool:
            pitdepth=5;
            fillwater=-1;
            holeradius=5;
            wallthickness=2;
            //aquify=-1;
            floor=340;
            floorvar=3;
            break;
        case pitTypeMagma:
            stopatmagma=-1;
            exposemagma=-1;
            wallthickness=2;
            fillmagma=-1;
            floor=264;
            break;
        case pitTypeMPool:
            pitdepth=5;
            wallthickness=2;
            fillmagma=-1;
            floor=340;
            floorvar=3;
            break;
    }


    //Should tiles be revealed?
    int reveal=0;


    int accept = getyesno("Use default settings?",1);

    while ( !accept )
    {
        //Pit Depth
        pitdepth = getint( "Enter max depth (0 for bottom of map)", 0, INT_MAX, pitdepth );

        //Hole Size
        holeradius = getint( "Enter hole radius, 0 to 16", 0, 16, holeradius );

        //Wall thickness
        wallthickness = getint( "Enter wall thickness, 0 to 16", 0, 16, wallthickness );

        //Obsidian Pillars
        holepillar = getint( "Number of Obsidian Pillars in hole, 0 to 255", 0, 255, holepillar );
        wallpillar = getint( "Number of Obsidian Pillars in wall, 0 to 255", 0, 255, wallpillar );

        //Open Hell?
        exposehell=getyesno("Expose the pit to hell (no walls in hell)?",exposehell);

        //Stop when magma sea is hit?
        stopatmagma=getyesno("Stop at magma sea?",stopatmagma);
        exposemagma=getyesno("Expose magma sea (no walls in magma)?",exposemagma);

        //Fill?
        fillmagma=getyesno("Fill with magma?",fillmagma);
        if (fillmagma) aquify=fillwater=0;
        fillwater=getyesno("Fill with water?",fillwater);
        //aquify=getyesno("Aquifer?",aquify);


        ///////////////////////////////////////////////////////////////////////////////////////////////
        //Print settings.
        //If a settings struct existed, this could be in a routine
        printf("Using Settings:\n");
        printf("Pit Type......: %d = %s\n", pittype, pitTypeDesc[pittype]);
        printf("Depth.........: %d\n",  pitdepth);
        printf("Hole Radius...: %d\n",  holeradius);
        printf("Wall Thickness: %d\n",  wallthickness);
        printf("Pillars, Hole.: %d\n",  holepillar);
        printf("Pillars, Wall.: %d\n",  wallpillar);
        printf("Expose Hell...: %c\n", (exposehell?'Y':'N') );
        printf("Stop at Magma.: %c\n", (stopatmagma?'Y':'N') );
        printf("Expose Magma..: %c\n", (exposemagma?'Y':'N') );
        printf("Magma Fill....: %c\n", (fillmagma?'Y':'N') );
        printf("Water Fill....: %c\n", (fillwater?'Y':'N') );
        printf("Aquifer.......: %c\n", (aquify?'Y':'N') );

        accept = getyesno("Accept these settings?",1);
    }


    int64_t n;
    uint32_t x_max,y_max,z_max;


    //Pattern to dig
    unsigned char pattern[16][16];


    for (int regen=1;regen; )
    {
        regen=0;

        memset(pattern,0,sizeof(pattern));

        //Calculate a randomized circle.
        //These values found through experimentation.
        int x=0, y=0, n=0;

        //Two concentric irregular circles
        //Outer circle, solid.
        if ( wallthickness )
        {
            drawcircle(holeradius+wallthickness, pattern, 2);
        }
        //Inner circle, hole.
        if ( holeradius )
        {
            drawcircle(holeradius, pattern, 1);
        }


        //Post-process to be certain the wall totally encloses hole.
        if (wallthickness)
        {
            for (y=0;y<16;++y)
            {
                for (x=0;x<16;++x)
                {
                    if ( 1==pattern[x][y] )
                    {
                        //No hole at edges.
                        if ( x<1 || x>14 || y<1 || y>14 )
                        {
                            pattern[x][y]=2;
                        }
                    }
                    else if ( 0==pattern[x][y] )
                    {
                        //check neighbors
                        checkneighbors( pattern , x,y, 1, 2);
                    }
                }
            }
        }

        //Makes sure that somewhere random gets a vertical pillar of rock which is safe
        //to dig stairs down, to permit access to anywhere within the pit from the top.
        for (n=holepillar; n ; --n)
        {
            settileat( pattern , 1 , 3 , rand()&255 );
        }
        for (n=wallpillar; n ; --n)
        {
            settileat( pattern , 2 , 3 , rand()&255 );
        }

        //Note:
        //At this point, the pattern holds:
        //0 for all tiles which will be ignored.
        //1 for all tiles set to empty pit space.
        //2 for all normal walls.
        //3 for the straight obsidian top-to-bottom wall.
        //4 is randomized between wall or floor (!not implemented!)

        printf("\nPattern:\n");
        const char patternkey[] = ".cW!?567890123";

        //Print the pattern
        for (y=0;y<16;++y)
        {
            for (x=0;x<16;++x)
            {
                cout << patternkey[ pattern[x][y] ];
            }
            cout << endl;
        }
        cout << endl;

        regen = !getyesno("Acceptable Pattern?",1);
    }

    //Post-process settings to fix problems here
    if (pitdepth<1)
    {
        pitdepth=INT_MAX;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////


    cerr << "Loading memory map..." << endl;

    //Connect to DF!
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();



    //Init
    cerr << "Attaching to DF..." << endl;
    try
    {
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    // init the map
    DFHack::Maps *Mapz = DF->getMaps();
    if (!Mapz->Start())
    {
        cerr << "Can't init map.  Exiting." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    Mapz->getSize(x_max,y_max,z_max);


    //Get cursor
    int32_t cursorX, cursorY, cursorZ;
    DFHack::Gui *Gui = DF->getGui();
    Gui->getCursorCoords(cursorX,cursorY,cursorZ);
    if (-30000==cursorX)
    {
        cout << "No cursor position found.  Exiting." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    //Block coordinates
    int32_t bx=cursorX/16, by=cursorY/16, bz=cursorZ;
    //Tile coordinates within block
    int32_t tx=cursorX%16, ty=cursorY%16, tz=cursorZ;

    /*
    //Access the DF interface to pause the game.
    //Copied from the reveal tool.
    DFHack::Gui *Gui =DF->getGui();
    cout << "Pausing..." << endl;
    Gui->SetPauseState(true);
    DF->Resume();
    waitmsec(1000);
    DF->Suspend();
    */

    //Verify that every z-level at this location exists.
    for (int32_t Z = 0; Z<= bz ;Z++)
    {
        if ( ! Mapz->isValidBlock(bx,by,Z) )
        {
            cout << "This block does't exist!  Exiting." << endl;
            #ifndef LINUX_BUILD
                cin.ignore();
            #endif
            return 1;
        }
    }

    //Get all the map features.
    vector<DFHack::t_feature> global_features;
    if (!Mapz->ReadGlobalFeatures(global_features))
    {
        cout << "Couldn't load global features! Probably a version problem." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    std::map <DFHack::DFCoord, std::vector<DFHack::t_feature *> > local_features;
    if (!Mapz->ReadLocalFeatures(local_features))
    {
        cout << "Couldn't load local features! Probably a version problem." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    //Get info on current tile, to determine how to generate the pit
    mapblock40d topblock;
    Mapz->ReadBlock40d( bx, by, bz , &topblock );
    //Related block info
    DFCoord pc(bx,by);
    mapblock40d block;
    const TileRow * tp;
    t_designation * d;

    //////////////////////////////////////
    //From top to bottom, dig this thing.
    //////////////////////////////////////

    //Top level, cap.
    //Might make this an option in the future
    //For now, no wall means no cap.
    if (wallthickness)
    {
        Mapz->ReadBlock40d( bx, by, bz , &block );
        for (uint32_t x=0;x<16;++x)
        {
            for (uint32_t y=0;y<16;++y)
            {
                if ( (pattern[x][y]>1) || (roof && pattern[x][y]) )
                {
                    tp = getTileRow(block.tiletypes[x][y]);
                    d = &block.designation[x][y];
                    //Only modify this level if it's 'empty'
                    if ( EMPTY != tp->shape && RAMP_TOP != tp->shape && STAIR_DOWN != tp->shape && DFHack::BROOK_TOP != tp->shape)
                    {
                        continue;
                    }

                    //Need a floor for empty space.
                    if (reveal)
                    {
                        d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
                    }
                    //Always clear the dig designation.
                    d->bits.dig = designation_no;
                    //unlock fluids, so they fall down the pit.
                    d->bits.flow_forbid = d->bits.liquid_static=0;
                    block.blockflags.bits.liquid_1 = block.blockflags.bits.liquid_2 = 1;
                    //Remove aquifer, to prevent bugginess
                    d->bits.water_table=0;
                    //Set the tile.
                    block.tiletypes[x][y] = cap + rand()%4;
                }
            }
        }
        //Write the block.
        Mapz->WriteBlockFlags(bx,by,bz, block.blockflags );
        Mapz->WriteDesignations(bx,by,bz, &block.designation );
        Mapz->WriteTileTypes(bx,by,bz, &block.tiletypes );
        Mapz->WriteDirtyBit(bx,by,bz,1);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //All levels in between.
    int done=0;
    uint32_t t,v;
    int32_t z = bz-1;
    int32_t bottom = max(0,bz-pitdepth-1);
    assert( bottom>=0 && bottom<=bz );
    for ( ; !done && z>=bottom ; --z)
    {
        int watercount=0;
        int magmacount=0;
        int moltencount=0;
        int rockcount=0;
        int veincount=0;
        int emptycount=0;
        int hellcount=0;
        int templecount=0;
        int adamcount=0;
        int featcount=0;
        int tpat;

        cout << z << endl;
        assert( Mapz->isValidBlock(bx,by,z) );
        if (!Mapz->ReadBlock40d( bx, by, z , &block ))
        {
            cout << "Bad block! " << bx << "," << by << "," << z << endl;
        }

        //Pre-process this z-level, to get some tile statistics.
        for (int32_t x=0;x<16;++x)
        {
            for (int32_t y=0;y<16;++y)
            {
                t=0;
                tp = getTileRow(block.tiletypes[x][y]);
                d = &block.designation[x][y];
                tpat=pattern[x][y];

                //Tile type material categories
                switch ( tp->material )
                {
                    case AIR:
                        ++emptycount;
                        break;
                    case MAGMA:
                        ++moltencount;
                        break;
                    case VEIN:
                        ++veincount;
                        break;
                    case FEATSTONE:
                    case HFS:
                    case OBSIDIAN:
                        //basicly, ignored.
                        break;
                    default:
                        if ( EMPTY == tp->shape || RAMP_TOP == tp->shape || STAIR_DOWN == tp->shape )
                        {
                            ++emptycount;
                        }
                        else
                        {
                            ++rockcount;
                        }
                        break;
                }

                //Magma and water
                if ( d->bits.flow_size )
                {
                    if (d->bits.liquid_type)
                    {
                        ++magmacount;
                    }
                    else
                    {
                        ++watercount;
                    }
                }


                //Check for Features
                if ( block.local_feature > -1 || block.global_feature > -1 )
                {
                    //Count tiles which actually are in the feature.
                    //It is possible for a block to have a feature, but no tiles to be feature.
                    if ( d->bits.feature_global || d->bits.feature_local )
                    {
                        //All features
                        ++featcount;

                        if ( d->bits.feature_global && d->bits.feature_local )
                        {
                            cout << "warn:tile is global and local at same time!" << endl;
                        }

                        n=0;
                        if ( block.global_feature > -1 && d->bits.feature_global )
                        {
                            n=global_features[block.global_feature].type;
                            switch ( n )
                            {
                                case feature_Other:
                                    //no count
                                    break;
                                case feature_Adamantine_Tube:
                                    ++adamcount;
                                    break;
                                case feature_Underworld:
                                    ++hellcount;
                                    break;
                                case feature_Hell_Temple:
                                    ++templecount;
                                    break;
                                default:
                                    //something here. for debugging, it may be interesting to know.
                                    if (n) cout << '(' << n << ')';
                            }
                        }

                        n=0;
                        if ( block.local_feature > -1 && d->bits.feature_local )
                        {
                            n=local_features[pc][block.local_feature]->type;
                            switch ( n )
                            {
                                case feature_Other:
                                    //no count
                                    break;
                                case feature_Adamantine_Tube:
                                    ++adamcount;
                                    break;
                                case feature_Underworld:
                                    ++hellcount;
                                    break;
                                case feature_Hell_Temple:
                                    ++templecount;
                                    break;
                                default:
                                    //something here. for debugging, it may be interesting to know.
                                    if (n) cout << '[' << n << ']';
                            }
                        }
                    }
                }
            }
        }


        //If stopping at magma, and no no non-feature stone in this layer, and magma found, then we're either at
        //or below the magma sea / molten rock.
        if ( stopatmagma && (moltencount || magmacount) && (!exposemagma || !rockcount) )
        {
            //If not exposing magma, quit at the first sign of magma.
            //If exposing magma, quite once magma is exposed.
            done=-1;
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////
        //Some checks, based on settings and stats collected
        //First check, are we at illegal depth?
        if ( !done && hellcount && stopatmagma )
        {
            //Panic!
            done=-1;
            tpat=0;
            cout << "error: illegal breach of hell!" << endl;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////
        //Actually process the current z-level.
        //These loops do the work.
        for (int32_t x=0;!done && x<16;++x)
        {
            for (int32_t y=0;!done && y<16;++y)
            {
                t=0;
                tp = getTileRow(block.tiletypes[x][y]);
                d = &block.designation[x][y];
                tpat=pattern[x][y];

                //Up front, remove aquifer, to prevent bugginess
                //It may be added back if aquify is set.
                d->bits.water_table=0;

                //Change behaviour based on settings and stats from this z-level

                //In hell?
                if ( tpat && tpat!=3 && isfeature(global_features, local_features,block,pc,x,y,feature_Underworld ) )
                {
                    if ( exposehell )
                    {
                        tpat=0;
                    }
                }

                //Expose magma?
                if ( tpat && tpat!=3 && exposemagma )
                {
                    //Leave certain tiles unchanged.
                    switch ( tp->material )
                    {
                        case HFS:
                        case FEATSTONE:
                        case MAGMA:
                            tpat=0;
                        default:
                            break;
                    }
                    //Adamantine may be left unchanged...
                    if ( isfeature(global_features, local_features,block,pc,x,y,feature_Adamantine_Tube ) )
                    {
                        tpat=0;
                    }
                    //Leave magma sea unchanged.
                    if ( d->bits.flow_size && d->bits.liquid_type)
                    {
                        tpat=0;
                    }
                }


                //For all situations...
                //Special modification for walls, always for adamantine.
                if ( isfeature(global_features, local_features,block,pc,x,y,feature_Adamantine_Tube ) )
                {
                    if ( 2==pattern[x][y] || 3==pattern[x][y] )
                    {
                        tpat=2;
                    }
                }


                //Border or space?
                switch (tpat)
                {
                    case 0:
                        continue;
                        break;
                    case 1:
                        //Empty Space
                        t=32;
                        //d->bits.light = topblock.designation[x][y].bits.light;
                        //d->bits.skyview = topblock.designation[x][y].bits.skyview;
                        //d->bits.subterranean = topblock.designation[x][y].bits.subterranean;

                        //Erase special markers?
                        //d->bits.feature_global = d->bits.feature_local = 0;

                        //Water? Magma?
                        if (fillmagma || fillwater)
                        {
                            d->bits.flow_size=7;
                            d->bits.water_stagnant = false;
                            d->bits.water_salt = false;
                            if (fillmagma)
                            {
                                d->bits.liquid_type=liquid_magma;
                            }
                            else
                            {
                                d->bits.liquid_type=liquid_water;
                            }
                        }
                        else
                        {
                            //Otherwise, remove all liquids.
                            d->bits.flow_size=0;
                            d->bits.water_stagnant = false;
                            d->bits.water_salt = false;
                            d->bits.liquid_type = liquid_water;
                        }

                        break;
                    case 2:
                        //Wall.
                        //First guess based on current material
                        switch ( tp->material )
                        {
                            case OBSIDIAN:
                                t=wmagma;
                                break;
                            case MAGMA:
                                t=wmolten;
                                break;
                            case HFS:
                                //t=whell;
                                break;
                            case VEIN:
                                t=440; //Solid vein block
                                break;
                            case FEATSTONE:
                                t=335; //Solid feature stone block
                                break;
                            default:
                                t=wcave;
                        }
                        //Adamantine (a local feature) trumps veins.
                        {
                            //Local Feature?
                            if ( block.local_feature > -1  )
                            {
                                switch ( n=local_features[pc][block.local_feature]->type )
                                {
                                    case feature_Underworld:
                                    case feature_Hell_Temple:
                                        //Only adopt these if there is no global feature present
                                        if ( block.global_feature >-1 )
                                        {
                                            break;
                                        }
                                    case feature_Adamantine_Tube:
                                        //Always for adamantine, sometimes for others
                                        //Whatever the feature is made of. "featstone wall"
                                        d->bits.feature_global = 0;
                                        d->bits.feature_local = 1;
                                        t=335;
                                        break;
                                }
                            }
                            //Global Feature?
                            else if (block.global_feature > -1 && !d->bits.feature_local )
                            {
                                switch ( n=global_features[block.global_feature].type )
                                {
                                    case feature_Adamantine_Tube:
                                    case feature_Underworld:
                                    case feature_Hell_Temple:
                                        //Whatever the feature is made of. "featstone wall"
                                        d->bits.feature_global = 1;
                                        t=335;
                                        break;
                                }
                            }
                        }

                        //Erase any liquids, as they cause problems.
                        d->bits.flow_size=0;
                        d->bits.water_stagnant = false;
                        d->bits.water_salt = false;
                        d->bits.liquid_type=liquid_water;

                        //Placing an aquifer?
                        //(bugged, these aquifers don't generate water!)
                        if ( aquify )
                        {
                            //Only normal stone types can be aquified
                            if ( tp->material!=MAGMA && tp->material!=FEATSTONE && tp->material!=HFS  )
                            {
                                //Only place next to the hole.
                                //If no hole, place in middle.
                                if ( checkneighbors(pattern,x,y,1) || (7==x && 7==y) )
                                {
                                    d->bits.water_table = 1;
                                    //t=265; //soil wall
                                }
                            }
                        }
                        break;
                    case 3:
                        ////No obsidian walls on bottom of map!
                        //if(z<1 && (d->bits.feature_global || d->bits.feature_local) ) {
                        //  t=335;
                        //}

                        //Special wall, always sets to obsidian, to give a stairway
                        t=331;

                        //Erase special markers
                        d->bits.feature_global = d->bits.feature_local = 0;

                        //Erase any liquids, as they cause problems.
                        d->bits.flow_size=0;
                        d->bits.water_stagnant = false;
                        d->bits.water_salt = false;
                        d->bits.liquid_type=liquid_water;
                        break;
                    default:
                        cout << ".error,bad pattern.";
                }

                //For all tiles.
                if (reveal)
                {
                    d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
                }
                //Always clear the dig designation.
                d->bits.dig=designation_no;
                //Make it underground, because it is capped
                d->bits.subterranean=1;
                d->bits.light=0;
                d->bits.skyview=0;
                //unlock fluids, so they fall down the pit.
                d->bits.flow_forbid = d->bits.liquid_static=0;
                block.blockflags.bits.liquid_1 = block.blockflags.bits.liquid_2 = 1;
                //Set the tile.
                block.tiletypes[x][y] = t;

            }
        }

        //Write the block.
        Mapz->WriteBlockFlags(bx,by,z, block.blockflags );
        Mapz->WriteDesignations(bx,by,z, &block.designation );
        Mapz->WriteTileTypes(bx,by,z, &block.tiletypes );
        Mapz->WriteDirtyBit(bx,by,z,1);

    }

    //Re-process the last z-level handled above.
    z++;
    assert( z>=0 );


    ///////////////////////////////////////////////////////////////////////////////////////////////
    //The bottom level is special.
    if (-1)
    {
        if (!Mapz->ReadBlock40d( bx, by, z , &block ))
        {
            cout << "Bad block! " << bx << "," << by << "," << z << endl;
        }
        for (uint32_t x=0;x<16;++x)
        {
            for (uint32_t y=0;y<16;++y)
            {
                t=floor;
                v=floorvar;
                tp = getTileRow(block.tiletypes[x][y]);
                d = &block.designation[x][y];

                if ( exposehell )
                {
                    //Leave hell tiles unchanged when exposing hell.
                    if ( isfeature(global_features,local_features,block,pc,x,y,feature_Underworld) )
                    {
                        continue;
                    }
                }

                //Does expose magma need anything at this level?
                if ( exposemagma && stopatmagma )
                {
                    continue;
                }

                switch (pattern[x][y])
                {
                    case 0:
                        continue;
                        break;
                    case 1:
                        //Empty becomes floor.

                        //Base floor type on the z-level first, features, then tile type.
                        if (!z) {
                            //Bottom of map, use the floor specified, always.
                            break;
                        }

                        ////Only place floor where ground is already solid when exposing
                        //if( EMPTY == tp->c || RAMP_TOP == tp->c || STAIR_DOWN == tp->c ){
                        //  continue;
                        //}

                        if ( d->bits.feature_global || d->bits.feature_global ) {
                            //Feature Floor!
                            t=344;
                            break;
                        }

                        //Tile material check.
                        switch ( tp->material )
                        {
                            case OBSIDIAN:
                                t=340;
                                v=3;
                                break;
                            case MAGMA:
                                v=0;
                                t=264; //magma flow
                                break;
                            case HFS:
                                //should only happen at bottom of map
                                break;
                            case VEIN:
                                t=441;  //vein floor
                                v=3;
                                break;
                            case FEATSTONE:
                                t=344;
                                v=3;
                                break;
                        }

                        break;
                    case 2:
                    case 3:
                        //Walls already drawn.
                        //Ignore.
                        continue;
                        break;
                }

                //For all tiles.
                if (reveal) d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
                //Always clear the dig designation.
                d->bits.dig=designation_no;
                //unlock fluids
                d->bits.flow_forbid = d->bits.liquid_static=0;
                block.blockflags.bits.liquid_1 = block.blockflags.bits.liquid_2 = 1;

                //Set the tile.
                block.tiletypes[x][y] = t + ( v ? rand()&v : 0 );
            }
        }
        //Write the block.
        Mapz->WriteBlockFlags(bx,by,z, block.blockflags );
        Mapz->WriteDesignations(bx,by,z, &block.designation );
        Mapz->WriteTileTypes(bx,by,z, &block.tiletypes );
        Mapz->WriteDirtyBit(bx,by,z,1);
    }

    DF->Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
