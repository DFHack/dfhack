#include <iostream>
#include <integers.h>
#include <string.h> // for memset
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <stdio.h>
#include <cstdlib>
using namespace std;

#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFHackAPI.h>
#include <modules/Maps.h>
#include <modules/Position.h>
#include <modules/Materials.h>
#include <DFTileTypes.h>
#include <argstream.h>

#define MAX_DIM 0x300
class Point
{
    public:
    Point(uint32_t x, uint32_t y, uint32_t z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    Point()
    {
        x = y = z = 0;
    }
    bool operator==(const Point &other) const
    {
        return (other.x == x && other.y == y && other.z == z);
    }
    bool operator<(const Point &other) const
    {
        return ( (y*z*1000 + y*MAX_DIM + x) < (other.y*other.z*1000 + other.y*MAX_DIM + other.x));
    }
    Point operator/(int number) const
    {
        return Point(x/number, y/number, z);
    }
    Point operator%(int number) const
    {
        return Point(x%number, y%number, z);
    }
    Point operator-(int number) const
    {
        return Point(x,y,z-number);
    }
    Point operator+(int number) const
    {
        return Point(x,y,z+number);
    }
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

class Block
{
    public:
    Block(DFHack::Maps *_m, Point _bcoord)
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
    int16_t MaterialAt(Point p)
    {
        return materials[p.x][p.y];
    }
    void ClearMaterialAt(Point p)
    {
        materials[p.x][p.y] = -1;
    }
    int16_t TileTypeAt(Point p)
    {
        return raw.tiletypes[p.x][p.y];
    }
    DFHack::t_designation DesignationAt(Point p)
    {
        return raw.designation[p.x][p.y];
    }
    bool setDesignationAt(Point p, DFHack::t_designation des)
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
    Point bcoord;
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
        map<Point, Block *>::iterator p;
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
    
    Block * BlockAt (Point blockcoord)
    {
        if(!valid) return 0;
        
        map <Point, Block*>::iterator iter = blocks.find(blockcoord);
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
    
    uint16_t tiletypeAt (Point tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->TileTypeAt(tilecoord % 16);
        }
        return 0;
    }
    
    int16_t materialAt (Point tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            return b->MaterialAt(tilecoord % 16);
        }
        return 0;
    }
    bool clearMaterialAt (Point tilecoord)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->ClearMaterialAt(tilecoord % 16);
        }
        return 0;
    }


    DFHack::t_designation designationAt (Point tilecoord)
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
    bool setDesignationAt (Point tilecoord, DFHack::t_designation des)
    {
        Block * b= BlockAt(tilecoord / 16);
        if(b && b->valid)
        {
            b->setDesignationAt(tilecoord % 16, des);
            return true;
        }
        return false;
    }
    bool testCoord (Point tilecoord)
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
        map<Point, Block *>::iterator p;
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
    map<Point, Block *> blocks;
};

int main (int argc, char* argv[])
{
    // Command line options
    bool updown = false;
    argstream as(argc,argv);

    as  >>option('x',"updown",updown,"Dig up and down stairs to reach other z-levels.")
        >>help();

    // sane check
    if (!as.isOk())
    {
        cout << as.errorLog();
        return 1;
    }
    
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
    
    uint32_t x_max,y_max,z_max;
    DFHack::Maps * Maps = DF.getMaps();
    DFHack::Materials * Mats = DF.getMaterials();
    DFHack::Position * Pos = DF.getPosition();
    
    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map. Make sure you have a map loaded in DF." << endl;
        DF.Detach();
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
        DF.Resume();
        cin.ignore();
        DF.Suspend();
        Pos->getCursorCoords(cx,cy,cz);
    }
    Point xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        cerr << "I won't dig the borders. That would be cheating!" << endl;
        DF.Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    MapCache * L = new MapCache(Maps);
    
    
    DFHack::t_designation des = L->designationAt(xy);
    int16_t tt = L->tiletypeAt(xy);
    int16_t veinmat = L->materialAt(xy);
    
    if( veinmat == -1 )
    {
        cerr << "This tile is non-vein. Bye :)" << endl;
        delete L;
        DF.Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    printf("%d/%d/%d tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);
    stack <Point> flood;
    flood.push(xy);


    while( !flood.empty() )  
    {  
        Point current = flood.top();
        flood.pop();
        int16_t vmat2 = L->materialAt(current);
        
        if(vmat2!=veinmat)
            continue;
        
        // found a good tile, dig+unset material
        
        DFHack::t_designation des = L->designationAt(current);
        DFHack::t_designation des_minus;
        DFHack::t_designation des_plus;
        int16_t vmat_minus;
        int16_t vmat_plus;
        bool below = 0;
        bool above = 0;
        if(updown)
        {
            if(L->testCoord(current-1))
            {
                below = 1;
            }
            des_minus = L->designationAt(current-1);
            vmat_minus = L->materialAt(current-1);
            if(L->testCoord(current+1))
            {
                above = 1;
            }
            des_plus = L->designationAt(current+1);
            vmat_plus = L->materialAt(current+1);
        }
        if(L->testCoord(current))
        {
            L->clearMaterialAt(current);
            if(current.x < tx_max - 2)
            {
                flood.push(Point(current.x + 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(Point(current.x + 1, current.y + 1,current.z));
                    flood.push(Point(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(Point(current.x + 1, current.y - 1,current.z));
                    flood.push(Point(current.x, current.y - 1,current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(Point(current.x - 1, current.y,current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(Point(current.x - 1, current.y + 1,current.z));
                    flood.push(Point(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(Point(current.x - 1, current.y - 1,current.z));
                    flood.push(Point(current.x, current.y - 1,current.z));
                }
            }
            if(updown)
            {
                if(current.z > 0 && below && vmat_minus == vmat2)
                {
                    flood.push(Point(current.x, current.y,current.z - 1));
                    
                    if(des_minus.bits.dig == DFHack::designation_d_stair)
                        des_minus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_minus.bits.dig = DFHack::designation_u_stair;
                    L->setDesignationAt(current-1,des_minus);
                    
                    des.bits.dig = DFHack::designation_d_stair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(Point(current.x, current.y,current.z + 1));
                    
                    if(des_plus.bits.dig == DFHack::designation_u_stair)
                        des_plus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_plus.bits.dig = DFHack::designation_d_stair;
                    L->setDesignationAt(current+1,des_plus);
                    
                    if(des.bits.dig == DFHack::designation_d_stair)
                        des.bits.dig = DFHack::designation_ud_stair;
                    else
                        des.bits.dig = DFHack::designation_u_stair;
                }
            }
            if(des.bits.dig == DFHack::designation_no)
                des.bits.dig = DFHack::designation_default;
            L->setDesignationAt(current,des);
        }
    }
    L->WriteAll();
    delete L;
    DF.Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}

