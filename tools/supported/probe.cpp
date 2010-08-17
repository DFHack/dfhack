// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>

#define DFHACK_WANT_MISCUTILS
#define DFHACK_WANT_TILETYPES
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
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
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
            print_bits<uint32_t>(block.designation[tileX][tileY].whole,std::cout);
            std::cout << endl;
            print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,std::cout);
            std::cout << endl;
            
            // tiletype
            std::cout <<"tiletype: " << tiletype;
            if(tileTypeTable[tiletype].name)
                std::cout << " = " << tileTypeTable[tiletype].name;
            std::cout << std::endl;
            
            
            std::cout <<"temperature1: " << tmpb1[tileX][tileY] << " U" << std::endl;
            std::cout <<"temperature2: " << tmpb2[tileX][tileY] << " U" << std::endl;
            
            // biome, geolayer
            std::cout << "biome: " << des.biome << std::endl;
            std::cout << "geolayer: " << des.geolayer_index << std::endl;
            // liquids
            if(des.flow_size)
            {
                if(des.liquid_type == DFHack::liquid_magma)
                    std::cout <<"magma: ";
                else std::cout <<"water: ";
                std::cout << des.flow_size << std::endl;
            }
            if(des.flow_forbid)
                std::cout << "flow forbid" << std::endl;
            if(des.pile)
                std::cout << "stockpile?" << std::endl;
            if(des.rained)
                std::cout << "rained?" << std::endl;
            if(des.smooth)
                std::cout << "smooth?" << std::endl;
            uint32_t designato = block.origin + designatus + (tileX * 16 + tileY) * sizeof(t_designation);
            printf("designation offset: 0x%x\n", designato);
            if(des.light)
                std::cout << "Light ";
            else
                std::cout << "      ";
            if(des.skyview)
                std::cout << "SkyView ";
            else
                std::cout << "        ";
            if(des.subterranean)
                std::cout << "Underground ";
            else
                std::cout << "            ";
            std::cout << std::endl;
        }
    }
    #ifndef LINUX_BUILD
    std::cout << "Done. Press any key to continue" << std::endl;
    cin.ignore();
    #endif
    return 0;
}
