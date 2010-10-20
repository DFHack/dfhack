// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>

#define DFHACK_WANT_MISCUTILS 1
#define DFHACK_WANT_TILETYPES 1
#include <DFHack.h>

using namespace DFHack;
int main (int numargs, const char ** args)
{
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();
    
BEGIN_PROBE:
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
    DFHack::VersionInfo* mem = DF->getMemoryInfo();
    DFHack::Maps *Maps = DF->getMaps();
    DFHack::Process * p = DF->getProcess();
    OffsetGroup *mapsg = mem->getGroup("Maps");
    OffsetGroup *mapblockg = mapsg->getGroup("block");
    OffsetGroup *localfeatg = mapsg->getGroup("features")->getGroup("local");

    uint32_t region_x_offset = mapsg->getAddress("region_x");
    uint32_t region_y_offset = mapsg->getAddress("region_y");
    uint32_t region_z_offset = mapsg->getAddress("region_z");

    uint32_t designatus = mapblockg->getOffset("designation");
    uint32_t block_feature1 = mapblockg->getOffset("feature_local");
    uint32_t block_feature2 = mapblockg->getOffset("feature_global");

    uint32_t feature1_start_ptr = localfeatg->getAddress("start_ptr");
    int32_t regionX, regionY, regionZ;

    // read position of the region inside DF world
    p->readDWord (region_x_offset, (uint32_t &)regionX);
    p->readDWord (region_y_offset, (uint32_t &)regionY);
    p->readDWord (region_z_offset, (uint32_t &)regionZ);

    Maps->Start();

    vector<DFHack::t_feature> global_features;
    std::map <DFHack::planecoord, std::vector<DFHack::t_feature *> > local_features;
	Maps->ReadLocalFeatures(local_features);
	Maps->ReadGlobalFeatures(global_features);

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
                std::cout << " = " << tileTypeTable[tiletype].name << std::endl;

			printf("%-10s: %4d %s\n","Class",tileTypeTable[tiletype].c,TileClassString[ tileTypeTable[tiletype].c ] , 0);
			printf("%-10s: %4d %s\n","Material",tileTypeTable[tiletype].c,TileMaterialString[ tileTypeTable[tiletype].m ] , 0);
			printf("%-10s: %4d %s\n","Special",tileTypeTable[tiletype].c,TileSpecialString[ tileTypeTable[tiletype].s ] , 0);
			printf("%-10s: %4d\n","Variant",tileTypeTable[tiletype].v , 0);
			printf("%-10s: %s\n","Direction",tileTypeTable[tiletype].d.getStr() , 0);


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

#define PRINT_FLAG( X )  printf("%-16s= %c\n", #X , ( des.X ? 'Y' : ' ' ) )
			PRINT_FLAG( hidden );
			PRINT_FLAG( light );
			PRINT_FLAG( skyview );
			PRINT_FLAG( subterranean );
			PRINT_FLAG( water_table );
			//PRINT_FLAG( rained );

			planecoord pc;
			pc.dim.x=blockX; pc.dim.y=blockY;

			PRINT_FLAG( feature_local );
			if( des.feature_local ){
				printf("%-16s  %4d (%2d) %s\n", "", 
					block.local_feature, 
					local_features[pc][block.local_feature]->type, 
					sa_feature[local_features[pc][block.local_feature]->type]
					);
			}

			PRINT_FLAG( feature_global );
			if( des.feature_global ){
				printf("%-16s  %4d (%2d) %s\n", "", 
					block.global_feature, 
					global_features[block.global_feature].type, 
					sa_feature[global_features[block.global_feature].type]
					);
			}

#undef PRINT_FLAG

            std::cout << std::endl;
        }
    }
	DF->Detach();
    #ifndef LINUX_BUILD
    //std::cout << "Done. Press any key to continue" << std::endl;
	std::cout << "Press any key to refresh..." << std::endl;
    cin.ignore();
	goto BEGIN_PROBE;
    #endif
    return 0;
}
