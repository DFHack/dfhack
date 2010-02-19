// digger.cpp

// Usage: Call with a list of TileClass ids separated by a space,
// every (visible) tile on the map with that id will be designated for digging.

// NOTE currently only works with trees 

// TODO add a sort of "sub-target" to dig() to make it able to designate stone as well
// TODO add proper cli
// TODO add interactive text based menu
// TODO add ability to mark num closest to cursor

#include <iostream>
#include <integers.h>
#include <vector>
#include <list>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
using namespace std;

#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFHackAPI.h>
#include <argstream/argstream.h>

// counts the occurances of a certain element in a vector
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

// calculates the manhattan distance between two coords
int manhattan_distance(int x, int y, int z, int xx, int yy, int zz)
{
    return abs(x-xx)+abs(y-yy)+abs(z-zz);
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

    int source_distance;	// the distance to the source coords, used for sorting

    int grid_x, grid_y;
    int local_x, local_y;
    int real_x, real_y;
    int z;

    bool operator<(const DigTarget& o) const { return source_distance < o.source_distance; }
};

int dig(DFHack::API& DF, 
        vector<uint16_t>& targets,
        int num = -1,
        const int x_source = 0, 
        const int y_source = 0, 
        const int z_source = 0)
{
    if (num == 0)
        return 0; // max limit of 0, nothing to do

    uint32_t x_max,y_max,z_max;
    DFHack::t_designation designations[16][16];
    uint16_t tiles[16][16];
    DF.getSize(x_max,y_max,z_max);

    // every tile found, will later be sorted by distance to source
    vector<DigTarget> candidates;

    //cout << "============================" << endl;
    //cout << "source is " << x_source << " " << y_source << " " << z_source << endl;

    // walk the map
    for(uint32_t x = 0; x < x_max; x++)
    {
        for(uint32_t y = 0; y < y_max; y++)
        {
            for(uint32_t z = 0; z < z_max; z++)
            {
                if(DF.isValidBlock(x,y,z))
                {
                    // read block designations and tiletype
                    DF.ReadDesignations(x,y,z, (uint32_t *) designations);
                    DF.ReadTileTypes(x,y,z, (uint16_t *) tiles);

                    // search all tiles for dig targets:
                    // visible, not yet marked for dig and matching tile type
                    for(uint32_t lx = 0; lx < 16; lx++)
                    {
                        for(uint32_t ly = 0; ly < 16; ly++)
                        {
                            if (designations[lx][ly].bits.hidden == 0 && 
                                designations[lx][ly].bits.dig == 0 && 
                                vec_count(targets, DFHack::tileTypeTable[tiles[lx][ly]].c) > 0)
                            {
                                candidates.push_back(DigTarget(
                                    x, y, z,
                                    lx, ly,
                                    x_source, y_source, z_source));

                                //cout << "target found at " << world_x << " " << world_y << " " << z;
                                //cout << ", " << dt->source_distance << " tiles to source" << endl;
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

    //cout << "============================" << endl;
    //cout << "source is " << x_source << " " << y_source << " " << z_source << endl;

    // mark the tiles for actual digging
    for (vector<DigTarget>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
    {
        //cout << "designating at " << (*i).real_x << " " << (*i).real_y << " " << (*i).z;
        //cout << ", " << (*i).source_distance << " tiles to source" << endl;

        // TODO this could probably be made much better, theres a big chance the trees are on the same grid
        // TODO move into function in DigTarget
        DF.ReadDesignations((*i).grid_x, (*i).grid_y, (*i).z, (uint32_t *) designations);
        designations[(*i).local_x][(*i).local_y].bits.dig = DFHack::designation_default;
        DF.WriteDesignations((*i).grid_x, (*i).grid_y, (*i).z, (uint32_t *) designations);
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
test();

    // Command line options
    string s_targets;
    string s_origin = "0,0,0";
    bool verbose;
    int max;
    argstream as(argc,argv);

    as  >>option('v',"verbose",verbose,"Active verbose mode") // TODO handle verbose
        >>parameter('o',"origin",s_origin,"Close to where we should designate targets, format: x,y,z")
        >>parameter('t',"targets",s_targets,"What kinds of tile we should designate, format: type1,type2")
        >>parameter('m',"max",max,"The maximum limit of designated targets")
        >>help();

    // handle some commands
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
        //cout << "Usage: Call with a list of TileClass ids separated by a space,\n";
        //cout << "every (visible) tile on the map with that id will be designated for digging.\n\n";
        cout << as.usage();
    }
    else
    {
        DFHack::API DF("Memory.xml");
        if(!DF.Attach())
        {
            cerr << "DF not found" << endl;
            return 1;
        }
        DF.InitMap();

        // TODO hack until we have a proper cli to specify origin
        int x_source = 134, y_source = 134, z_source = 16; // my wagon starts here; cut trees close to wagon 
        //DF.InitViewAndCursor();
        //if (!DF.getViewCoords(x_source, y_source, z_source))
        //{
        //	cerr << "Enable cursor" << endl;
        //	return 1;
        //}

        int count = dig(DF, targets, 10, origin[0],origin[1],origin[2]); // <-- important part
        cout << count << " targets designated" << endl;

        DF.Detach();
    }
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
