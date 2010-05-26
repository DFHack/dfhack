// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFHack.h>

using namespace DFHack;
int main (int numargs, const char ** args)
{
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();
    
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
    
    DFHack::Position *Pos = DF->getPosition();
    DFHack::memory_info* mem = DF->getMemoryInfo();
    DFHack::Maps *Maps = DF->getMaps();
    DFHack::Process * p = DF->getProcess();
    uint32_t designatus = mem->getOffset("map_data_designation");
    uint32_t block_feature1 = mem->getOffset("map_data_feature_local");
    uint32_t block_feature2 = mem->getOffset("map_data_feature_global");
    uint32_t region_x_offset = mem->getAddress("region_x");
    uint32_t region_y_offset = mem->getAddress("region_y");
    uint32_t region_z_offset = mem->getAddress("region_z");
    uint32_t feature1_start_ptr = mem->getAddress("local_feature_start_ptr");
    int32_t regionX, regionY, regionZ;
    
    // read position of the region inside DF world
    p->readDWord (region_x_offset, (uint32_t &)regionX);
    p->readDWord (region_y_offset, (uint32_t &)regionY);
    p->readDWord (region_z_offset, (uint32_t &)regionZ);
    
    Maps->Start();
    
    int32_t cursorX, cursorY, cursorZ;
    Pos->getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX != -30000)
    {
        uint32_t blockX = cursorX / 16;
        uint32_t tileX = cursorX % 16;
        uint32_t blockY = cursorY / 16;
        uint32_t tileY = cursorY % 16;
        t_temperatures tmpb1, tmpb2;
        mapblock40d block;
        if(Maps->ReadBlock40d(blockX,blockY,cursorZ,&block))
        {
            Maps->ReadTemperatures(blockX,blockY,cursorZ,&tmpb1, &tmpb2);
            printf("block addr: 0x%x\n", block.origin);
            int16_t tiletype = block.tiletypes[tileX][tileY];
            naked_designation &des = block.designation[tileX][tileY].bits;
            uint32_t &desw = block.designation[tileX][tileY].whole;
            print_bits<uint32_t>(block.designation[tileX][tileY].whole,cout);
            cout << endl;
            print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,cout);
            cout << endl;
            
            // tiletype
            cout <<"tiletype: " << tiletype;
            if(tileTypeTable[tiletype].name)
                cout << " = " << tileTypeTable[tiletype].name;
            cout << endl;
            
            
            cout <<"temperature1: " << tmpb1[tileX][tileY] << " U" << endl;
            cout <<"temperature2: " << tmpb2[tileX][tileY] << " U" << endl;
            
            // biome, geolayer
            cout << "biome: " << des.biome << endl;
            cout << "geolayer: " << des.geolayer_index << endl;
            // liquids
            if(des.flow_size)
            {
                if(des.liquid_type == DFHack::liquid_magma)
                    cout <<"magma: ";
                else cout <<"water: ";
                cout << des.flow_size << endl;
            }
            if(des.flow_forbid)
                cout << "flow forbid" << endl;
            if(des.pile)
                cout << "stockpile?" << endl;
            if(des.rained)
                cout << "rained?" << endl;
            if(des.smooth)
                cout << "smooth?" << endl;
            uint32_t designato = block.origin + designatus + (tileX * 16 + tileY) * sizeof(t_designation);
            printf("designation offset: 0x%x\n", designato);
            if(des.light)
                cout << "Light ";
            else
                cout << "      ";
            if(des.skyview)
                cout << "SkyView ";
            else
                cout << "        ";
            if(des.subterranean)
                cout << "Underground ";
            else
                cout << "            ";
            cout << endl;
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
