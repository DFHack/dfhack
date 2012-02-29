#include <iostream>
#include <string.h> // for memset
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <stdio.h>
#include <cstdlib>
using namespace std;

#include <DFHack.h>
#include <extra/MapExtras.h>
using namespace MapExtras;
//#include <argstream.h>

void usage(int argc, const char * argv[])
{
    cout
	<< "Usage:" << endl
	<< argv[0] << " [option 1] [option 2] [...]" << endl
	<< "-q            : Suppress \"Press any key to continue\" at program termination" << endl
	<< "-u <n>        : Dig upwards <n> times (default 5)" << endl
	<< "-d <n>        : Dig downwards <n> times (default 5)" << endl
	;
}

void digat(MapCache * MCache, DFHack::DFCoord xy)
{
    int16_t tt;
    tt = MCache->tiletypeAt(xy);
    if(!DFHack::isWallTerrain(tt))
	return;

    // found a good tile, dig+unset material
    DFHack::t_designation des = MCache->designationAt(xy);

    if(MCache->testCoord(xy))
    {
	MCache->clearMaterialAt(xy);

	if(des.bits.dig == DFHack::designation_no)
	    des.bits.dig = DFHack::designation_default;
	MCache->setDesignationAt(xy,des);
    }
}

int strtoint(const string &str)
{
    stringstream ss(str);
    int result;
    return ss >> result ? result : -1;
}

typedef struct 
{
    int16_t x;
    int16_t y;
} pos;

int main (int argc, const char* argv[])
{
    // Command line options
    bool updown = false;
    bool quiet = true;
    // let's be more useful when double-clicked on windows
    #ifndef LINUX_BUILD
    quiet = false;
    #endif
    int dig_up_n = 5;
    int dig_down_n = 5;

    for(int i = 1; i < argc; i++)
    {
        string arg_cur = argv[i];
        string arg_next = "";
	int arg_next_int = -99999;
	/* Check if argv[i+1] is a number >= 0 */
	if (i < argc-1) {
	    arg_next = argv[i+1];
	    arg_next_int = strtoint(arg_next);
	    if (arg_next != "0" && arg_next_int == 0) {
		arg_next_int = -99999;
	    }
	}
	if (arg_cur == "-x")
	{
	    updown = true;
	}
	else if (arg_cur == "-q")
	{
	    quiet = true;
	}
	else if(arg_cur == "-u" && i < argc-1)
	{
	    if (arg_next_int < 0 || arg_next_int >= 99999) {
		usage(argc, argv);
		return 1;
	    }
	    dig_up_n = arg_next_int;
	    i++;
	}
	else if(arg_cur == "-d" && i < argc-1)
	{
	    if (arg_next_int < 0 || arg_next_int >= 99999) {
		usage(argc, argv);
		return 1;
	    }
	    dig_down_n = arg_next_int;
	    i++;
	}
	else
	{
	    usage(argc, argv);
	    return 1;
	}
    }

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << "Error getting context: " << e.what() << endl;
        if (!quiet)
            cin.ignore();

        return 1;
    }

    uint32_t x_max,y_max,z_max;
    DFHack::Maps * Maps = DF->getMaps();
    DFHack::Gui * Gui = DF->getGui();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map. Make sure you have a map loaded in DF." << endl;
        DF->Detach();
        if (!quiet)
            cin.ignore();

        return 1;
    }

    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

    Gui->getCursorCoords(cx,cy,cz);
    if (cx == -30000)
    {
        cerr << "Cursor is not active. Point the cursor at the position to dig at." << endl;
        DF->Detach();
        if (!quiet)
	{
            cin.ignore();
	}
	return 1;
    }

    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        cerr << "I won't dig the borders. That would be cheating!" << endl;
        DF->Detach();
        if (!quiet)
	{
            cin.ignore();
	}
        return 1;
    }
    MapCache * MCache = new MapCache(Maps);

    DFHack::t_designation des = MCache->designationAt(xy);
    int16_t tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);

    /*
    if( veinmat == -1 )
    {
        cerr << "This tile is non-vein. Bye :)" << endl;
        delete MCache;
        DF->Detach();
        if (!quiet) {
            cin.ignore();
	}
        return 1;
    }
    */
    printf("Digging at (%d/%d/%d), tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);

    // 1 < xy.x < tx_max - 1
    // 1 < xy.y < ty_max - 1
    // xy.z

    // X____
    // X_XXX
    // XXXXX
    // __XXX
    // __XXX
    // _____
    pos map[] = 
    {
	  { 0,0 }
	, { 0,1 }
	, { 0,2 }         , { 2,2 }, { 3,2 }, { 4,2 }
	, { 0,3 }, { 1,3 }, { 2,3 }, { 3,3 }, { 4,3 }
	                  , { 2,4 }, { 3,4 }, { 4,4 }
    // this is mirrored, goes left instead of right
	                  , {-2,2 }, {-3,2 }, {-4,2 }
	         , {-1,3 }, {-2,3 }, {-3,3 }, {-4,3 }
	                  , {-2,4 }, {-3,4 }, {-4,4 }
    };

    DFHack::DFCoord npos = xy;

    if (dig_up_n > 0)
    {
	for (int j = 0; j < dig_up_n; j++)
	{
	    for (int i = 0; i < sizeof(map)/sizeof(map[0]); i++) 
	    {
		npos=xy;
		npos.x += map[i].x;
		npos.y -= 4*j + map[i].y;
		printf("Digging at (%d/%d/%d)\n", npos.x, npos.y, npos.z);
		digat(MCache, npos);
	    }
	}
    }
    if (dig_down_n > 0)
    {
	for (int j = 0; j < dig_down_n; j++)
	{
	    for (int i = 0; i < sizeof(map)/sizeof(map[0]); i++) 
	    {
		npos=xy;
		npos.x += map[i].x;
		npos.y += 4*j + map[i].y;
		printf("Digging at (%d/%d/%d)\n", npos.x, npos.y, npos.z);
		digat(MCache, npos);
	    }
	}
    }

    MCache->WriteAll();
    delete MCache;
    DF->Detach();
    if (!quiet) {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    return 0;
}
