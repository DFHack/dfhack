// digger.cpp

// NOTE currently only works with trees 

// TODO add a sort of "sub-target" to dig() to make it able to designate stone as well

#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
using namespace std;

#include <DFHack.h>
#include <DFTileTypes.h>
#include <argstream.h>

// counts the occurances of a certain element in a vector
// used to determine of a given tile is a target
int vec_count(vector<uint16_t>& vec, uint16_t t)
{
    int count = 0;
    for (uint32_t i = 0; i < vec.size(); ++i)
    {
        if (vec[i] == t)
            ++count;
    }
    return count;
}

// splits a string on a certain char
//
// src is the string to split
// delim is the delimiter to split the string around
// tokens is filled with every occurance between delims
void string_split(vector<string>& tokens, const std::string& src, const std::string& delim)
{
    std::string::size_type start = 0;
    std::string::size_type end;
    while (true) 
    {
        end = src.find(delim, start);
        tokens.push_back(src.substr(start, end - start));
        if (end == std::string::npos) // last token handled
            break;
        start = end + delim.size(); // skip next delim
    }
}

// this is used to parse the command line options
void parse_int_csv(vector<uint16_t>& targets, const std::string& src)
{    
    std::string::size_type start = 0;
    std::string::size_type end;
    while (true) 
    {
        end = src.find(",", start);
        targets.push_back(atoi(src.substr(start, end - start).c_str()));
        if (end == std::string::npos) // last token handled
            break;
        start = end + 1; // skip next delim
    }
}

struct DigTarget
{
    DigTarget() :
    source_distance(0),
    grid_x(0), grid_y(0),
    local_x(0), local_y(0),
    real_x(0), real_y(0), z(0)
    {
    }

    DigTarget(
        int realx, int realy, int _z, 
        int sourcex, int sourcey, int sourcez) :
    real_x(realx), real_y(realy), z(_z)
    {
        grid_x = realx/16;
        grid_y = realy/16;

        local_x = realx%16;
        local_y = realy%16;

        source_distance = manhattan_distance(
            real_x, real_y, z,
            sourcex, sourcey, sourcez);
    }

    DigTarget(
        int gridx, int gridy, int _z,
        int localx, int localy,
        int sourcex, int sourcey, int sourcez) :
    grid_x(gridx), grid_y(gridy),
    local_x(localx), local_y(localy),
    z(_z)
    {
        real_x = (grid_x*16)+local_x;
        real_y = (grid_y*16)+local_y;

        source_distance = manhattan_distance(
            real_x, real_y, z,
            sourcex, sourcey, sourcez);
    }

    int source_distance;	    // the distance to the source coords, used for sorting

    int grid_x, grid_y;         // what grid the target is in
    int local_x, local_y;       // on what coord in the grid the target is in (0-16)
    int real_x, real_y;         // real coordinates for target, thats grid*16+local
    int z;                      // z position for target, stored plain since there arent z grids

    bool operator<(const DigTarget& o) const { return source_distance < o.source_distance; }

private:
    // calculates the manhattan distance between two coords
    int manhattan_distance(int x, int y, int z, int xx, int yy, int zz)
    {
        return abs(x-xx)+abs(y-yy)+abs(z-zz);
    }
};

