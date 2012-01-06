//

#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
#include <limits>
using namespace std;

#include <conio.h>


#include <DFHack.h>
#include <DFTileTypes.h>

//Avoid including Windows.h because it causes name clashes
extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long milliseconds);

//Trim
#define WHITESPACE " \t\r\n"
inline string trimr(const string & s, const string & t = WHITESPACE)
{
    string d (s);
    string::size_type i (d.find_last_not_of (t));
    if (i == string::npos)
        return "";
    else
        return d.erase (d.find_last_not_of (t) + 1) ;
}  
inline string triml(const string & s, const string & t = WHITESPACE)
{
    string d (s);
    return d.erase (0, s.find_first_not_of (t)) ;
}  
inline string trim(const string & s, const string & t = WHITESPACE)
{
    string d (s);
    return triml(trimr(d, t), t);
}  

void printtiletype( int i ){
	printf("%s\n%4i ; %-13s ; %-11s ; %c ; %-12s ; %s\n",
		( DFHack::tileTypeTable[i].name ? DFHack::tileTypeTable[i].name : "[invalid tile]" ),
		i,
		( DFHack::tileTypeTable[i].name ? DFHack::TileShapeString[ DFHack::tileTypeTable[i].shape ]    : "" ),
		( DFHack::tileTypeTable[i].name ? DFHack::TileMaterialString[ DFHack::tileTypeTable[i].material ] : "" ),
		( DFHack::tileTypeTable[i].variant ? '0'+ DFHack::tileTypeTable[i].variant : ' ' ),
		( DFHack::tileTypeTable[i].special ? DFHack::TileSpecialString[ DFHack::tileTypeTable[i].special ]  : "" ),
		( DFHack::tileTypeTable[i].direction.whole ? DFHack::tileTypeTable[i].direction.getStr()        : "" ),
		0
		);
}


