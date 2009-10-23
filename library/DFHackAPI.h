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

#ifndef SIMPLEAPI_H_INCLUDED
#define SIMPLEAPI_H_INCLUDED

#ifdef LINUX_BUILD
#	ifndef DFHACKAPI
#		define DFHACKAPI
#	endif
#else
#	ifdef BUILD_DFHACK_LIB
#		ifndef DFHACKAPI
#			define DFHACKAPI extern "C" __declspec(dllexport)
#		endif
#	else
#		ifndef DFHACKAPI
#			define DFHACKAPI extern "C" __declspec(dllimport)
#		endif
#	endif
#endif

#include <string>
#include <vector>
#include "integers.h"
using namespace std;

class DFHackAPI;

// Imported function for actually creating an instance
DFHACKAPI DFHackAPI *CreateDFHackAPI0(const char *path_to_xml);

// C++ wrapper for convenience
inline DFHackAPI *CreateDFHackAPI(const string &path_to_xml)
{
	return CreateDFHackAPI0(path_to_xml.c_str());
}

// Technically the functions that use std::vector are unsafe.
class DFHackAPI
{
public:
	virtual ~DFHackAPI() {}

    virtual bool Attach() = 0;
    virtual bool Detach() = 0;
    virtual bool isAttached() = 0;
    /**
     * Matgloss. next four methods look very similar. I could use two and move the processing one level up...
     * I'll keep it like this, even with the code duplication as it will hopefully get more features and separate data types later.
     * Yay for nebulous plans for a rock survey tool that tracks how much of which metal could be smelted from available resorces
     */
    virtual bool ReadStoneMatgloss(vector<t_matgloss> & output) = 0;
    virtual bool ReadWoodMatgloss (vector<t_matgloss> & output) = 0;
    virtual bool ReadMetalMatgloss(vector<t_matgloss> & output) = 0;
    virtual bool ReadPlantMatgloss(vector<t_matgloss> & output) = 0;
    virtual bool ReadCreatureMatgloss(vector<t_matgloss> & output) = 0;
    // FIXME: add creatures for all the creature products

    // read region surroundings, get their vectors of geolayers so we can do translation (or just hand the translation table to the client)
    // returns an array of 9 vectors of indices into stone matgloss
    /**
        Method for reading the geological surrounding of the currently loaded region.
        assign is a reference to an array of nine vectors of unsigned words that are to be filled with the data
        array is indexed by the BiomeOffset enum

        I omitted resolving the layer matgloss in this API, because it would
        introduce overhead by calling some method for each tile. You have to do it
        yourself. First get the stuff from ReadGeology and then for each block get
        the RegionOffsets. For each tile get the real region from RegionOffsets and
        cross-reference it with the geology stuff (region -- array of vectors, depth --
        vector). I'm thinking about turning that Geology stuff into a
        two-dimensional array with static size.

        this is the algorithm for applying matgloss:
        void DfMap::applyGeoMatgloss(Block * b)
        {
            // load layer matgloss
            for(int x_b = 0; x_b < BLOCK_SIZE; x_b++)
            {
                for(int y_b = 0; y_b < BLOCK_SIZE; y_b++)
                {
                    int geolayer = b->designation[x_b][y_b].bits.geolayer_index;
                    int biome = b->designation[x_b][y_b].bits.biome;
                    b->material[x_b][y_b].type = Mat_Stone;
                    b->material[x_b][y_b].index = v_geology[b->RegionOffsets[biome]][geolayer];
                }
            }
        }
     */
    virtual bool ReadGeology( vector < vector <uint16_t> >& assign ) = 0;

    /*
     * BLOCK DATA
     */
    /// allocate and read pointers to map blocks
    virtual bool InitMap() = 0;
    /// destroy the mapblock cache
    virtual bool DestroyMap() = 0;
    /// get size of the map in tiles
    virtual void getSize(uint32_t& x, uint32_t& y, uint32_t& z) = 0;

    /**
     * Return false/0 on failure, buffer allocated by client app, 256 items long
     */
    virtual bool isValidBlock(uint32_t blockx, uint32_t blocky, uint32_t blockz) = 0;
    
    virtual bool ReadTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer) = 0; // 256 * sizeof(uint16_t)
    virtual bool WriteTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer) = 0; // 256 * sizeof(uint16_t)
    
    virtual bool ReadDesignations(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) = 0; // 256 * sizeof(uint32_t)
    virtual bool WriteDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) = 0;
    
    virtual bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) = 0; // 256 * sizeof(uint32_t)
    virtual bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) = 0; // 256 * sizeof(uint32_t)
    
    /// read region offsets of a block
    virtual bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer) = 0; // 16 * sizeof(uint8_t)
    
    /// read aggregated veins of a block
    virtual bool ReadVeins(uint32_t blockx, uint32_t blocky, uint32_t blockz, vector <t_vein> & veins) = 0;
    
    /**
     * Buildings, constructions, plants, all pretty straighforward. InitReadBuildings returns all the building types as a mapping between a numeric values and strings
     */
    virtual uint32_t InitReadConstructions() = 0;
    virtual bool ReadConstruction(const uint32_t &index, t_construction & construction) = 0;
    virtual void FinishReadConstructions() = 0;

    virtual uint32_t InitReadBuildings(vector <string> &v_buildingtypes) = 0;
    virtual bool ReadBuilding(const uint32_t &index, t_building & building) = 0;
    virtual void FinishReadBuildings() = 0;

    virtual uint32_t InitReadVegetation() = 0;
    virtual bool ReadVegetation(const uint32_t &index, t_tree_desc & shrubbery) = 0;
    virtual void FinishReadVegetation() = 0;
    
    virtual uint32_t InitReadCreatures() = 0;
    virtual bool ReadCreature(const uint32_t &index, t_creature & furball) = 0;
    virtual void FinishReadCreatures() = 0;
};

