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

#ifndef SIMPLEAPIC_H_INCLUDED
#define SIMPLEAPIC_H_INCLUDED

#ifdef LINUX_BUILD
# ifndef DFHACKAPI
#  define DFHACKAPI extern "C"
# endif
#else
# ifdef BUILD_DFHACK_LIB
#  ifndef DFHACKAPI
#   define DFHACKAPI extern "C" __declspec(dllexport)
#  endif
# else
#  ifndef DFHACKAPI
#   define DFHACKAPI extern "C" __declspec(dllimport)
#  endif
# endif
#endif

#include <integers.h>

#ifdef __cplusplus
# include <vector>
# include <string>
using namespace std;
#endif

typedef enum DFHackAPIVectorTypeC
{
    DFHackAPIVectorTypeC_Normal, // array of struct's
    DFHackAPIVectorTypeC_Matgloss, // array of t_matgloss's
    DFHackAPIVectorTypeC_Uint16, // array of uint16_t's
    DFHackAPIVectorTypeC_Vein,  // array of t_vein's
    DFHackAPIVectorTypeC_String, // array of const char *'s
    DFHackAPIVectorTypeC_Recursive, // array of DFHackAPIVectorC struct's
    DFHackAPIVectorTypeC_DWord = 0xffffffff // Unused
} DFHackAPIVectorTypeC;

typedef struct DFHackAPIVectorC
{
    void *data;
    uint32_t length;
    DFHackAPIVectorTypeC type;
} DFHackAPIVector;

#ifdef __cplusplus
typedef class DFHackAPIImpl *DFHackAPIHandle;
#else
typedef struct DFHackAPIImpl *DFHackAPIHandle;
typedef char bool;
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// The C interface for vector management
    DFHACKAPI void DFHackAPIVector_free (DFHackAPIVectorC *vector);

