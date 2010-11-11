// TO BE DEPRECATED SOON.

#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>

int main (int argc, char** argv)
{
    bool quiet = false;
    for(int i = 1; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-q")
        {
            quiet = true;
        }
	}
	int32_t x,y,z;
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;
    DFHack::t_temperatures temp1,temp2;
    uint32_t x_max,y_max,z_max;
    
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    DFHack::Maps * Maps;
    DFHack::Position * Position;
    try
    {
        DF=DFMgr.getSingleContext();
        DF->Attach();
        Maps = DF->getMaps();
        Maps->Start();
        Maps->getSize(x_max,y_max,z_max);
        Position = DF->getPosition();
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
    cout << "Welcome to the liquid spawner. type 'help' for a list of available commands, 'q' to quit." << endl;
    string mode="magma";
    string brush="point";
    string flowmode="f+";
    string setmode ="s.";
    int amount = 7;
    int width = 1;
    int height = 1;
    while(!end)
    {
        DF->Resume();
        string command = "";
        cout <<"[" << mode << ":" << amount << ":" << flowmode << ":" << setmode << "]# ";
        getline(cin, command);
        if(command=="help")
        {
            cout << "Modes:" << endl
                 << "m             - switch to magma" << endl
                 << "w             - switch to water" << endl
                 << "o             - make obsidian wall instead" << endl
                 << "f             - flow bits only" << endl
                 << "Set-Modes:" << endl
                 << "s+            - only add" << endl
                 << "s.            - set" << endl
                 << "s-            - only remove" << endl
                 << "Properties:" << endl
                 << "f+            - make the spawned liquid flow" << endl
                 << "f.            - don't change flow state (read state in flow mode)" << endl
                 << "f-            - make the spawned liquid static" << endl
                 << "0-7           - set liquid amount" << endl
                 << "Brush:" << endl
                 << "point         - single tile [p]" << endl
                 << "range         - rectangle with cursor at top left [r]" << endl
                 << "block         - block with cursor in it" << endl
                 << "Other:" << endl
                 << "q             - quit" << endl
                 << "help          - print this list of commands" << endl
                 << "empty line    - put liquid" << endl
                 << endl
                 << "Usage: point the DF cursor at a tile you want to modify" << endl
                 << "and use the commands available :)" << endl;
        }
        else if(command == "m")
        {
            mode = "magma";
        }
        else if(command == "o")
        {
            mode = "obsidian";
        }
        else if(command == "clmn")
        {
            mode = "column";
        }
        else if(command == "starruby")
        {
            mode = "starruby";
        }
        else if(command == "darkhide")
        {
            mode = "darkhide";
        }
        else if(command == "w")
        {
            mode = "water";
        }
        else if(command == "f")
        {
            mode = "flowbits";
        }
        else if(command == "point" || command == "p")
        {
            brush = "point";
        }
        else if(command == "range" || command == "r")
        {
            cout << " :set range width<" << width << "># ";
            getline(cin, command);
            width = command == "" ? width : atoi (command.c_str());
            if(width < 1) width = 1;
            cout << " :set range height<" << height << "># ";
            getline(cin, command);
            height = command == "" ? height : atoi (command.c_str());
            if(height < 1) height = 1;
            brush = "range";
        }
        else if(command == "block")
        {
            brush = "block";
        }
        else if(command == "q")
        {
            end = true;
        }
        else if(command == "f+")
        {
            flowmode = "f+";
        }
        else if(command == "f-")
        {
            flowmode = "f-";
        }
        else if(command == "f.")
        {
            flowmode = "f.";
        }
        else if(command == "s+")
        {
            setmode = "s+";
        }
        else if(command == "s-")
        {
            setmode = "s-";
        }
        else if(command == "s.")
        {
            setmode = "s.";
        }
        // blah blah, bad code, bite me.
        else if(command == "0")
            amount = 0;
        else if(command == "1")
            amount = 1;
        else if(command == "2")
            amount = 2;
        else if(command == "3")
            amount = 3;
        else if(command == "4")
            amount = 4;
        else if(command == "5")
            amount = 5;
        else if(command == "6")
            amount = 6;
        else if(command == "7")
            amount = 7;
        else if(command.empty())
        {
            DF->Suspend();
            do
            {
                if(!Maps->Start())
                {
                    cout << "Can't see any DF map loaded." << endl;
                    break;
                }
                if(!Position->getCursorCoords(x,y,z))
                {
                    cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                    break;
                }
                cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
                if(!Maps->isValidBlock(x/16,y/16,z))
                {
                    cout << "Not a valid block." << endl;
                    break;
                }
                if(mode == "obsidian")
                {
                    Maps->ReadTileTypes((x/16),(y/16),z, &tiles);
                    tiles[x%16][y%16] = 331;
                    Maps->WriteTileTypes((x/16),(y/16),z, &tiles);
                }
                else if(mode == "column")
                {
                    int zzz = z;
                    int16_t tile;
                    while ( zzz < z_max )
                    {
                        Maps->ReadTileTypes((x/16),(y/16),zzz, &tiles);
                        tile = tiles[x%16][y%16];
                        if (DFHack::tileTypeTable[tile].c == DFHack::WALL)
                            break;
                        tiles[x%16][y%16] = 331;
                        Maps->WriteTileTypes((x/16),(y/16),zzz, &tiles);
                        zzz++;
                    } 
                }
                // quick hack, do not use for serious stuff
                /*
                else if(mode == "starruby")
                {
                    if(Maps->isValidBlock((x/16),(y/16),z))
                    {
                        Maps->ReadTileTypes((x/16),(y/16),z, &tiles);
                        Maps->ReadDesignations((x/16),(y/16),z, &designations);
                        Maps->ReadTemperatures((x/16),(y/16),z, &temp1, &temp2);
                        cout << "sizeof(designations) = " << sizeof(designations) << endl;
                        cout << "sizeof(tiletypes) = " << sizeof(tiles) << endl;
                        for(uint32_t xx = 0; xx < 16; xx++) for(uint32_t yy = 0; yy < 16; yy++)
                        {
                            cout<< xx << " " << yy <<": " << tiles[xx][yy] << endl;
                            tiles[xx][yy] = 335;// 45
                            DFHack::naked_designation & des = designations[xx][yy].bits;
                            des.feature_local = true;
                            des.feature_global = false;
                            des.flow_size = 0;
                            des.skyview = 0;
                            des.light = 0;
                            des.subterranean = 1;
                            temp1[xx][yy] = 10015;
                            temp2[xx][yy] = 10015;
                        }
                        Maps->WriteTemperatures((x/16),(y/16),z, &temp1, &temp2);
                        Maps->WriteDesignations((x/16),(y/16),z, &designations);
                        Maps->WriteTileTypes((x/16),(y/16),z, &tiles);
                        Maps->WriteLocalFeature((x/16),(y/16),z, 36);
                    }
                }
                else if(mode == "darkhide")
                {
                    int16_t zzz = z;
                    while ( zzz >=0 )
                    {
                        if(Maps->isValidBlock((x/16),(y/16),zzz))
                        {
                            int xx = x %16;
                            int yy = y %16;
                            Maps->ReadDesignations((x/16),(y/16),zzz, &designations);
                            designations[xx][yy].bits.skyview = 0;
                            designations[xx][yy].bits.light = 0;
                            designations[xx][yy].bits.subterranean = 1;
                            Maps->WriteDesignations((x/16),(y/16),zzz, &designations);
                        }
                        zzz --;
                    }
                }
                */
                else
                {
                    // place the magma
                    if(brush != "range")
                    {
                        Maps->ReadDesignations((x/16),(y/16),z, &designations);
                        Maps->ReadTemperatures((x/16),(y/16),z, &temp1, &temp2);
                        if(brush == "point")
                        {
                            if(mode != "flowbits")
                            {
                                // fix temperatures so we don't produce lethal heat traps
                                if(amount == 0 || designations[x%16][y%16].bits.liquid_type == DFHack::liquid_magma && mode == "water")
                                    temp1[x%16][y%16] = temp2[x%16][y%16] = 10015;
                                DFHack::naked_designation & flow = designations[x%16][y%16].bits;
                                if(setmode == "s.")
                                {
                                    flow.flow_size = amount;
                                }
                                else if(setmode == "s+")
                                {
                                    if(flow.flow_size < amount)
                                        flow.flow_size = amount;
                                }
                                else if(setmode == "s-")
                                {
                                    if (flow.flow_size > amount)
                                        flow.flow_size = amount;
                                }
                            }
                            if(mode == "magma")
                                designations[x%16][y%16].bits.liquid_type = DFHack::liquid_magma;
                            else if(mode == "water")
                                designations[x%16][y%16].bits.liquid_type = DFHack::liquid_water;
                        }
                        else
                        {
                            for(uint32_t xx = 0; xx < 16; xx++) for(uint32_t yy = 0; yy < 16; yy++)
                            {
                                if(mode != "flowbits")
                                {
                                    // fix temperatures so we don't produce lethal heat traps
                                    if(amount == 0 || designations[xx][yy].bits.liquid_type == DFHack::liquid_magma && mode == "water")
                                        temp1[xx%16][yy%16] = temp2[xx%16][yy%16] = 10015;
                                    DFHack::naked_designation & flow= designations[xx][yy].bits;
                                    if(setmode == "s.")
                                    {
                                        flow.flow_size = amount;
                                    }
                                    else if(setmode == "s+")
                                    {
                                        if(flow.flow_size < amount)
                                            flow.flow_size = amount;
                                    }
                                    else if(setmode == "s-")
                                    {
                                        if (flow.flow_size > amount)
                                            flow.flow_size = amount;
                                    }
                                }
                                if(mode == "magma")
                                    designations[xx][yy].bits.liquid_type = DFHack::liquid_magma;
                                else if(mode == "water")
                                    designations[xx][yy].bits.liquid_type = DFHack::liquid_water;
                            }
                        }
                        Maps->WriteTemperatures((x/16),(y/16),z, &temp1, &temp2);
                        Maps->WriteDesignations(x/16,y/16,z, &designations);

                        // make the magma flow :)
                        DFHack::t_blockflags bflags;
                        Maps->ReadBlockFlags((x/16),(y/16),z,bflags);
                        // 0x00000001 = job-designated
                        // 0x0000000C = run flows? - both bit 3 and 4 required for making magma placed on a glacier flow
                        if(flowmode == "f+")
                        {
                            bflags.bits.liquid_1 = true;
                            bflags.bits.liquid_2 = true;
                        }
                        else if(flowmode == "f-")
                        {
                            bflags.bits.liquid_1 = false;
                            bflags.bits.liquid_2 = false;
                        }
                        else
                        {
                            cout << "flow bit 1 = " << bflags.bits.liquid_1 << endl; 
                            cout << "flow bit 2 = " << bflags.bits.liquid_2 << endl;
                        }
                        Maps->WriteBlockFlags((x/16),(y/16),z,bflags);
                    }
                    else if (brush == "range")
                    {
                        // Crop the range into each block if necessary
                        int beginxblock = x/16;
                        int endxblock = (x+width)/16;
                        int beginyblock = y/16;
                        int endyblock = (y+height)/16;
                        for(uint32_t bx = beginxblock; bx < endxblock+1; bx++) for(uint32_t by = beginyblock; by < endyblock+1; by++)
                        {
                            if(Maps->isValidBlock(bx,by,z))
                            {
                                Maps->ReadDesignations(bx,by,z, &designations);
                                Maps->ReadTemperatures(bx,by,z, &temp1, &temp2);
                                // Take original range and crop it into current block
                                int nx = x;
                                int ny = y;
                                int nwidth = width;
                                int nheight = height;
                                if(x/16 < bx) //Start point is left of block
                                {
                                    nx = bx*16;
                                    nwidth -= nx - x;
                                }
                                if (nx/16 < (nx+nwidth-1)/16)// End point is right of block
                                {
                                    nwidth = (bx*16)+16-nx;
                                }
                                if(y/16 < by) //Start point is above block
                                {
                                    ny = by*16;
                                    nheight -= ny - y;
                                }
                                if (ny/16 < (ny+nheight-1)/16) // End point is below block
                                {
                                    nheight = (by*16)+16-ny;
                                }
                                cout << " Block:" << bx << "," << by << ":" << endl;
                                cout << " Start:" << nx << "," << ny << ":" << endl;
                                cout << " Area: " << nwidth << "," << nheight << ":" << endl;
                                for(uint32_t xx = nx; xx < nx+nwidth; xx++) for(uint32_t yy = ny; yy < ny+nheight; yy++)
                                {
                                    if(mode != "flowbits")
                                    {
                                        // fix temperatures so we don't produce lethal heat traps
                                        if(amount == 0 || designations[xx%16][yy%16].bits.liquid_type == DFHack::liquid_magma && mode == "water")
                                            temp1[xx%16][yy%16] = temp2[xx%16][yy%16] = 10015;
                                        DFHack::naked_designation & flow= designations[xx%16][yy%16].bits;
                                        if(setmode == "s.")
                                        {
                                            flow.flow_size = amount;
                                        }
                                        else if(setmode == "s+")
                                        {
                                            if(flow.flow_size < amount)
                                                flow.flow_size = amount;
                                        }
                                        else if(setmode == "s-")
                                        {
                                            if (flow.flow_size > amount)
                                                flow.flow_size = amount;
                                        }
                                    }
                                    if(mode == "magma")
                                        designations[xx%16][yy%16].bits.liquid_type = DFHack::liquid_magma;
                                    else if(mode == "water")
                                        designations[xx%16][yy%16].bits.liquid_type = DFHack::liquid_water;
                                }
                                Maps->WriteTemperatures(bx,by,z, &temp1, &temp2);
                                Maps->WriteDesignations(bx,by,z, &designations);

                                // make the magma flow :)
                                DFHack::t_blockflags bflags;
                                Maps->ReadBlockFlags(bx,by,z,bflags);
                                // 0x00000001 = job-designated
                                // 0x0000000C = run flows? - both bit 3 and 4 required for making magma placed on a glacier flow
                                if(flowmode == "f+")
                                {
                                    bflags.bits.liquid_1 = true;
                                    bflags.bits.liquid_2 = true;
                                }
                                else if(flowmode == "f-")
                                {
                                    bflags.bits.liquid_1 = false;
                                    bflags.bits.liquid_2 = false;
                                }
                                else
                                {
                                    cout << "flow bit 1 = " << bflags.bits.liquid_1 << endl; 
                                    cout << "flow bit 2 = " << bflags.bits.liquid_2 << endl;
                                }
                                Maps->WriteBlockFlags(bx,by,z,bflags);
                            }
                        }
                    }

                }
                cout << "OK" << endl;
                Maps->Finish();
            } while (0);
        }
    }
    DF->Detach();
    #ifndef LINUX_BUILD
	if(!quiet)
	{
	    cout << "Done. Press any key to continue" << endl;
	    cin.ignore();
	}
    #endif
    return 0;
}