#ifdef BUILD_DFHACK_LIB

class memory_info;
class DfVector;
class ProcessManager;
class Process;
class DataModel;

//FIXME: better control over state, creation and destruction
//TODO: give this the pimpl treatment?
class DFHackAPIImpl : public DFHackAPI
{
private:
    // internals
    uint32_t * block;
    uint32_t x_block_count, y_block_count, z_block_count;
    uint32_t regionX, regionY, regionZ;
    uint32_t worldSizeX, worldSizeY;
    
    uint32_t tile_type_offset;
    uint32_t designation_offset;
    uint32_t occupancy_offset;
    
    uint32_t creature_pos_offset;
    uint32_t creature_type_offset;
    uint32_t creature_flags1_offset;
    uint32_t creature_flags2_offset;
    
    ProcessManager* pm;
    Process* p;
    DataModel* dm;
    memory_info* offset_descriptor;
    vector<uint16_t> v_geology[eBiomeCount];
    string xml;

    bool constructionsInited;
    bool buildingsInited;
    bool vegetationInited;
    bool creaturesInited;
    uint32_t tree_offset;
    DfVector *p_cre;
    DfVector *p_cons;
    DfVector *p_bld;
    DfVector *p_veg;
    
    
public:
    DFHackAPIImpl(const string path_to_xml);

    bool Attach();
    bool Detach();
    bool isAttached();
    /**
     * Matgloss. next four methods look very similar. I could use two and move the processing one level up...
     * I'll keep it like this, even with the code duplication as it will hopefully get more features and separate data types later.
     * Yay for nebulous plans for a rock survey tool that tracks how much of which metal could be smelted from available resorces
     */
    bool ReadStoneMatgloss(vector<t_matgloss> & output);
    bool ReadWoodMatgloss (vector<t_matgloss> & output);
    bool ReadMetalMatgloss(vector<t_matgloss> & output);
    bool ReadPlantMatgloss(vector<t_matgloss> & output);
    bool ReadCreatureMatgloss(vector<t_matgloss> & output);

    // read region surroundings, get their vectors of geolayers so we can do translation (or just hand the translation table to the client)
    // returns an array of 9 vectors of indices into stone matgloss
    /**
        Method for reading the geological surrounding of the currently loaded region.
        assign is a reference to an array of nine vectors of unsigned words that are to be filled with the data
        array is indexed by the BiomeOffset enum

        I omitted resolving the layer matgloss in this API, because it would
        introduce overhead by calling some method for each tile. You have to do it
        yourself. First get the stuff from ReadGeology and then for each block get
        the RegionOffsets. For each tile get the real region from RegionOffsets and
        cross-reference it with the geology stuff (region -- array of vectors, depth --
        vector). I'm thinking about turning that Geology stuff into a
        two-dimensional array with static size.

        this is the algorithm for applying matgloss:
        void DfMap::applyGeoMatgloss(Block * b)
        {
            // load layer matgloss
            for(int x_b = 0; x_b < BLOCK_SIZE; x_b++)
            {
                for(int y_b = 0; y_b < BLOCK_SIZE; y_b++)
                {
                    int geolayer = b->designation[x_b][y_b].bits.geolayer_index;
                    int biome = b->designation[x_b][y_b].bits.biome;
                    b->material[x_b][y_b].type = Mat_Stone;
                    b->material[x_b][y_b].index = v_geology[b->RegionOffsets[biome]][geolayer];
                }
            }
        }
     */
    bool ReadGeology( vector < vector <uint16_t> >& assign );

    /*
     * BLOCK DATA
     */
    /// allocate and read pointers to map blocks
    bool InitMap();
    /// destroy the mapblock cache
    bool DestroyMap();
    /// get size of the map in tiles
    void getSize(uint32_t& x, uint32_t& y, uint32_t& z);

    /**
     * Return false/0 on failure, buffer allocated by client app, 256 items long
     */
    bool isValidBlock(uint32_t blockx, uint32_t blocky, uint32_t blockz);
    
    bool ReadTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer); // 256 * sizeof(uint16_t)
    bool WriteTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer); // 256 * sizeof(uint16_t)
    
    bool ReadDesignations(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer); // 256 * sizeof(uint32_t)
    bool WriteDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
    
    bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer); // 256 * sizeof(uint32_t)
    bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer); // 256 * sizeof(uint32_t)
    
    /// read region offsets of a block
    bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer); // 16 * sizeof(uint8_t)
    
    /// read aggregated veins of a block
    bool ReadVeins(uint32_t blockx, uint32_t blocky, uint32_t blockz, vector <t_vein> & veins);
    
    /**
     * Buildings, constructions, plants, all pretty straighforward. InitReadBuildings returns all the building types as a mapping between a numeric values and strings
     */
    uint32_t InitReadConstructions();
    bool ReadConstruction(const uint32_t &index, t_construction & construction);
    void FinishReadConstructions();

    uint32_t InitReadBuildings(vector <string> &v_buildingtypes);
    bool ReadBuilding(const uint32_t &index, t_building & building);
    void FinishReadBuildings();

    uint32_t InitReadVegetation();
    bool ReadVegetation(const uint32_t &index, t_tree_desc & shrubbery);
    void FinishReadVegetation();
    
    uint32_t InitReadCreatures();
    bool ReadCreature(const uint32_t &index, t_creature & furball);
    void FinishReadCreatures();
};

#endif

#endif // SIMPLEAPI_H_INCLUDED
