/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "DFCommon.h"
#include "DfMap.h"
#include "DfMapHeader.h"
#include "ZlibHelper.h"
/*
bool DfMap::loadMatgloss2(FILE * Decompressed)
{
    char buffer [256];
    uint32_t nummatgloss;
    uint32_t temp;
    fread(&nummatgloss, sizeof(uint32_t), 1, Decompressed);
    ///FIXME: buffer overrun possible? probably not. but fix it anyway.
    for(uint32_t i = 0; i< nummatgloss;i++)
    {
        fread(&temp, sizeof(uint32_t), 1, Decompressed); // string length
        fread(&buffer, sizeof(char), temp, Decompressed); // string
        buffer[temp] = 0;
        v_matgloss[Mat_Stone].push_back(buffer);
    }
}
bool DfMap::loadBlocks2(FILE * Decompressed,DfMapHeader & h)
{
    uint32_t x, y, z;
    uint32_t numveins;
    t_vein vein;

//    for (uint32_t tile_block = 0U; tile_block < h.tile_block_count; ++tile_block)
    {
        fread(&x, sizeof(uint32_t), 1, Decompressed);
        fread(&y, sizeof(uint32_t), 1, Decompressed);
        fread(&z, sizeof(uint32_t), 1, Decompressed);

        Block * b = allocBlock(x,y,z);

        fread(&b->tile_type, sizeof(b->tile_type), 1, Decompressed);
        fread(&b->designation, sizeof(b->designation), 1, Decompressed);
        fread(&b->occupancy, sizeof(b->occupancy), 1, Decompressed);
        fread(&b->RegionOffsets,sizeof(b->RegionOffsets),1,Decompressed);
        // load all veins of this block
        fread(&numveins, sizeof(uint32_t), 1, Decompressed);
        if(v_matgloss[Mat_Stone].size())
        {
            applyGeoMatgloss(b);
        }
        for(uint32_t i = 0; i < numveins; i++)
        {
            fread(&vein,sizeof(t_vein),1,Decompressed);
            b->veins.push_back(vein);
        }
        if(numveins)
            b->collapseVeins();
    }
}
bool DfMap::loadRegion2(FILE * Decompressed)
{
    uint32_t temp, temp2;
    for(uint32_t i = eNorthWest; i< eBiomeCount;i++)
    {
        fread(&temp, sizeof(uint32_t), 1, Decompressed); // layer vector length
        for(uint32_t j = 0; j < temp;j++) // load all geolayers into vectors (just 16bit matgloss indices)
        {
            fread(&temp2, sizeof(uint16_t), 1, Decompressed);
            v_geology[i].push_back(temp2);
        }
    }
}
bool DfMap::loadVersion2(FILE * Decompressed,DfMapHeader & h)
{
    return false;

    uint32_t tile_block_count; // how many tile blocks to read from the data location

    //uint32_t x_block_count, y_block_count, z_block_count; // DF's map block count

    fread(&tile_block_count, sizeof(uint32_t), 1, Decompressed);
    fread(&x_block_count, sizeof(uint32_t), 1, Decompressed);
    fread(&y_block_count, sizeof(uint32_t), 1, Decompressed);
    fread(&z_block_count, sizeof(uint32_t), 1, Decompressed);

    // load new size information
    //x_block_count = h.x_block_count;
    //y_block_count = h.y_block_count;
    //z_block_count = h.z_block_count;
    // make sure those size variables are in sync
    updateCellCount();
    // alloc new space for our new size
    allocBlockArray(x_block_count,y_block_count,z_block_count);

//    fseek(Decompressed, h.map_data_location, SEEK_SET);

    // read matgloss vector
    loadMatgloss2(Decompressed);
    // read region data
    loadRegion2(Decompressed);
    // read blocks
    loadBlocks2(Decompressed,h);
    return true;
}

void DfMap::writeMatgloss2(FILE * SaveFile)
{
    uint32_t nummatgloss = v_matgloss[Mat_Stone].size();
    fwrite(&nummatgloss, sizeof(uint32_t), 1, SaveFile);
    for(uint32_t i = 0; i< nummatgloss;i++)
    {
        const char *saveme = v_matgloss[Mat_Stone][i].c_str();
        uint32_t length = v_matgloss[Mat_Stone][i].size();
        fwrite(&length, sizeof(uint32_t),1,SaveFile);
        fwrite(saveme, sizeof(char), length, SaveFile);
    }
}

void DfMap::writeRegion2(FILE * SaveFile)
{
    uint32_t temp, temp2;
    for(uint32_t i = eNorthWest; i< eBiomeCount;i++)
    {
        temp = v_geology[i].size();
        fwrite(&temp, sizeof(uint32_t), 1, SaveFile); // layer vector length
        for(uint32_t j = 0; j < temp;j++) // write all geolayers (just 16bit matgloss indices)
        {
            temp2 = v_geology[i][j];
            fwrite(&temp2, sizeof(uint16_t), 1, SaveFile);
        }
    }
}

void DfMap::writeBlocks2(FILE * SaveFile)
{
    uint32_t x, y, z, numveins;
    for (x = 0; x < x_block_count; x++ )
    {
        for (y = 0; y < y_block_count; y++ )
        {
            for (z = 0; z < z_block_count; z++ )
            {
                Block *b = getBlock(x,y,z);
                if(b != NULL)
                {
                    // which block it is
                    fwrite(&x, sizeof(uint32_t), 1, SaveFile);
                    fwrite(&y, sizeof(uint32_t), 1, SaveFile);
                    fwrite(&z, sizeof(uint32_t), 1, SaveFile);
                    // block data
                    fwrite(&b->tile_type, sizeof(uint16_t), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                    fwrite(&b->designation, sizeof(uint32_t), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                    fwrite(&b->occupancy, sizeof(uint32_t), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                    fwrite(&b->RegionOffsets, sizeof(b->RegionOffsets), 1, SaveFile);
                    // write all veins
                    numveins = b->veins.size();
                    fwrite(&numveins, sizeof(uint32_t), 1, SaveFile);
                    for(uint32_t i = 0; i < numveins; i++)
                    {
                        fwrite(&b->veins[i],sizeof(t_vein),1,SaveFile);
                    }
                }
            }
        }
    }
}

bool DfMap::writeVersion2(FILE * SaveFile)
{
    // write matgloss vector
    writeMatgloss2(SaveFile);
    // write region data
    writeRegion2(SaveFile);
    // write blocks
    writeBlocks2(SaveFile);
    return true;
}
*/
