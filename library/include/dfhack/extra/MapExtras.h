#ifndef MAPEXTRAS_H
#define MAPEXTRAS_H

#include "../modules/Maps.h"
#include "../DFTileTypes.h"
#include "../DFIntegers.h"
#include <cstring>
namespace MapExtras
{
void SquashVeins (DFHack::Maps *m, DFHack::DFCoord bcoord, DFHack::mapblock40d & mb, DFHack::t_blockmaterials & materials)
{
    memset(materials,-1,sizeof(materials));
    vector <DFHack::t_vein> veins;
    m->ReadVeins(bcoord.x,bcoord.y,bcoord.z,&veins);
    //iterate through block rows
    for(uint32_t j = 0;j<16;j++)
    {
        //iterate through columns
        for (uint32_t k = 0; k< 16;k++)
        {
            int16_t tt = mb.tiletypes[k][j];
            DFHack::TileMaterial mat = DFHack::tileTypeTable[tt].m;
            if(mat == DFHack::VEIN)
            {
                for(int i = (int) veins.size() - 1; i >= 0;i--)
                {
                    if(!!(((1 << k) & veins[i].assignment[j]) >> k))
                    {
                        materials[k][j] = veins[i].type;
                        i = -1;
                    }
                }
            }
        }
    }
}

class Block
{
    public:
    Block(DFHack::Maps *_m, DFHack::DFCoord _bcoord)
    {
        m = _m;
        dirty_designations = false;
        dirty_tiletypes = false;
        dirty_temperatures = false;
        dirty_blockflags = false;
        valid = false;
        bcoord = _bcoord;
        if(m->ReadBlock40d(bcoord.x,bcoord.y,bcoord.z,&raw))
        {
            SquashVeins(m,bcoord,raw,materials);
            valid = true;
        }
        m->ReadTemperatures(bcoord.x,bcoord.y, bcoord.z,&temp1,&temp2);
    }
    int16_t MaterialAt(DFHack::DFCoord p)
    {
        return materials[p.x][p.y];
    }
    void ClearMaterialAt(DFHack::DFCoord p)
    {
        materials[p.x][p.y] = -1;
    }

    uint16_t TileTypeAt(DFHack::DFCoord p)
    {
        return raw.tiletypes[p.x][p.y];
    }
    bool setTiletypeAt(DFHack::DFCoord p, uint16_t tiletype)
    {
        if(!valid) return false;
        dirty_tiletypes = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        raw.tiletypes[p.x][p.y] = tiletype;
        return true;
    }

    uint16_t temperature1At(DFHack::DFCoord p)
    {
        return temp1[p.x][p.y];
    }
    bool setTemp1At(DFHack::DFCoord p, uint16_t temp)
    {
        if(!valid) return false;
        dirty_temperatures = true;
        temp1[p.x][p.y] = temp;
        return true;
    }

    uint16_t temperature2At(DFHack::DFCoord p)
    {
        return temp2[p.x][p.y];
    }
    bool setTemp2At(DFHack::DFCoord p, uint16_t temp)
    {
        if(!valid) return false;
        dirty_temperatures = true;
        temp2[p.x][p.y] = temp;
        return true;
    }

    DFHack::t_designation DesignationAt(DFHack::DFCoord p)
    {
        return raw.designation[p.x][p.y];
    }
    bool setDesignationAt(DFHack::DFCoord p, DFHack::t_designation des)
    {
        if(!valid) return false;
        dirty_designations = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        raw.designation[p.x][p.y] = des;
        return true;
    }

    DFHack::t_blockflags BlockFlags()
    {
        return raw.blockflags;
    }
    bool setBlockFlags(DFHack::t_blockflags des)
    {
        if(!valid) return false;
        dirty_blockflags = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        raw.blockflags = des;
        return true;
    }

