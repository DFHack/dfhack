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

//// manhattan distance
//int source_distance(int sx, int sy, int sz, 
//					int x, int y, int z, int i)
//{
//	// TODO changing x and y seems to be optimized away (?)
//	cout << x << " " << i << " " << i%16 << " " << x+(i%16) << endl;
//
//	// handle the fact that x,y,z refers to a 16x16 grid
//	//x += i%16;
//	//y += i/16;
//	int dx = i%16;
//	int dy = i/16;
//	//x *= 16;
//	//y *= 16;
//	//x += dx;
//	//y += dy;
//	return abs(sx-(x+(i%16)))+abs(sy-(y+(i/16)))+abs(sz-z);
//}

int manhattan_distance(int x, int y, int z, int xx, int yy, int zz)
{
	return abs(x-xx)+abs(y-yy)+abs(z-zz);
}

struct DigTarget
{
//public:
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
		//grid_x(realx/16), grid_y(realy/16),
		//local_x(realx%16), local_y(realy%16),
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
		//source_distance(manhattan_distance(
		//	realx, realy, realz,
		//	sourcex, sourcey, sourcez)),
		grid_x(gridx), grid_y(gridy),
		local_x(localx), local_y(localy),
		z(_z)
		//real_x(realx), real_y(realy), real_z(realz)
		{
			real_x = (grid_x*16)+local_x;
			real_y = (grid_y*16)+local_y;

			source_distance = manhattan_distance(
				real_x, real_y, z,
				sourcex, sourcey, sourcez);
		}

	int source_distance;	// the distance to the source coords, used for sorting
		//int source_distance() const { return _source_distance; }

	int grid_x, grid_y;
	int local_x, local_y;
	int real_x, real_y;
	int z;
	//int index;

	//const bool valid;

	bool operator<(const DigTarget& o) const { return source_distance < o.source_distance; }

//private:
//	int source_x, source_y, source_z;
//	int _source_distance;
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
    //uint32_t count = 0;
    DF.getSize(x_max,y_max,z_max);

	// every tile found, will later be sorted by distance to source
	vector<DigTarget> candidates;

	cout << "============================" << endl;
	cout << "source is " << x_source << " " << y_source << " " << z_source << endl;

	//int debugmaxx = 0;
	//int debugmaxy = 0;
    
    // walk the map
    for(uint32_t x = 0; x < x_max; x++)
    {
        for(uint32_t y = 0; y < y_max; y++)
        {
            for(uint32_t z = 0; z < z_max; z++)
            {
				if (z != z_source)
					continue; // hack to cut down on targets

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
								//cout << "target found at: ";
								//cout << x << "," << y << "," << z << "," << i << endl;

								//designations[i].bits.dig = DFHack::designation_default;
								//++count;

								//int realx = (x*16)+lx;
								//int realy = (y*16)+ly;

								candidates.push_back(DigTarget(
									x, y, z,
									lx, ly,
									x_source, y_source, z_source));

								//cout << "target found at " << world_x << " " << world_y << " " << z;
								//cout << ", " << dt->source_distance << " tiles to source" << endl;

								//if (world_x > debugmaxx)
								//	debugmaxx = world_x;
								//if (world_y > debugmaxy)
								//	debugmaxy = world_y;
							}
						} // local y
                    } // local x
                }
            }
        }
    }

	// TODO the following routine doesnt check if the tile is already marked for digging

	// if we found more tiles than was requested, sort them by distance to source,
	// keep the front 'num' elements and drop the rest
	if (num != -1 && candidates.size() > (unsigned int)num)
	{
		sort(candidates.begin(), candidates.end());
		candidates.resize(num);
	}
	num = candidates.size();

	cout << "============================" << endl;
	cout << "source is " << x_source << " " << y_source << " " << z_source << endl;

	// mark the tiles for actual digging
	for (vector<DigTarget>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
	{
		//int grid_x = (*i).x/16;
		//int grid_y = (*i).y/16;
		//int z = (*i).z;

		//int local_x = (*i).x%grid_x;
		//int local_y = (*i).y%grid_y;

		cout << "designating at " << (*i).real_x << " " << (*i).real_y << " " << (*i).z;
		cout << ", " << (*i).source_distance << " tiles to source" << endl;

		// TODO this could probably be made much better, theres a big chance the trees are on the same grid
		// TODO move into function in DigTarget
		DF.ReadDesignations((*i).grid_x, (*i).grid_y, (*i).z, (uint32_t *) designations);
		designations[(*i).local_x][(*i).local_y].bits.dig = DFHack::designation_default;
		DF.WriteDesignations((*i).grid_x, (*i).grid_y, (*i).z, (uint32_t *) designations);
	}

	//cout << debugmaxx << " " << debugmaxy << endl;

	return num;
}

void test()
{
	{
		DigTarget dt;
		//assert(!dt.valid);
	}

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
		//assert(dt.valid);
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
		//assert(dt.valid);
	}

	//{ // sorting
	//	DigTarget a(
	//		20, 35, 16, 
	//		10, 12, 14);

	//	DigTarget b(
	//		2, 4, 16,
	//		5, 10,
	//		10, 12, 14);

	//	vector<DigTarget> v;
	//	v.push_back(b);
	//	v.push_back(a);
	//	sort(v.begin(), v.end());
	//	assert(*(v.begin()) == a);
	//}
}

int main (int argc, const char* argv[])
{
	test();

    vector<uint16_t> targets;
    for (int i = 1; i < argc; ++i)
    {
        targets.push_back(atoi(argv[i]));
    }
    if (targets.size() == 0)
    {
        cout << "Usage: Call with a list of TileClass ids separated by a space,\n";
        cout << "every (visible) tile on the map with that id will be designated for digging.\n\n";
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
        
        int count = dig(DF, targets, 10, x_source, y_source, z_source); // <-- important part
        cout << count << " targets designated" << endl;
        
        DF.Detach();
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}