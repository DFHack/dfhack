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
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>
#include <dfhack/modules/Gui.h>
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
	X(pitTypeMagma,"Magma Pit (similar to volcanoe, no hell access)" ) 
//end PITTYPEMACRO

#define X(name,desc) name,
enum e_pitType {
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




int getyesno( const char * msg , int default_value ){
	const int bufferlen=4;
	static char buf[bufferlen];
	memset(buf,0,bufferlen);
	while(-1){
		if(msg) printf("\n%s (default=%s)\n:" , msg , (default_value?"yes":"no") );
		fflush(stdin);
		fgets(buf,bufferlen,stdin);
		switch(buf[0]){
			case 0: case 0x0d: case 0x0a:
				return default_value;
			case 'y': case 'Y': case 'T': case 't': case '1':
				return -1;
			case 'n': case 'N': case 'F': case 'f': case '0':
				return 0;
		}
	}
	return 0;
}

int getint( const char * msg , int min, int max, int default_value ){
	const int bufferlen=16;
	static char buf[bufferlen];
	int n=0;	
	memset(buf,0,bufferlen);
	while(-1){
		if(msg) printf("\n%s (default=%d)\n:" , msg , default_value);
		fflush(stdin);		
		fgets(buf,bufferlen,stdin);
		if( !buf[0] || 0x0a==buf[0] || 0x0d==buf[0] ) return default_value;
		if( sscanf(buf,"%d", &n) ){
			if(n>=min && n<=max ) return n;
		}
	}
}



//Interactive, get pit type from user
#define X( e , desc ) printf("%2d) %s\n", (e), (desc) );
e_pitType selectPitType(e_pitType default_value ){
	e_pitType r=pitTypeInvalid;
	int c=-1;
	while( -1 ){
		printf("Enter the type of hole to dig (if no entry, default everything):\n" );
		PITTYPEMACRO
		printf(":");
		return (e_pitType)getint(NULL, 0, pitTypeCount-1, default_value );
	}
}
#undef X


void drawcircle(const int radius, unsigned char pattern[16][16], unsigned char v ){
	//Small circles get better randomness if handled manually
	if( 1==radius ){
		pattern[7][7]=v;
		if( (rand()&1) ) pattern[6][7]=v;
		if( (rand()&1) ) pattern[8][7]=v;
		if( (rand()&1) ) pattern[7][6]=v;
		if( (rand()&1) ) pattern[7][8]=v;
	
	}else if( 2==radius ){
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

		if( (rand()&1) ) pattern[6][5]=v;
		if( (rand()&1) ) pattern[5][6]=v;
		if( (rand()&1) ) pattern[8][5]=v;
		if( (rand()&1) ) pattern[9][6]=v;
		if( (rand()&1) ) pattern[6][9]=v;
		if( (rand()&1) ) pattern[5][8]=v;
		if( (rand()&1) ) pattern[8][9]=v;
		if( (rand()&1) ) pattern[9][8]=v;
	}else{
		//radius 3 or larger, simple circle calculation.
		int x,y;
		for(y=0-radius; y<=radius; ++y){
			for(x=0-radius; x<=radius; ++x){
				if(x*x+y*y <= radius*radius + (rand()&31-8) ){
					pattern[ minmax(0,7+x,15) ][ minmax(0,7+y,15) ]=v;
				}
			}
		}
		//Prevent boxy patterns with a quick modification on edges
		if(rand()&1) pattern[ 7 ][ minmax(0,7+radius+1,15) ] = v;
		if(rand()&1) pattern[ 7 ][ minmax(0,7-radius-1,15) ] = v;
		if(rand()&1) pattern[ minmax(0,7+radius+1,15) ][ 7 ] = v;
		if(rand()&1) pattern[ minmax(0,7-radius-1,15) ][ 7 ] = v;
	}
}

void settileat(unsigned char pattern[16][16], const unsigned char needle, const unsigned char v, const int index )
{
	int ok=0;
	int safety=256*256;
	int y,x,i=0;
	//Scan for sequential index
	while( !ok && --safety ){
		for(y=0 ; !ok && y<16 ; ++y ){
			for(x=0 ; !ok && x<16 ; ++x ){
				if( needle==pattern[x][y] ){
					++i;
					if( index==i ){
						//Got it!
						pattern[x][y]=v;
						ok=-1;
					}
				}
			}
		}
	}
}


int main (void)
{
	srand ( (unsigned int)time(NULL) );

	//Message of intent
	cout << 
		"DF Hell Hole" << endl << 
		"This program will instantly dig a bottomless hole through hell, wherever your cursor is." << endl <<
		"This can not be undone!  End program now if you don't want hellish fun." << endl
		;
	
	//User selection of settings should have it own routine, a structure for settings, I know
	//sloppy mess, but this is just a demo utility.

	//Pit Types.
	e_pitType pittype = selectPitType(pitTypeInvalid);


	//Hole Diameter
	int holeradius=6;
	if( pitTypeInvalid != pittype && pitTypeMagma != pittype ){
		holeradius = getint( "Enter hole radius, 0 to 8", 0, 8, holeradius );
	}

	//Wall thickness
	int wallthickness=1;
	if( pitTypeInvalid != pittype && pitTypeMagma != pittype ){
		wallthickness = getint( "Enter wall thickness, 0 to 8", 0, 8, wallthickness );
	}

	//Obsidian Pillars
	int pillarwall=1;
	if( pitTypeInvalid != pittype  ){
		pillarwall = getint( "Number of Obsidian Pillars in wall, 0 to 255", 0, 255, pillarwall );
	}
	int pillarchasm=1;
	if( pitTypeInvalid != pittype  ){
		pillarchasm = getint( "Number of Obsidian Pillars in hole, 0 to 255", 0, 255, pillarchasm );
	}

	//Open Hell?
	int exposehell = 0;
	if( pitTypeInvalid != pittype && pitTypeMagma != pittype && wallthickness ){
		exposehell=getyesno("Expose the pit to hell (no walls in hell)?",0);
	}

	//Fill?
	int fillmagma=0;
	if( pitTypeInvalid != pittype ){
		fillmagma=getyesno("Fill with magma?",0);
	}
	int fillwater=0;
	if( pitTypeInvalid != pittype && !fillmagma){
		fillwater=getyesno("Fill with water?",0);
	}


	//If skipped all settings, fix the pit type
	if( pitTypeInvalid == pittype ) pittype = pitTypeChasm;


	///////////////////////////////////////////////////////////////////////////////////////////////
	//Print settings.
	//If a settings struct existed, this could be in a routine
	printf("Using Settings:\n");
	printf("Pit Type......: %d = %s\n", pittype, pitTypeDesc[pittype]);
	printf("Hole Radius...: %d\n",  holeradius);
	printf("Wall Thickness: %d\n",  wallthickness);
	printf("Pillars, Wall.: %d\n",  pillarwall);
	printf("Pillars, Hole.: %d\n",  pillarchasm);
	printf("Expose Hell...: %c\n", (exposehell?'Y':'N') );
	printf("Magma Fill....: %c\n", (fillmagma?'Y':'N') );
	printf("Water Fill....: %c\n", (fillwater?'Y':'N') );



	int64_t n;
    uint32_t x_max,y_max,z_max;

	//The Tile Type to use for the walls lining the hole
	//263 is semi-molten rock, 331 is obsidian
	uint32_t whell=263, wmolten=263, wmagma=331, wcave=331;
	//The Tile Type to use for the hole's floor at bottom of the map
	//35 is chasm, 42 is eerie pit , 340 is obsidian floor
	uint32_t floor=35, cap=340;
	if( pitTypeEerie == pittype ) floor=42;

	//Should tiles be revealed?
	int reveal=1;


	//Pattern to dig
	unsigned char pattern[16][16];


	for(int regen=1;regen; ){
		regen=0;

		memset(pattern,0,sizeof(pattern));

		//Calculate a randomized circle.
		//These values found through experimentation.
		int x=0, y=0, n=0;

		//Two concentric irregular circles
		//Outer circle, solid.
		if( wallthickness ){
			drawcircle(holeradius+wallthickness, pattern, 2);
		}
		//Inner circle, hole.
		if( holeradius ){
			drawcircle(holeradius, pattern, 1);
		}


		//Post-process to be certain the wall totally encloses hole.
		if(wallthickness){
			for(y=0;y<16;++y){
				for(x=0;x<16;++x){
					if( 1==pattern[x][y] ){
						//No hole at edges.
						if( x<1 || x>14 || y<1 || y>14 ){
							pattern[x][y]=2;
						}
					}else if( 0==pattern[x][y] ){
						//check neighbors
						if( x>0 && y>0 && 1==pattern[x-1][y-1] ){ pattern[x][y]=2; continue; }
						if( x>0        && 1==pattern[x-1][y  ] ){ pattern[x][y]=2; continue; }
						if(        y>0 && 1==pattern[x  ][y-1] ){ pattern[x][y]=2; continue; }
						if( x<15       && 1==pattern[x+1][y  ] ){ pattern[x][y]=2; continue; }
						if( x<15&& y>0 && 1==pattern[x+1][y-1] ){ pattern[x][y]=2; continue; }
						if( x<15&& y<15&& 1==pattern[x+1][y+1] ){ pattern[x][y]=2; continue; }
						if(        y<15&& 1==pattern[x  ][y+1] ){ pattern[x][y]=2; continue; }
						if( x>0 && y<15&& 1==pattern[x-1][y+1] ){ pattern[x][y]=2; continue; }
					}
				}
			}
		}

		//Final pass, makes sure that somewhere random gets a vertical pillar of rock which is safe
		//to dig stairs down, to permit access to anywhere within the pit from the top.
		for(n=pillarchasm; n ; --n){
			settileat( pattern , 1 , 3 , rand()&255 );
		}
		for(n=pillarwall; n ; --n){
			settileat( pattern , 2 , 3 , rand()&255 );
		}


		//Note:
		//At this point, the pattern holds:
		//0 for all tiles which will be ignored.
		//1 for all tiles set to empty pit space.
		//2 for all normal walls.
		//3 for the straight obsidian top-to-bottom wall.

		printf("\nPattern:\n");
		const char patternkey[] = ".cW!4567890123";

		//Print the pattern
		for(y=0;y<16;++y){
			for(x=0;x<16;++x){
				cout << patternkey[ pattern[x][y] ];
			}
			cout << endl;
		}
		cout << endl;

		regen = !getyesno("Acceptable Pattern?",1);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////

	//Connect to DF!    
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();



	//Init
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
    if(!Mapz->Start())
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
    DFHack::Position *Pos = DF->getPosition();
    Pos->getCursorCoords(cursorX,cursorY,cursorZ);
	if(-30000==cursorX){
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
	for(int32_t z = 0; z<= bz ;z++){
		if( ! Mapz->isValidBlock(bx,by,z) ){
			cout << "This block does't exist yet!" << endl << "Designate the lowest level for digging to make DF allocate the block, then try again." << endl;
			#ifndef LINUX_BUILD
				cin.ignore();
			#endif
			return 1;
		}
	}

	//Get all the map features.
    vector<DFHack::t_feature> global_features;
	if(!Mapz->ReadGlobalFeatures(global_features)){
			cout << "Couldn't load global features! Probably a version problem." << endl;
			#ifndef LINUX_BUILD
				cin.ignore();
			#endif
			return 1;
	}
    std::map <DFHack::planecoord, std::vector<DFHack::t_feature *> > local_features;
	if(!Mapz->ReadLocalFeatures(local_features)){
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
	planecoord pc;
	pc.dim.x=bx; pc.dim.y=by;
	mapblock40d block;
	const TileRow * tp;
	t_designation * d;

	//From top to bottom, dig this dude.

	//Top level, cap.
	Mapz->ReadBlock40d( bx, by, bz , &block );
	for(uint32_t x=0;x<16;++x){
		for(uint32_t y=0;y<16;++y){
			if(pattern[x][y]){
				tp = getTileTypeP(block.tiletypes[x][y]);
				d = &block.designation[x][y];
				//Only modify this level if it's 'empty'
				if( EMPTY != tp->c && RAMP_TOP != tp->c && STAIR_DOWN != tp->c && DFHack::TILE_STREAM_TOP != tp->s) continue;

				//Need a floor for empty space.
				if(reveal) d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
				//Always clear the dig designation.
				d->bits.dig=designation_no;
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

	//Various behaviour flags.
	int moltencount=0;
	int solidcount=0;
	int emptycount=0;
	int hellcount=0;
	int tpat;

	//All levels in between.
	uint32_t t;
	for(int32_t z = bz-1; z>0 ; --z){
		moltencount=0;
		solidcount=0;
		emptycount=0;
		hellcount=0;
		cout << z << endl;
		assert( Mapz->isValidBlock(bx,by,z) );
		if(!Mapz->ReadBlock40d( bx, by, z , &block )){
				cout << "Bad block! " << bx << "," << by << "," << z;
		}
		for(int32_t x=0;x<16;++x){
			for(int32_t y=0;y<16;++y){
				t=0;
				tp = getTileTypeP(block.tiletypes[x][y]);
				d = &block.designation[x][y];
				tpat=pattern[x][y];

				//Manipulate behaviour based on settings and location
				switch( tp->m ){
					case MAGMA:
						++moltencount;
						//Making a fake volcanoe/magma pipe?
						if( pitTypeMagma == pittype ){
							//Leave tile unchanged.
							tpat=0;
						}
						break;
					case OBSIDIAN:
					case FEATSTONE:
					case HFS:
						//ignore, even though it is technically solid
						break;
					case VEIN:
					default:
						if( EMPTY != tp->c ){
							++emptycount;
						}else{
							++solidcount;
						}
				}

				//Check hell status
				if( 
					(block.local_feature > -1 && feature_Underworld==local_features[pc][block.local_feature]->type )
					||
					(block.global_feature > -1 && feature_Underworld==global_features[block.global_feature].type )
				){
					++hellcount;

					//Are we leaving hell open?
					if( exposehell ){
						//Leave tile unchanged.
						tpat=0;
					}
				}


				//Border or space?
				switch(tpat){
				case 0:
					continue;
					break;
				case 1:
					//Empty Space
					t=32;
					//d->bits.light = topblock.designation[x][y].bits.light;
					//d->bits.skyview = topblock.designation[x][y].bits.skyview;
					//d->bits.subterranean = topblock.designation[x][y].bits.subterranean;

					//Erase special markers
					d->bits.feature_global = d->bits.feature_local = 0;

					//Water? Magma?
					if(fillmagma || fillwater){
						d->bits.flow_size=7;
						d->bits.liquid_character = liquid_fresh;
						if(fillmagma){
							d->bits.liquid_type=liquid_magma;
						}else{
							d->bits.liquid_type=liquid_water;
						}
					}

					break;
				case 2:
					//Border.
					//First guess based on current material
					switch( tp->m ){
						case OBSIDIAN:
							t=wmagma;
							break;
						case MAGMA:
							t=wmolten;
							break;
						case HFS:
							t=whell;
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

					//If the tile already is a feature, or if it is a vein, we're done.
					//Otherwise, adopt block features.
					if( VEIN!=tp->m && !d->bits.feature_global && !d->bits.feature_local ){
						//Local Feature?
						if( block.local_feature > -1 ){
							switch( n=local_features[pc][block.local_feature]->type ){
								case feature_Adamantine_Tube:
								case feature_Underworld:
								case feature_Hell_Temple:
									//Whatever the feature is made of. "featstone wall"
									d->bits.feature_local = 1;
									t=335;
									break;
								default:
									//something here. for debugging, it may be interesting to know.
									if(n) cout << '(' << n << ')';
							}
						}
						//Global Feature?
						else if(block.global_feature > -1 ){
							switch( n=global_features[block.global_feature].type ){
								case feature_Adamantine_Tube:
								case feature_Underworld:
								case feature_Hell_Temple:
									//Whatever the feature is made of. "featstone wall"
									d->bits.feature_global = 1;
									t=335;
									break;
								default:
									//something here. for debugging, it may be interesting to know.
									if(n) cout << '[' << n << ']';
							}
						}
					}

					//Erase any liquids, as they cause problems.
					d->bits.flow_size=0;
					d->bits.liquid_character = liquid_fresh;
					d->bits.liquid_type=liquid_water;

					break;
				
				case 3:
					//Special wall, always sets to obsidian, to give a stairway
					t=331;

					//Erase special markers
					d->bits.feature_global = d->bits.feature_local = 0;

					//Erase any liquids, as they cause problems.
					d->bits.flow_size=0;
					d->bits.liquid_character = liquid_fresh;
					d->bits.liquid_type=liquid_water;
					break;
				default:
					cout << ".err,bad pattern.";
				}

				//For all tiles.
				if(reveal) d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
				//Always clear the dig designation.
				d->bits.dig=designation_no;
				//unlock fluids, so they fall down the pit.
				d->bits.flow_forbid = d->bits.liquid_static=0;
				block.blockflags.bits.liquid_1 = block.blockflags.bits.liquid_2 = 1;
				//Remove aquifer, to prevent bugginess
				d->bits.water_table=0;
				//Set the tile.
				block.tiletypes[x][y] = t;

			}
		}

		//Write the block.
		Mapz->WriteBlockFlags(bx,by,z, block.blockflags ); 
		Mapz->WriteDesignations(bx,by,z, &block.designation ); 
		Mapz->WriteTileTypes(bx,by,z, &block.tiletypes ); 
		Mapz->WriteDirtyBit(bx,by,z,1); 

		//Making a fake volcanoe/magma pipe?
		if( pitTypeMagma == pittype && !solidcount){
			//Nothing «solid», we're making a magma pipe, we're done.
			z=0;
		}

	}


	//The bottom level is special.
	if( pitTypeMagma != pittype  ) {
		Mapz->ReadBlock40d( bx, by, 0 , &block );
		for(uint32_t x=0;x<16;++x){
			for(uint32_t y=0;y<16;++y){
				//Only the portion below the empty space is handled.
				if(pattern[x][y]){
					if( 1==pattern[x][y] ) t=floor;
					else if( 3==pattern[x][y] ) t=331;
					else continue;

					tp = getTileTypeP(block.tiletypes[x][y]);
					d = &block.designation[x][y];

					//For all tiles.
					if(reveal) d->bits.hidden = 0; //topblock.designation[x][y].bits.hidden;
					//Always clear the dig designation.
					d->bits.dig=designation_no;
					//unlock fluids, so they fall down the pit.
					d->bits.flow_forbid = d->bits.liquid_static=0;
					block.blockflags.bits.liquid_1 = block.blockflags.bits.liquid_2 = 1;
					//Set the tile.
					block.tiletypes[x][y] = floor;
				}
			}
		}
		//Write the block.
		Mapz->WriteBlockFlags(bx,by,0, block.blockflags ); 
		Mapz->WriteDesignations(bx,by,0, &block.designation ); 
		Mapz->WriteTileTypes(bx,by,0, &block.tiletypes ); 
		Mapz->WriteDirtyBit(bx,by,0,1); 
	}


    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