int dig(DFHack::Maps* Maps, 
        vector<uint16_t>& targets,
        int num = -1,
        const int x_source = 0, 
        const int y_source = 0, 
        const int z_source = 0,
        bool verbose = false)
{
    if (num == 0)
        return 0; // max limit of 0, nothing to do

    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;
    Maps->getSize(x_max,y_max,z_max);

    // every tile found, will later be sorted by distance to source
    vector<DigTarget> candidates;

    if (verbose)
        cout << "source is " << x_source << " " << y_source << " " << z_source << endl;

    // walk the map
    for(uint32_t x = 0; x < x_max; x++)
    {
        for(uint32_t y = 0; y < y_max; y++)
        {
            for(uint32_t z = 0; z < z_max; z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    // read block designations and tiletype
                    Maps->ReadDesignations(x,y,z, &designations);
                    Maps->ReadTileTypes(x,y,z, &tiles);

                    // search all tiles for dig targets:
                    // visible, not yet marked for dig and matching tile type
                    for(uint32_t lx = 0; lx < 16; lx++)
                    {
                        for(uint32_t ly = 0; ly < 16; ly++)
                        {
                            if (/*designations[lx][ly].bits.hidden == 0 && */
                                designations[lx][ly].bits.dig == 0 && 
                                vec_count(targets, DFHack::tileShape(tiles[lx][ly])) > 0)
                            {
                                DigTarget dt(
                                    x, y, z,
                                    lx, ly,
                                    x_source, y_source, z_source);
                                candidates.push_back(dt);

                                if (verbose) 
                                {
                                    cout << "target found at " << dt.real_x << " " << dt.real_y << " " << dt.z;
                                    cout << ", " << dt.source_distance << " tiles to source" << endl;
                                }
                            }
                        } // local y
                    } // local x
                }
            }
        }
    }

    // if we found more tiles than was requested, sort them by distance to source,
    // keep the front 'num' elements and drop the rest
    if (num != -1 && candidates.size() > (unsigned int)num)
    {
        sort(candidates.begin(), candidates.end());
        candidates.resize(num);
    }
    num = candidates.size();

    if (verbose)
        cout << "=== proceeding to designating targets ===" << endl;

    // mark the tiles for actual digging
    for (vector<DigTarget>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
    {
        if (verbose)
        {
            cout << "designating at " << (*i).real_x << " " << (*i).real_y << " " << (*i).z;
            cout << ", " << (*i).source_distance << " tiles to source" << endl;
        }

        // TODO this could probably be made much better, theres a big chance the trees are on the same grid
        Maps->ReadDesignations((*i).grid_x, (*i).grid_y, (*i).z, &designations);
        designations[(*i).local_x][(*i).local_y].bits.dig = DFHack::designation_default;
        Maps->WriteDesignations((*i).grid_x, (*i).grid_y, (*i).z, &designations);

        // Mark as dirty so the jobs are properly picked up by the dwarves
        Maps->WriteDirtyBit((*i).grid_x, (*i).grid_y, (*i).z, true);
    }

    return num;
}

void test()
{
    //////////////////////////
    // DigTarget
    {
        DigTarget dt(
            20, 35, 16, 
            10, 12, 14);

        assert(dt.grid_x == 1);
        assert(dt.grid_y == 2);

        assert(dt.local_x == 4);
        assert(dt.local_y == 3);

        assert(dt.real_x == 20);
        assert(dt.real_y == 35);

        assert(dt.z == 16);
        assert(dt.source_distance == 35);
    }
    {
        DigTarget dt(
            2, 4, 16,
            5, 10,
            10, 12, 14);

        assert(dt.grid_x == 2);
        assert(dt.grid_y == 4);

        assert(dt.local_x == 5);
        assert(dt.local_y == 10);

        assert(dt.real_x == 37);
        assert(dt.real_y == 74);

        assert(dt.z == 16);
        assert(dt.source_distance == 91);
    }
    
    //////////////////////////
    // string splitter
    {
        vector<string> tokens;
        string src = "10,9,11";
        string delim = ",";
        string_split(tokens, src, delim);

        assert(tokens.size() == 3);
        assert(tokens[0] == "10");
        assert(tokens[1] == "9");
        assert(tokens[2] == "11");
    }
    {
        vector<string> tokens;
        string src = "10";
        string delim = ",";
        string_split(tokens, src, delim);

        assert(tokens.size() == 1);
        assert(tokens[0] == "10");
    }
    {
        vector<uint16_t> targets;
        parse_int_csv(targets, "9,10");
        assert(targets[0] == 9);
        assert(targets[1] == 10);
    }
}

int main (int argc, char** argv)
{
    //test();

    // Command line options
    string s_targets;
    string s_origin;
    bool verbose;
    int max = 10;
    argstream as(argc,argv);

    as  >>option('v',"verbose",verbose,"Active verbose mode")
        >>parameter('o',"origin",s_origin,"Close to where we should designate targets, format: x,y,z")
        >>parameter('t',"targets",s_targets,"What kinds of tile we should designate, format: type1,type2")
        >>parameter('m',"max",max,"The maximum limit of designated targets")
        >>help();

    // some commands need extra care
    vector<uint16_t> targets;
    parse_int_csv(targets, s_targets);
    
    vector<uint16_t> origin;
    parse_int_csv(origin, s_origin);

    // sane check
    if (!as.isOk())
    {
        cout << as.errorLog();
    }
    else if (targets.size() == 0 || origin.size() != 3)
    {
        cout << as.usage();
    }
    else
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
        DFHack::Maps *Maps = DF->getMaps();
        if (Maps && Maps->Start())
        {
            int count = dig(Maps, targets, max, origin[0],origin[1],origin[2], verbose);
            cout << count << " targets designated" << endl;
            Maps->Finish();
            
            if (!DF->Detach())
            {
                cerr << "Unable to detach DF process" << endl;
            }
        }
        else
        {
            cerr << "Unable to init map" << endl;
        }
    }
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
