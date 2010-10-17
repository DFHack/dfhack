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



int main (void)
{
	srand ( time(NULL) );

	int64_t n;
    uint32_t x_max,y_max,z_max;

	//The Tile Type to use for the walls lining the hole
	//263 is semi-molten rock, 331 is obsidian
	uint32_t whell=263, wmolten=263, wmagma=331, wcave=331;
	//The Tile Type to use for the hole's floor at bottom of the map
	//35 is chasm, 42 is eerie pit , 340 is obsidian floor
	uint32_t floor=35, cap=340;
	//Should tiles be revealed?
	int reveal=0;


	//Pattern to dig
	unsigned char pattern[16][16] = {0,};

	{
		//Calculate a randomized circle.
		//These values found through experimentation.
		int radius=6;
		int x=0, y=0;

		for(y=-radius; y<=radius; ++y){
			for(x=-radius; x<=radius; ++x){
				if(x*x+y*y <= radius*radius + (rand()&31) ){
					pattern[7+x][7+y]=1;
				}
			}
		}
		//Post-process to figure out where to put walls.
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
		//Final pass, makes sure that somewhere random gets a vertical pillar of rock which is safe
		//to dig stairs down, to permit access to anywhere within the pit from the top.
		for(x=0, y=0; 1!=pattern[x][y]; x=rand()&15, y=rand()&15 ){}
		pattern[x][y]=3;

#if 0
		cout << endl;
		//Print the pattern (debugging)
		for(y=0;y<16;++y){
			for(x=0;x<16;++x){
				printf("%d",pattern[x][y]);
			}
			cout << endl;
		}
		cout << endl;
		cin.ignore();
		return 0;
#endif

	}


    
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();


	//Message of intent
	cout << 
		"DF Hell Hole" << endl << 
		"This program will instantly dig a hole through hell, wherever your cursor is." << endl <<
		"This can not be undone!  End program now if you don't want hellish fun." << endl
		;
	cin.ignore();

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
	for(uint32_t z = 0; z<= bz ;z++){
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

	//All levels in between.
	uint32_t t;
	for(uint32_t z = bz-1; z>0 ; --z){
		cout << z << endl;
		assert( Mapz->isValidBlock(bx,by,z) );
		if(!Mapz->ReadBlock40d( bx, by, z , &block )){
				cout << "Bad block! " << bx << "," << by << "," << z;
		}
		for(uint32_t x=0;x<16;++x){
			for(uint32_t y=0;y<16;++y){
				t=0;
				tp = getTileTypeP(block.tiletypes[x][y]);
				d = &block.designation[x][y];
				//Border or space?
				switch(pattern[x][y]){
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
	}


	//The bottom level is special.
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



    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
