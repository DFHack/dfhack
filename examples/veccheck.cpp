// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <integers.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFGlobal.h>
#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>
#include <DFVector.h>
#include <DFTypes.h>
#include <modules/Materials.h>
#include <modules/Position.h>
#include <modules/Maps.h>
#include <modules/Constructions.h>
#include "miscutils.h"
#include <DFTileTypes.h>

using namespace DFHack;
/*
int __userpurge get_feature_at<eax>(__int16 tZ<ax>, __int16 tY<cx>, int world_base<ebx>, signed __int16 tX)
{
    int block; // ebp@1
    signed __int16 __tX; // di@1
    signed int _tY; // esi@1
    int designation; // eax@2
    int _tX; // ecx@2
    signed int v9; // eax@4
    int v10; // edx@4
    __int64 region_x_local; // qax@4
    __int16 v12; // cx@4
    __int16 v13; // ax@5
    int v14; // esi@5
    int result; // eax@7
    unsigned int some_stuff; // ebp@10
    int v17; // edx@11
    int _designation; // [sp+10h] [bp+4h]@2

    __tX = tX;
    LOWORD(_tY) = tY;
    block = getBlock(world_base, tX, tY, tZ);
    if ( !block )
        goto LABEL_17;
    _tX = tX;
    _tY = (signed __int16)_tY;
    designation = *(_DWORD *)(block + 0x29C + 4 * ((signed __int16)_tY % 16 + 16 * tX % 16));
    _designation = designation;
    if ( designation & 0x10000000 && *(_WORD *)(block + 0x2C) != -1 )// first feature_present bit - adamantine
    {
        region_x_local = __tX / 48 + *(_DWORD *)(world_base + 0x525C8);// tile_x / 48 + region_x
        v12 = ((BYTE4(region_x_local) & 0xF) + (_DWORD)region_x_local) >> 4;
        WORD2(region_x_local) = (_tY / 48 + *(_DWORD *)(world_base + 0x525CC)) / 16;// tile_y / 48 + region_y
        v9 = v12;
        _tX = SWORD2(region_x_local);
        v10 = *(_DWORD *)(*(_DWORD *)(*(_DWORD *)(world_base + 0x54440) + 4 * (v9 >> 4))
                          + 16 * (SWORD2(region_x_local) >> 4)
                          + 4);
        if ( v10 )
        {
            _tX %= 16;
            v14 = v10 + 24 * ((signed __int16)_tX + 16 * v9 % 16);
            v13 = *(_WORD *)(block + 0x2C);
            if ( v13 >= 0 )
            {
                _tX = (*(_DWORD *)(v14 + 16) - *(_DWORD *)(v14 + 12)) >> 2;
                if ( v13 < (unsigned int)_tX )
                    return *(_DWORD *)sub_519100(_tX, v10);
            }
        }
        designation = _designation;
    }
    if ( designation & 0x20000000 && (some_stuff = *(_DWORD *)(block + 0x30), some_stuff != -1) )// second feature_present bit - slade and hell
    {
        v17 = (*(_DWORD *)(world_base + 0x54384) - *(_DWORD *)(world_base + 0x54380)) >> 2;
        if ( some_stuff >= v17 )
            _invalid_parameter_noinfo(_tX, v17);
        result = *(_DWORD *)(*(_DWORD *)(*(_DWORD *)(world_base + 0x54380) + 4 * some_stuff) + 0x100);
    }
    else
    {
LABEL_17:
        result = 0;
    }
    return result;
}
*/
int main (int numargs, const char ** args)
{
    DFHack::API DF("Memory.xml");
    try
    {
        DF.Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::Position *Pos = DF.getPosition();
    DFHack::memory_info* mem = DF.getMemoryInfo();
    DFHack::Maps *Maps = DF.getMaps();
    DFHack::Process * p = DF.getProcess();
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
        mapblock40d block;
        if(Maps->ReadBlock40d(blockX,blockY,cursorZ,&block))
        {
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
            // local feature present
            if(des.feature_local)
            {
                cout << "local feature present: " << p->readWord(block.origin + block_feature1) << endl;
                int16_t idx = p->readWord(block.origin + block_feature1);
                if(idx != -1)
                {
                    uint64_t block48_x = cursorX / 48 + regionX;
                    cout << "cursorX: " << cursorX << ", regionX:" << regionX << endl;
                    cout << "bigblock_x: " << block48_x << endl;
                    
                    // region X coord offset by 8 big blocks (48x48 tiles)
                    uint16_t region_x_plus8 = ( block48_x + 8 ) / 16;
                    //uint16_t v12b = block48_x / 16;
                    //cout << "v12: " << v12 << " : " << v12b <<  endl;
                    // plain region Y coord
                    uint64_t region_y_local = (cursorY / 48 + regionY) / 16;
                    cout << "region_y_local: " << region_y_local << endl;
                    
                    // deref pointer to the humongo-structure
                    uint32_t base = p->readDWord(feature1_start_ptr);
                    cout << "base! : " << base << endl;
                    // this is just a few pointers to arrays of 16B (4 DWORD) structs
                    uint32_t array_elem = p->readDWord(base + (region_x_plus8 / 16) * 4);
                    cout << "array_elem : " << array_elem << endl;
                    // second element of the struct is a pointer
                    uint32_t wtf = p->readDWord(array_elem + (16*(region_y_local/16)) + 4); // rounding!
                                       
                    cout << "wtf : " << wtf << " @" << array_elem + (16*(region_y_local/16)) <<  endl;
                    if(wtf)
                    {
                        //v14 = v10 + 24 * ((signed __int16)_tX + 16 * v9 % 16);
                        uint32_t feat_vector = wtf + 24 * ((region_y_local % 16) + 16 * (region_x_plus8 % 16));
                        cout << "local feature vector: " << feat_vector << endl;
                        DfVector<uint32_t> p_features(p, feat_vector);
                        /*
                        for(int k = 0 ; k < p_features.size();k++)
                        {
                            printf("feature %d addr: 0x%x\n", k, p_features[k]);
                            string name = p->readClassName(p->readDWord( p_features[k] ));
                            cout << name << endl;
                        }
                        */
                        printf("feature %d addr: 0x%x\n", idx, p_features[idx]);
                        string name = p->readClassName(p->readDWord( p_features[idx] ));
                        bool discovered = p->readDWord( p_features[idx] + 4 );
                        cout << name << endl;
                        if(discovered)
                        {
                            cout << "You've discovered it already!" << endl;
                        }
                    }
                }
            }
            // global feature present
            if(des.feature_global)
            {
                int16_t idx = p->readWord(block.origin + block_feature2);
                cout << "global feature present: " << idx << endl;
                if(idx != -1)
                {
                    DfVector<uint32_t> p_features (p,mem->getAddress("global_feature_vector"));
                    if(idx < p_features.size())
                    {
                        uint32_t feat_ptr = p->readDWord(p_features[idx] + mem->getOffset("global_feature_funcptr_"));
                        printf("feature descriptor?: 0x%x\n", feat_ptr);
                        string name = p->readClassName(p->readDWord( feat_ptr));
                        bool discovered = p->readDWord( p_features[idx] + 4 );
                        cout << name << endl;
                        if(discovered)
                        {
                            cout << "You've discovered it already!" << endl;
                        }
                    }
                }
            }
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
            /*
            block.designation[tx][ty].bits.moss = 1;
            block.designation[tx][ty].bits.geolayer_index = 15;
            block.tiletypes[tx][ty] = 344;
            
            print_bits<uint32_t>(block.designation[tx][ty].whole,cout);
            Maps->WriteDesignations(bx,by,cz,&(block.designation));
            Maps->WriteTileTypes(bx,by,cz,&(block.tiletypes));
            */
            
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
