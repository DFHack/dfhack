// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#define DFHACK_WANT_MISCUTILS 1
#define DFHACK_WANT_TILETYPES 1
#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>

using namespace DFHack;
int main (int numargs, const char ** args)
{
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();

    #ifndef LINUX_BUILD
        BEGIN_PROBE:
    #endif
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


    DFHack::Gui *Gui = DF->getGui();
    DFHack::Materials *Materials = DF->getMaterials();
    DFHack::VersionInfo* mem = DF->getMemoryInfo();
    DFHack::Maps *Maps = DF->getMaps();
    DFHack::Process * p = DF->getProcess();
    bool hasmats = Materials->ReadInorganicMaterials();

    if(!Maps->Start())
    {
        std::cerr << "Unable to access map data!" << std::endl;
    }
    else
    {
        MapExtras::MapCache mc (Maps);

        OffsetGroup *mapsg = mem->getGroup("Maps");
        OffsetGroup *mapblockg = mapsg->getGroup("block");

        uint32_t region_x_offset = mapsg->getAddress("region_x");
        uint32_t region_y_offset = mapsg->getAddress("region_y");
        uint32_t region_z_offset = mapsg->getAddress("region_z");

        uint32_t designatus = mapblockg->getOffset("designation");
        uint32_t occup = mapblockg->getOffset("occupancy");
        uint32_t biomus =  mapblockg->getOffset("biome_stuffs");

        int32_t regionX, regionY, regionZ;

        // read position of the region inside DF world
        p->readDWord (region_x_offset, (uint32_t &)regionX);
        p->readDWord (region_y_offset, (uint32_t &)regionY);
        p->readDWord (region_z_offset, (uint32_t &)regionZ);

        vector<DFHack::t_feature> global_features;
        std::map <DFHack::DFCoord, std::vector<DFHack::t_feature *> > local_features;

        bool have_local = Maps->ReadLocalFeatures(local_features);
        bool have_global = Maps->ReadGlobalFeatures(global_features);

        int32_t cursorX, cursorY, cursorZ;
        Gui->getCursorCoords(cursorX,cursorY,cursorZ);
        if(cursorX != -30000)
        {
            DFCoord cursor (cursorX,cursorY,cursorZ);

            uint32_t blockX = cursorX / 16;
            uint32_t tileX = cursorX % 16;
            uint32_t blockY = cursorY / 16;
            uint32_t tileY = cursorY % 16;

            MapExtras::Block * b = mc.BlockAt(cursor/16);
            mapblock40d & block = b->raw;
            if(b)
            {
                printf("block addr: 0x%x\n", block.origin);
                int16_t tiletype = mc.tiletypeAt(cursor);
                naked_designation &des = block.designation[tileX][tileY].bits;

                uint32_t designato = block.origin + designatus + (tileX * 16 + tileY) * sizeof(t_designation);
                uint32_t occupr = block.origin + occup + (tileX * 16 + tileY) * sizeof(t_occupancy);

                printf("designation offset: 0x%x\n", designato);
                print_bits<uint32_t>(block.designation[tileX][tileY].whole,std::cout);
                std::cout << endl;

                printf("occupancy offset: 0x%x\n", occupr);
                print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,std::cout);
                std::cout << endl;

                // tiletype
                std::cout <<"tiletype: " << tiletype;
                if(tileTypeTable[tiletype].name)
                    std::cout << " = " << tileTypeTable[tiletype].name << std::endl;

                printf("%-10s: %4d %s\n","Class",tileTypeTable[tiletype].shape,TileClassString[ tileTypeTable[tiletype].shape ]);
                printf("%-10s: %4d %s\n","Material",tileTypeTable[tiletype].material,TileMaterialString[ tileTypeTable[tiletype].material ]);
                printf("%-10s: %4d %s\n","Special",tileTypeTable[tiletype].special,TileSpecialString[ tileTypeTable[tiletype].special ]);
                printf("%-10s: %4d\n","Variant",tileTypeTable[tiletype].variant);
                printf("%-10s: %s\n","Direction",tileTypeTable[tiletype].direction.getStr());

                std::cout << std::endl;
                std::cout <<"temperature1: " << mc.temperature1At(cursor) << " U" << std::endl;
                std::cout <<"temperature2: " << mc.temperature2At(cursor) << " U" << std::endl;

                // biome, geolayer
                std::cout << "biome: " << des.biome << std::endl;
                std::cout << "geolayer: " << des.geolayer_index << std::endl;
                int16_t base_rock = mc.baseMaterialAt(cursor);
                if(base_rock != -1)
                {
                    cout << "Layer material: " << dec << base_rock;
                    if(hasmats)
                        cout << " / " << Materials->inorganic[base_rock].id << " / " << Materials->inorganic[base_rock].name << endl;
                    else
                        cout << endl;
                }
                int16_t vein_rock = mc.veinMaterialAt(cursor);
                if(vein_rock != -1)
                {
                    cout << "Vein material (final): " << dec << vein_rock;
                    if(hasmats)
                        cout << " / " << Materials->inorganic[vein_rock].id << " / " << Materials->inorganic[vein_rock].name << endl;
                    else
                        cout << endl;
                }
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
                printf("biomestuffs: 0x%x\n", block.origin + biomus);

                #define PRINT_FLAG( X )  printf("%-16s= %c\n", #X , ( des.X ? 'Y' : ' ' ) )
                PRINT_FLAG( hidden );
                PRINT_FLAG( light );
                PRINT_FLAG( skyview );
                PRINT_FLAG( subterranean );
                PRINT_FLAG( water_table );
                PRINT_FLAG( rained );

                DFCoord pc(blockX, blockY);
                PRINT_FLAG( feature_local );

                if( des.feature_local && have_local )
                {
                    printf("%-16s  %4d (%2d) %s\n", "",
                        block.local_feature,
                        local_features[pc][block.local_feature]->type,
                        sa_feature(local_features[pc][block.local_feature]->type)
                        );
                }

                PRINT_FLAG( feature_global );

                if( des.feature_global && have_global )
                {
                    printf("%-16s  %4d (%2d) %s\n", "",
                        block.global_feature,
                        global_features[block.global_feature].type,
                        sa_feature(global_features[block.global_feature].type)
                        );
                }
                #undef PRINT_FLAG
                cout << "local feature idx: " << block.local_feature << endl;
                cout << "global feature idx: " << block.global_feature << endl;
                cout << "mystery: " << block.mystery << endl;
                std::cout << std::endl;
            }
        }
    }
    DF->Detach();
    #ifndef LINUX_BUILD
        std::cout << "Press any key to refresh..." << std::endl;
        cin.ignore();
        goto BEGIN_PROBE;
    #endif
    return 0;
}
