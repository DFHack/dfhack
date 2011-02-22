#include <iostream>
#include <string.h> // for memset
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <stdio.h>
#include <cstdlib>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>
//#include <argstream.h>

#define MAX_DIM 0x300

//TODO: turn into the official coord class for DFHack/DF
class Vertex
{
    public:
    Vertex(uint32_t _x, uint32_t _y, uint32_t _z):x(_x),y(_y),z(_z) {}
    Vertex()
    {
        x = y = z = 0;
    }
    bool operator==(const Vertex &other) const
    {
        return (other.x == x && other.y == y && other.z == z);
    }
    // FIXME: <tomprince> peterix_: you could probably get away with not defining operator< if you defined a std::less specialization for Vertex.
    bool operator<(const Vertex &other) const
    {
        // FIXME: could be changed to eliminate MAX_DIM and make std::map lookups faster?
        return ( (z*MAX_DIM*MAX_DIM + y*MAX_DIM + x) < (other.z*MAX_DIM*MAX_DIM + other.y*MAX_DIM + other.x));
    }
    Vertex operator/(int number) const
    {
        return Vertex(x/number, y/number, z);
    }
    Vertex operator%(int number) const
    {
        return Vertex(x%number, y%number, z);
    }
    Vertex operator-(int number) const
    {
        return Vertex(x,y,z-number);
    }
    Vertex operator+(int number) const
    {
        return Vertex(x,y,z+number);
    }
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

class Block
{
    public:
    Block(DFHack::Maps *_m, Vertex _bcoord)
    {
        vector <DFHack::t_vein> veins;
        m = _m;
        dirty = false;
        valid = false;
        bcoord = _bcoord;
        if(m->ReadBlock40d(bcoord.x,bcoord.y,bcoord.z,&raw))
        {
            memset(materials,-1,sizeof(materials));
            memset(bitmap,0,sizeof(bitmap));
            m->ReadVeins(bcoord.x,bcoord.y,bcoord.z,&veins);
            // for each vein
            for(int i = 0; i < (int)veins.size();i++)
            {
                //iterate through vein rows
                for(uint32_t j = 0;j<16;j++)
                {
                    //iterate through the bits
                    for (uint32_t k = 0; k< 16;k++)
                    {
                        // check if it's really a vein (FIXME: doing this too many times)
                        int16_t tt = raw.tiletypes[k][j];
                        if(DFHack::isWallTerrain(tt) && DFHack::tileTypeTable[tt].m == DFHack::VEIN)
                        {
                            // and the bit array with a one-bit mask, check if the bit is set
                            bool set = !!(((1 << k) & veins[i].assignment[j]) >> k);
                            if(set)
                            {
                                // store matgloss
                                materials[k][j] = veins[i].type;
                            }
                        }
                    }
                }
            }
            valid = true;
        }
    }
    int16_t MaterialAt(Vertex p)
    {
        return materials[p.x][p.y];
    }
    void ClearMaterialAt(Vertex p)
    {
        materials[p.x][p.y] = -1;
    }
    int16_t TileTypeAt(Vertex p)
    {
        return raw.tiletypes[p.x][p.y];
    }
    DFHack::t_designation DesignationAt(Vertex p)
    {
        return raw.designation[p.x][p.y];
    }
    bool setDesignationAt(Vertex p, DFHack::t_designation des)
    {
        if(!valid) return false;
        dirty = true;
        //printf("setting block %d/%d/%d , %d %d\n",x,y,z, p.x, p.y);
        raw.designation[p.x][p.y] = des;
        return true;
    }
    bool WriteDesignations ()
    {
        if(!valid) return false;
        if(dirty)
        {
            //printf("writing %d/%d/%d\n",x,y,z);
            m->WriteDesignations(bcoord.x,bcoord.y,bcoord.z, &raw.designation);
            m->WriteDirtyBit(bcoord.x,bcoord.y,bcoord.z,true);
        }
        return true;
    }
    volatile bool valid;
    volatile bool dirty;
    DFHack::Maps * m;
    DFHack::mapblock40d raw;
    Vertex bcoord;
    int16_t materials[16][16];
    int8_t bitmap[16][16];
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
        map<Vertex, Block *>::iterator p;
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
    
    Block * BlockAt (Vertex blockcoord)
    {
        if(!valid) return 0;
        
        map <Vertex, Block*>::iterator iter = blocks.find(blockcoord);
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
    
    uint16_t tiletypeAt (Vertex tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->TileTypeAt(tilecoord % 16);
        }
        return 0;
    }
    