    bool Write ()
    {
        if(!valid) return false;
        if(dirty_designations)
        {
            m->WriteDesignations(bcoord.x,bcoord.y,bcoord.z, &raw.designation);
            m->WriteDirtyBit(bcoord.x,bcoord.y,bcoord.z,true);
            dirty_designations = false;
        }
        if(dirty_tiletypes)
        {
            m->WriteTileTypes(bcoord.x,bcoord.y,bcoord.z, &raw.tiletypes);
            dirty_tiletypes = false;
        }
        if(dirty_temperatures)
        {
            m->WriteTemperatures(bcoord.x,bcoord.y,bcoord.z, &temp1, &temp2);
            dirty_temperatures = false;
        }
        if(dirty_blockflags)
        {
            m->WriteBlockFlags(bcoord.x,bcoord.y,bcoord.z,raw.blockflags);
            dirty_blockflags = false;
        }
        return true;
    }
    bool valid:1;
    bool dirty_designations:1;
    bool dirty_tiletypes:1;
    bool dirty_temperatures:1;
    bool dirty_blockflags:1;
    DFHack::Maps * m;
    DFHack::mapblock40d raw;
    DFHack::DFCoord bcoord;
    DFHack::t_blockmaterials materials;
    DFHack::t_temperatures temp1;
    DFHack::t_temperatures temp2;
};

class MapCache
{
    public:
    MapCache(DFHack::Maps * Maps)
    {
        valid = 0;
        this->Maps = Maps;
        Maps->getSize(x_bmax, y_bmax, z_max);
        valid = true;
    };
    ~MapCache()
    {
        map<DFHack::DFCoord, Block *>::iterator p;
        for(p = blocks.begin(); p != blocks.end(); p++)
        {
            delete p->second;
            //cout << stonetypes[p->first].id << " : " << p->second << endl;
        }
    }
    bool isValid ()
    {
        return valid;
    }
    /// get the map block at a *block* coord. Block coord = tile coord / 16
    Block * BlockAt (DFHack::DFCoord blockcoord)
    {
        if(!valid)
            return 0;
        map <DFHack::DFCoord, Block*>::iterator iter = blocks.find(blockcoord);
        if(iter != blocks.end())
        {
            return (*iter).second;
        }
        else
        {
            if(blockcoord.x < x_bmax && blockcoord.y < y_bmax && blockcoord.z < z_max)
            {
                Block * nblo = new Block(Maps,blockcoord);
                blocks[blockcoord] = nblo;
                return nblo;
            }
            return 0;
        }
    }
    uint16_t tiletypeAt (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->TileTypeAt(tilecoord % 16);
        }
        return 0;
    }
    bool setTiletypeAt(DFHack::DFCoord& tilecoord, uint16_t tiletype)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setTiletypeAt(tilecoord % 16, tiletype);
            return true;
        }
        return false;
    }

    uint16_t temperature1At (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->temperature1At(tilecoord % 16);
        }
        return 0;
    }
    bool setTemp1At(DFHack::DFCoord& tilecoord, uint16_t temperature)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setTemp1At(tilecoord % 16, temperature);
            return true;
        }
        return false;
    }

    uint16_t temperature2At (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->temperature2At(tilecoord % 16);
        }
        return 0;
    }
    bool setTemp2At(DFHack::DFCoord& tilecoord, uint16_t temperature)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setTemp2At(tilecoord % 16, temperature);
            return true;
        }
        return false;
    }

    int16_t materialAt (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->MaterialAt(tilecoord % 16);
        }
        return 0;
    }
    bool clearMaterialAt (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->ClearMaterialAt(tilecoord % 16);
        }
        return 0;
    }
    
    DFHack::t_designation designationAt (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->DesignationAt(tilecoord % 16);
        }
        DFHack:: t_designation temp;
        temp.whole = 0;
        return temp;
    }
    bool setDesignationAt (DFHack::DFCoord tilecoord, DFHack::t_designation des)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setDesignationAt(tilecoord % 16, des);
            return true;
        }
        return false;
    }
    
    bool testCoord (DFHack::DFCoord tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return true;
        }
        return false;
    }
    bool WriteAll()
    {
        map<DFHack::DFCoord, Block *>::iterator p;
        for(p = blocks.begin(); p != blocks.end(); p++)
        {
            p->second->Write();
        }
        return true;
    }
    private:
    volatile bool valid;
    uint32_t x_bmax;
    uint32_t y_bmax;
    uint32_t x_tmax;
    uint32_t y_tmax;
    uint32_t z_max;
    DFHack::Maps * Maps;
    map<DFHack::DFCoord, Block *> blocks;
};
}
#endif