int main (void)
{
    int32_t x,y,z,tx,ty;
    //DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;
    //DFHack::t_temperatures temp1,temp2;
    uint32_t x_max,y_max,z_max;
    int32_t oldT, newT;
	int count, dirty;

	//Brush defaults
	DFHack::TileShape BrushClass = DFHack::WALL;
	DFHack::TileMaterial BrushMat = DFHack::tilematerial_invalid;
	int BrushType = -1;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    DFHack::Maps * Maps;
    DFHack::Gui * Gui;
    try
    {
        DF=DFMgr.getSingleContext();
        DF->Attach();
        Maps = DF->getMaps();
        Maps->Start();
        Maps->getSize(x_max,y_max,z_max);
        Gui = DF->getGui();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    bool end = false;
    cout << "Welcome to the Tile Drawing tool.\nType 'help' or ? for a list of available commands, 'q' to quit" << endl;
    string mode = "wall";
    string command = "";

	while(!end)
	{
        DF->Resume();

		cout << endl << ":";
		getline(cin, command);
		int ch = command[0];
		if(command.length()<=0) ch=0;
		if( ((int)command.find("help")) >=0 ) ch='?';  //under windows, find was casting unsigned!
		switch(ch)
        {
		case '?':
            cout << "Modes:" << endl
				 << "O             - draw Open Space" << endl
                 << "M             - draw material only (shape unchanged)" << endl
                 << "m number      - use Material value entered" << endl
                 << "r             - use Rock/stone material" << endl
                 << "l             - use Soil material" << endl
                 << "v             - use Vein material" << endl
                 << "H             - draw tile shape only (material unchanged)" << endl
				 << "h number      - draw Tile Shape value entered" << endl
                 << "w             - draw Wall tiles" << endl
                 << "f             - draw Floor tiles" << endl
				 << "t number      - draw exact tile type entered" << endl
                 << "Commands:" << endl
                 << "p             - print tile shapes and materials, and current brush" << endl
                 << "P             - print all tile types" << endl
                 << "q             - quit" << endl
                 << "help OR ?     - print this list of commands" << endl
                 << "d             - being drawing" << endl
                 << endl
                 << "Usage:\nChoose a mode (default is walls), then enter 'd' to being drawing.\nMove the cursor in DF wherever you want to draw.\nPress any key to pause drawing." << endl;
			break;
		case 'p':
			//Classes
			printf("\nTile Type Classes:\n");
			for(int i=0;i<DFHack::tileshape_count;++i)
			{
				printf("%4i ; %s\n", i, DFHack::TileShapeString[i] ,0 );
			}
			//Materials
			printf("\nTile Type Materials:\n");
			for(int i=0;i<DFHack::tilematerial_count;++i)
			{
				printf("%4i ; %s\n", i, DFHack::TileMaterialString[i] ,0 );
			}
			//fall through...
		case 10:
		case 13:
		case 0:
			//Print current cursor & brush settings.
			cout << "\nCurrent Brush:\n";
			cout << "tile = ";
			if(BrushClass<0) cout<<"(not drawing)"; else cout<<DFHack::TileShapeString[BrushClass]; cout << endl;
			cout << "mat  = ";
			if(BrushMat<0) cout<<"(not drawing)"; else cout<<DFHack::TileMaterialString[BrushMat]; cout << endl;
			cout << "type = ";
			if(BrushType<0){
				cout<<"(not drawing)";
			}else{
				printtiletype(BrushType);
			}
			break;
		case 'P':
			cout << "\nAll Valid Tile Types:\n";
			for(int i=0;i<TILE_TYPE_ARRAY_LENGTH;++i)
			{
				if( DFHack::tileTypeTable[i].name )
					printtiletype(i);
			}
		case 'w':
			BrushType=-1;
            BrushClass = DFHack::WALL;
            cout << "Tile brush shape set to Wall." << endl;
			break;
		case 'f':
			BrushType=-1;
            BrushClass = DFHack::FLOOR;
            cout << "Tile brush shape set to Floor." << endl;
			break;
		case 'h':
			BrushType=-1;
			BrushClass = (DFHack::TileShape)atol( command.c_str()+1 );
            cout << "Tile brush shape set to " << BrushClass << endl;
			break;
		case 'M':
            BrushClass = DFHack::tileshape_invalid;
            cout << "Tile brush will not draw tile shape." << endl;
			break;
		case 'r':
			BrushType=-1;
            BrushMat = DFHack::STONE;
            cout << "Tile brush material set to Rock." << endl;
			break;
		case 'l':
			BrushType=-1;
            BrushMat = DFHack::SOIL;
            cout << "Tile brush material set to Soil." << endl;
			break;
		case 'v':
			BrushType=-1;
            BrushMat = DFHack::VEIN;
            cout << "Tile brush material set to Vein." << endl;
			break;
		case 'm':
			BrushType=-1;
			BrushMat = (DFHack::TileMaterial)atol( command.c_str()+1 );
            cout << "Tile brush material set to " << BrushMat << endl;
			break;
		case 'H':
            BrushMat = DFHack::tilematerial_invalid;
            cout << "Tile brush will not draw material." << endl;
			break;
		case 'O':
			BrushType=-1;
			BrushClass = DFHack::EMPTY;
			BrushMat = DFHack::AIR;
            cout << "Tile brush will draw Open Space." << endl;
			break;
		case 't':
			BrushClass = DFHack::tileshape_invalid ;
			BrushMat = DFHack::tilematerial_invalid;
			BrushType = atol( command.c_str()+1 );
			cout << "Tile brush type set to:" << endl;
			printtiletype(BrushType);
			break;
		case 'q':
            end = true;
            cout << "Bye!" << endl;
			break;
		case 'd':
        {
			count=0;
			cout << "Beginning to draw at cursor." << endl << "Press any key to stop drawing." << endl;
            //DF->Suspend();
			kbhit(); //throw away, just to be sure.
            for(;;)
            {
				if(!Maps->Start())
                {
                    cout << "Can't see any DF map loaded." << endl;
                    break;
                }
                if(!Gui->getCursorCoords(x,y,z))
                {
                    cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                    break;
                }
                //cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
				tx=x%16; ty=y%16;

                if(!Maps->isValidBlock(x/16,y/16,z))
                {
                    cout << "Invalid block." << endl;
                    break;
                }

				//Read the tiles.
				dirty=0;
				Maps->ReadTileTypes((x/16),(y/16),z, &tiles);
                oldT = tiles[tx][ty];

				newT = -1;
				if( 0<BrushType ){
					//Explicit tile type set.  Trust the user.
					newT = BrushType;
				}else if( 0==BrushMat && 0==BrushClass ){
					//Special case, Empty Air.
					newT = 32;
				}else if( BrushMat>=0 && BrushClass>=0 && ( BrushClass != DFHack::tileTypeTable[oldT].shape || BrushMat != DFHack::tileTypeTable[oldT].material) ){
					//Set tile material and class
					newT = DFHack::findTileType(BrushClass,BrushMat, DFHack::tileTypeTable[oldT].variant , DFHack::tileTypeTable[oldT].special , DFHack::tileTypeTable[oldT].direction );
					if(newT<0) newT = DFHack::findTileType(BrushClass,BrushMat, DFHack::tilevariant_invalid, DFHack::tileTypeTable[oldT].special , DFHack::tileTypeTable[oldT].direction );
					if(newT<0) newT = DFHack::findTileType(BrushClass,BrushMat, DFHack::tilevariant_invalid , DFHack::tileTypeTable[oldT].special , (uint32_t)0 );
				}else if( BrushMat<0 && BrushClass>=0 && BrushClass != DFHack::tileTypeTable[oldT].shape ){
					//Set current tile class only, as accurately as can be expected
                    newT = DFHack::findSimilarTileType(oldT,BrushClass);
				}else if( BrushClass<0 && BrushMat>=0 && BrushMat != DFHack::tileTypeTable[oldT].material ){
					//Set current tile material only
					newT = DFHack::findTileType(DFHack::tileTypeTable[oldT].shape,BrushMat, DFHack::tileTypeTable[oldT].variant , DFHack::tileTypeTable[oldT].special , DFHack::tileTypeTable[oldT].direction );
					if(newT<0) newT = DFHack::findTileType(DFHack::tileTypeTable[oldT].shape,BrushMat, DFHack::tilevariant_invalid , DFHack::tileTypeTable[oldT].special , DFHack::tileTypeTable[oldT].direction );
					if(newT<0) newT = DFHack::findTileType(DFHack::tileTypeTable[oldT].shape,BrushMat, DFHack::tilevariant_invalid , DFHack::tileTypeTable[oldT].special , (uint32_t)0 );
				}
                //If no change, skip it (couldn't find a good tile type, or already what we want)
				if ( newT > 0 && oldT != newT ){
                    //Set new tile type
                    tiles[tx][ty] = newT;
                    dirty=-1;
				}
                //If anything was changed, write it all.
                if (dirty)
                {
                    //Maps->WriteDesignations(x/16,y/16,z/16, &designations);
                    Maps->WriteTileTypes(x/16,y/16,z, &tiles);
					printf("(%4d,%4d,%4d)",x,y,z);
                    ++count;
                }

                Maps->Finish();

				Sleep(10);
				if( kbhit() ) break;				
            }
			cin.clear();
			cout << endl << count << " tiles were drawn." << endl << "Drawing halted.  Entering command mode." << endl;
		}
			continue;
			break;
		default:
			cout << "Unknown command: " << command << endl;
		}

    }
    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