    int16_t materialAt (Vertex tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->MaterialAt(tilecoord % 16);
        }
        return 0;
    }
    bool clearMaterialAt (Vertex tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->ClearMaterialAt(tilecoord % 16);
        }
        return 0;
    }


    DFHack::t_designation designationAt (Vertex tilecoord)
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
    bool setDesignationAt (Vertex tilecoord, DFHack::t_designation des)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setDesignationAt(tilecoord % 16, des);
            return true;
        }
        return false;
    }
    bool testCoord (Vertex tilecoord)
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
        map<Vertex, Block *>::iterator p;
        for(p = blocks.begin(); p != blocks.end(); p++)
        {
            p->second->WriteDesignations();
            //cout << stonetypes[p->first].id << " : " << p->second << endl;
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
    map<Vertex, Block *> blocks;
};

int main (int argc, char* argv[])
{
    // Command line options
    bool updown = false;
    /*
    argstream as(argc,argv);

    as  >>option('x',"updown",updown,"Dig up and down stairs to reach other z-levels.")
        >>help();
        

    // sane check
    if (!as.isOk())
    {
        cout << as.errorLog();
        return 1;
    }
        */
    if(argc > 1 && strcmp(argv[1],"-x") == 0)
        updown = true;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
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

    uint32_t x_max,y_max,z_max;
    DFHack::Maps * Maps = DF->getMaps();
    DFHack::Materials * Mats = DF->getMaterials();
    DFHack::Position * Pos = DF->getPosition();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map. Make sure you have a map loaded in DF." << endl;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

    Pos->getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        cerr << "Cursor is not active. Point the cursor at a vein." << endl;
        DF->Resume();
        cin.ignore();
        DF->Suspend();
        Pos->getCursorCoords(cx,cy,cz);
    }
    Vertex xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        cerr << "I won't dig the borders. That would be cheating!" << endl;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    MapCache * MCache = new MapCache(Maps);
    
    
    DFHack::t_designation des = MCache->designationAt(xy);
    int16_t tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->materialAt(xy);
    
    if( veinmat == -1 )
    {
        cerr << "This tile is non-vein. Bye :)" << endl;
        delete MCache;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    printf("%d/%d/%d tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);
    stack <Vertex> flood;
    flood.push(xy);


    while( !flood.empty() )  
    {  
        Vertex current = flood.top();
        flood.pop();
        int16_t vmat2 = MCache->materialAt(current);
        
        if(vmat2!=veinmat)
            continue;
        
        // found a good tile, dig+unset material
        
        DFHack::t_designation des = MCache->designationAt(current);
        DFHack::t_designation des_minus;
        DFHack::t_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        bool below = 0;
        bool above = 0;
        if(updown)
        {
            if(MCache->testCoord(current-1))
            {
                below = 1;
                des_minus = MCache->designationAt(current-1);
                vmat_minus = MCache->materialAt(current-1);
            }
            
            if(MCache->testCoord(current+1))
            {
                above = 1;
                des_plus = MCache->designationAt(current+1);
                vmat_plus = MCache->materialAt(current+1);
            }
        }
        if(MCache->testCoord(current))
        {
            MCache->clearMaterialAt(current);
            if(current.x < tx_max - 2)
            {
                flood.push(Vertex(current.x + 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(Vertex(current.x + 1, current.y + 1,current.z));
                    flood.push(Vertex(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(Vertex(current.x + 1, current.y - 1,current.z));
                    flood.push(Vertex(current.x, current.y - 1,current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(Vertex(current.x - 1, current.y,current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(Vertex(current.x - 1, current.y + 1,current.z));
                    flood.push(Vertex(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(Vertex(current.x - 1, current.y - 1,current.z));
                    flood.push(Vertex(current.x, current.y - 1,current.z));
                }
            }
            if(updown)
            {
                if(current.z > 0 && below && vmat_minus == vmat2)
                {
                    flood.push(current-1);
                    
                    if(des_minus.bits.dig == DFHack::designation_d_stair)
                        des_minus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_minus.bits.dig = DFHack::designation_u_stair;
                    MCache->setDesignationAt(current-1,des_minus);
                    
                    des.bits.dig = DFHack::designation_d_stair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);
                    
                    if(des_plus.bits.dig == DFHack::designation_u_stair)
                        des_plus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_plus.bits.dig = DFHack::designation_d_stair;
                    MCache->setDesignationAt(current+1,des_plus);
                    
                    if(des.bits.dig == DFHack::designation_d_stair)
                        des.bits.dig = DFHack::designation_ud_stair;
                    else
                        des.bits.dig = DFHack::designation_u_stair;
                }
            }
            if(des.bits.dig == DFHack::designation_no)
                des.bits.dig = DFHack::designation_default;
            MCache->setDesignationAt(current,des);
        }
    }
    MCache->WriteAll();
    delete MCache;
    DF->Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}