// The C interface to DFHackAPI (for multiple language support)
    DFHACKAPI DFHackAPIHandle CreateDFHackAPI (const char *path_to_xml);
    DFHACKAPI void DestroyDFHackAPI (DFHackAPIHandle self);

    DFHACKAPI bool DFHackAPI_Attach (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_Detach (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_isAttached (DFHackAPIHandle self);

    DFHACKAPI bool DFHackAPI_ReadStoneMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output);
    DFHACKAPI bool DFHackAPI_ReadWoodMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output);
    DFHACKAPI bool DFHackAPI_ReadMetalMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output);
    DFHACKAPI bool DFHackAPI_ReadPlantMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output);
    DFHACKAPI bool DFHackAPI_ReadCreatureMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output);

    DFHACKAPI bool DFHackAPI_ReadGeology (DFHackAPIHandle self, DFHackAPIVectorC *assign);

    DFHACKAPI bool DFHackAPI_InitMap (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_DestroyMap (DFHackAPIHandle self);
    DFHACKAPI void DFHackAPI_getSize (DFHackAPIHandle self, uint32_t* x, uint32_t* y, uint32_t* z);

    DFHACKAPI bool DFHackAPI_isValidBlock (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz);

    DFHACKAPI bool DFHackAPI_ReadTileTypes (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer);
    DFHACKAPI bool DFHackAPI_WriteTileTypes (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer);

    DFHACKAPI bool DFHackAPI_ReadDesignations (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
    DFHACKAPI bool DFHackAPI_WriteDesignations (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);

    DFHACKAPI bool DFHackAPI_ReadOccupancy (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
    DFHACKAPI bool DFHackAPI_WriteOccupancy (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);

    DFHACKAPI bool DFHackAPI_ReadRegionOffsets (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer);

    DFHACKAPI bool DFHackAPI_ReadVeins (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, DFHackAPIVectorC * veins);


    DFHACKAPI uint32_t DFHackAPI_InitReadConstructions (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_ReadConstruction (DFHackAPIHandle self, const uint32_t *index, t_construction * construction);
    DFHACKAPI void DFHackAPI_FinishReadConstructions (DFHackAPIHandle self);

    DFHACKAPI uint32_t DFHackAPI_InitReadBuildings (DFHackAPIHandle self, DFHackAPIVectorC *v_buildingtypes);
    DFHACKAPI bool DFHackAPI_ReadBuilding (DFHackAPIHandle self, const uint32_t *index, t_building * building);
    DFHACKAPI void DFHackAPI_FinishReadBuildings (DFHackAPIHandle self);

    DFHACKAPI uint32_t DFHackAPI_InitReadVegetation (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_ReadVegetation (DFHackAPIHandle self, const uint32_t *index, t_tree_desc * shrubbery);
    DFHACKAPI void DFHackAPI_FinishReadVegetation (DFHackAPIHandle self);

    DFHACKAPI uint32_t DFHackAPI_InitReadCreatures (DFHackAPIHandle self);
    DFHACKAPI bool DFHackAPI_ReadCreature (DFHackAPIHandle self, const uint32_t *index, t_creature * furball);
    DFHACKAPI void DFHackAPI_FinishReadCreatures (DFHackAPIHandle self);

#ifdef __cplusplus
}
#endif // __cplusplus

// C++ wrappers for C API that use vectors
#ifdef __cplusplus
inline bool DFHackAPI_ReadStoneMatgloss (DFHackAPIHandle self, vector<t_matgloss> & output)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadStoneMatgloss (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        output.push_back ( ( (t_matgloss *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline bool DFHackAPI_ReadWoodMatgloss (DFHackAPIHandle self, vector<t_matgloss> & output)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadWoodMatgloss (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        output.push_back ( ( (t_matgloss *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline bool DFHackAPI_ReadMetalMatgloss (DFHackAPIHandle self, vector<t_matgloss> & output)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadMetalMatgloss (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        output.push_back ( ( (t_matgloss *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline bool DFHackAPI_ReadPlantMatgloss (DFHackAPIHandle self, vector<t_matgloss> & output)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadPlantMatgloss (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        output.push_back ( ( (t_matgloss *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline bool DFHackAPI_ReadCreatureMatgloss (DFHackAPIHandle self, vector<t_matgloss> & output)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadCreatureMatgloss (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        output.push_back ( ( (t_matgloss *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline bool DFHackAPI_ReadGeology (DFHackAPIHandle self, vector< vector<uint16_t> > &assign)
{
    DFHackAPIVectorC vec;
    bool result = DFHackAPI_ReadGeology (self, &vec);
    uint32_t i;
    for (i = 0; i < vec.length; i++)
    {
        DFHackAPIVectorC &current = ( (DFHackAPIVectorC *) vec.data) [i];
        vector<uint16_t> fill;
        uint32_t j;
        for (j = 0; j < current.length; j++)
            fill.push_back ( ( (uint16_t *) current.data) [j]);
        assign.push_back (fill);
    }
    DFHackAPIVector_free (&vec);
    return result;
}

inline bool DFHackAPI_ReadVeins (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, vector <t_vein> & veins)
{
    DFHackAPIVectorC vector;
    bool result = DFHackAPI_ReadVeins (self, blockx, blocky, blockz, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        veins.push_back ( ( (t_vein *) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}

inline uint32_t DFHackAPI_InitReadBuildings (DFHackAPIHandle self, vector <string> &v_buildingtypes)
{
    DFHackAPIVectorC vector;
    uint32_t result = DFHackAPI_InitReadBuildings (self, &vector);
    uint32_t i;
    for (i = 0; i < vector.length; i++)
        v_buildingtypes.push_back ( ( (const char **) vector.data) [i]);
    DFHackAPIVector_free (&vector);
    return result;
}
#endif // __cplusplus

// C++ class wrapper for C DFHackAPI
#ifdef __cplusplus
class CDFHackAPI
{
    DFHackAPIHandle handle;
public:
    CDFHackAPI (const string &path_to_xml)
            : handle (CreateDFHackAPI (path_to_xml.c_str()))
    {
        if (handle == NULL)
        {
            // TODO: handle failure
        }
    }

    inline ~CDFHackAPI()
    {
        DestroyDFHackAPI (handle);
        handle = NULL;
    }

    inline bool Attach()
    {
        return DFHackAPI_Attach (handle);
    }

    inline bool Detach()
    {
        return DFHackAPI_Detach (handle);
    }

    inline bool isAttached()
    {
        return DFHackAPI_isAttached (handle);
    }

    /**
     * Matgloss. next four methods look very similar. I could use two and move the processing one level up...
     * I'll keep it like this, even with the code duplication as it will hopefully get more features and separate data types later.
     * Yay for nebulous plans for a rock survey tool that tracks how much of which metal could be smelted from available resorces
     */
    inline bool ReadStoneMatgloss (vector<t_matgloss> & output)
    {
        return DFHackAPI_ReadStoneMatgloss (handle, output);
    }

    inline bool ReadWoodMatgloss (vector<t_matgloss> & output)
    {
        return DFHackAPI_ReadWoodMatgloss (handle, output);
    }

    inline bool ReadMetalMatgloss (vector<t_matgloss> & output)
    {
        return DFHackAPI_ReadMetalMatgloss (handle, output);
    }

    inline bool ReadPlantMatgloss (vector<t_matgloss> & output)
    {
        return DFHackAPI_ReadPlantMatgloss (handle, output);
    }

    inline bool ReadCreatureMatgloss (vector<t_matgloss> & output)
    {
        return DFHackAPI_ReadCreatureMatgloss (handle, output);
    }

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
    inline bool ReadGeology (vector < vector <uint16_t> >& assign)
    {
        return DFHackAPI_ReadGeology (handle, assign);
    }

    /*
     * BLOCK DATA
     */
    /// allocate and read pointers to map blocks
    inline bool InitMap()
    {
        return DFHackAPI_InitMap (handle);
    }

    /// destroy the mapblock cache
    inline bool DestroyMap()
    {
        return DFHackAPI_DestroyMap (handle);
    }

    /// get size of the map in tiles
    inline void getSize (uint32_t& x, uint32_t& y, uint32_t& z)
    {
        DFHackAPI_getSize (handle, &x, &y, &z);
    }

    /**
     * Return false/0 on failure, buffer allocated by client app, 256 items long
     */
    inline bool isValidBlock (uint32_t blockx, uint32_t blocky, uint32_t blockz)
    {
        return DFHackAPI_isValidBlock (handle, blockx, blocky, blockz);
    }

    inline bool ReadTileTypes (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer) // 256 * sizeof(uint16_t)
    {
        return DFHackAPI_ReadTileTypes (handle, blockx, blocky, blockz, buffer);
    }

    inline bool WriteTileTypes (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer) // 256 * sizeof(uint16_t)
    {
        return DFHackAPI_WriteTileTypes (handle, blockx, blocky, blockz, buffer);
    }

    inline bool ReadDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) // 256 * sizeof(uint32_t)
    {
        return DFHackAPI_ReadDesignations (handle, blockx, blocky, blockz, buffer);
    }

    inline bool WriteDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer)
    {
        return DFHackAPI_WriteDesignations (handle, blockx, blocky, blockz, buffer);
    }

    inline bool ReadOccupancy (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) // 256 * sizeof(uint32_t)
    {
        return DFHackAPI_ReadOccupancy (handle, blockx, blocky, blockz, buffer);
    }

    inline bool WriteOccupancy (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer) // 256 * sizeof(uint32_t)
    {
        return DFHackAPI_WriteOccupancy (handle, blockx, blocky, blockz, buffer);
    }

    /// read region offsets of a block
    inline bool ReadRegionOffsets (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer) // 16 * sizeof(uint8_t)
    {
        return DFHackAPI_ReadRegionOffsets (handle, blockx, blocky, blockz, buffer);
    }

    /// read aggregated veins of a block
    inline bool ReadVeins (uint32_t blockx, uint32_t blocky, uint32_t blockz, vector <t_vein> & veins)
    {
        return DFHackAPI_ReadVeins (handle, blockx, blocky, blockz, veins);
    }

    /**
     * Buildings, constructions, plants, all pretty straighforward. InitReadBuildings returns all the building types as a mapping between a numeric values and strings
     */
    inline uint32_t InitReadConstructions()
    {
        return DFHackAPI_InitReadConstructions (handle);
    }

    inline bool ReadConstruction (const uint32_t &index, t_construction & construction)
    {
        return DFHackAPI_ReadConstruction (handle, &index, & construction);
    }

    inline void FinishReadConstructions()
    {
        DFHackAPI_FinishReadConstructions (handle);
    }

    inline uint32_t InitReadBuildings (vector <string> &v_buildingtypes)
    {
        return DFHackAPI_InitReadBuildings (handle, v_buildingtypes);
    }

    inline bool ReadBuilding (const uint32_t &index, t_building & building)
    {
        return DFHackAPI_ReadBuilding (handle, &index, &building);
    }

    inline void FinishReadBuildings()
    {
        DFHackAPI_FinishReadBuildings (handle);
    }

    inline uint32_t InitReadVegetation()
    {
        return DFHackAPI_InitReadVegetation (handle);
    }

    inline bool ReadVegetation (const uint32_t &index, t_tree_desc & shrubbery)
    {
        return DFHackAPI_ReadVegetation (handle, &index, &shrubbery);
    }

    inline void FinishReadVegetation()
    {
        DFHackAPI_FinishReadVegetation (handle);
    }
    
    inline uint32_t InitReadCreatures()
    {
        return DFHackAPI_InitReadCreatures (handle);
    }
    
    inline bool ReadCreature (const uint32_t &index, t_creature & furball)
    {
        return DFHackAPI_ReadCreature (handle, &index, &furball);
    }
    
    inline void FinishReadCreatures()
    {
        DFHackAPI_FinishReadCreatures (handle);
    }
    
    
};
#endif // __cplusplus

#endif // SIMPLEAPIC_H_INCLUDED
