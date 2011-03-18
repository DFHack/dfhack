/*******************************************************************************
                                    M A P S
                            Read and write DF's map
*******************************************************************************/
#ifndef CL_MOD_MAPS
#define CL_MOD_MAPS

#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
#include "Vegetation.h"

/**
 * \defgroup grp_maps Maps module and its types
 * @ingroup grp_modules
 */

namespace DFHack
{
    /***************************************************************************
                                    T Y P E S
    ***************************************************************************/
    /**
     * \ingroup grp_maps
     */
    enum e_feature
    {
        feature_Other,
        feature_Adamantine_Tube,
        feature_Underworld,
        feature_Hell_Temple
    };
    /**
     * Function for translating feature index to its name
     * \ingroup grp_maps
     */
    extern DFHACK_EXPORT const char * sa_feature(e_feature index);

    /**
     * Class for holding a world coordinate. Can do math with coordinates and can be used as an index for std::map
     * \ingroup grp_maps
     */
    class DFCoord
    {
        public:
        DFCoord(uint16_t _x, uint16_t _y, uint16_t _z = 0): x(_x), y(_y), z(_z) {}
        DFCoord()
        {
            comparate = 0;
        }
        bool operator==(const DFCoord &other) const
        {
            return (other.comparate == comparate);
        }
        // FIXME: <tomprince> peterix_: you could probably get away with not defining operator< if you defined a std::less specialization for Vertex.
        bool operator<(const DFCoord &other) const
        {
            return comparate < other.comparate;
        }
        DFCoord operator/(int number) const
        {
            return DFCoord(x/number, y/number, z);
        }
        DFCoord operator*(int number) const
        {
            return DFCoord(x*number, y*number, z);
        }
        DFCoord operator%(int number) const
        {
            return DFCoord(x%number, y%number, z);
        }
        DFCoord operator-(int number) const
        {
            return DFCoord(x,y,z-number);
        }
        DFCoord operator+(int number) const
        {
            return DFCoord(x,y,z+number);
        }
        // this is a hack. beware.
        // x,y,z share the same space with comparate. comparate can be used for fast comparisons
        union
        {
            // new shiny DFCoord struct. notice the ludicrous space for Z-levels
            struct
            {
                uint16_t x;
                uint16_t y;
                uint32_t z;
            };
            // old planeccord struct for compatibility
            struct
            {
                uint16_t x;
                uint16_t y;
            } dim;
            // comparing thing
            uint64_t comparate;
        };
    };
    /**
     * \ingroup grp_maps
     */
    typedef DFCoord planecoord;

    /**
     * A local or global map feature
     * \ingroup grp_maps
     */
    struct t_feature
    {
        e_feature type;
        /// main material type - decides between stuff like bodily fluids, inorganics, vomit, amber, etc.
        int16_t main_material;
        /// generally some index to a vector of material types.
        int32_t sub_material;
        /// placeholder
        bool discovered;
        /// this is NOT part of the DF feature, but an address of the feature as seen by DFhack.
        uint32_t origin;
    };

    /**
     * mineral vein object - bitmap with a material type
     * \ingroup grp_maps
     */
    struct t_vein
    {
        uint32_t vtable;
        /// index into the inorganic material vector
        int32_t type;
        /// bit mask describing how the vein maps to the map block
        /// assignment[y] & (1 << x) describes the tile (x, y) of the block
        int16_t assignment[16];
        uint32_t flags;
        /// this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
        uint32_t address_of; 
    };

    /**
     * stores what tiles should appear when the ice melts - bitmap of material types
     * \ingroup grp_maps
     */
    struct t_frozenliquidvein
    {
        uint32_t vtable;
        /// a 16x16 array of the original tile types
        int16_t tiles[16][16];
        /// this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
        uint32_t address_of;
    };

    /**
     * a 'spattervein' defines what coverings the individual map tiles have (snow, blood, etc)
     * bitmap of intensity with matrial type
     * \ingroup grp_maps
     * @see PrintSplatterType
     */
    struct t_spattervein
    {
        uint32_t vtable;
        /// generic material.
        uint16_t mat1;
        uint16_t unk1;
        /// material vector index
        uint32_t mat2;
        /// something even more specific?
        uint16_t mat3;
        /// 16x16 array of covering 'intensity'
        uint8_t intensity[16][16];
        /// this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
        uint32_t address_of;
    };
    /**
     * a 'grass vein' defines the grass coverage of a map block
     * bitmap of density (max = 100) with plant material type
     * \ingroup grp_maps
     */
    struct t_grassvein
    {
        uint32_t vtable;
        /// material vector index
        uint32_t material;
        /// 16x16 array of covering 'intensity'
        uint8_t intensity[16][16];
        /// this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
        uint32_t address_of;
    };
    /**
     * defines the world constructions present. The material member is a mystery.
     * \ingroup grp_maps
     */
    struct t_worldconstruction
    {
        uint32_t vtable;
        /// material vector index
        uint32_t material;
        /// 16x16 array of bits
        uint16_t assignment[16];
        /// this is NOT part of the structure, but an address of it as seen by DFhack.
        uint32_t address_of;
    };

    /**
     * \ingroup grp_maps
     */
    enum BiomeOffset
    {
        eNorthWest,
        eNorth,
        eNorthEast,
        eWest,
        eHere,
        eEast,
        eSouthWest,
        eSouth,
        eSouthEast,
        eBiomeCount
    };

    /**
     * \ingroup grp_maps
     */
    enum e_traffic
    {
        traffic_normal,
        traffic_low,
        traffic_high,
        traffic_restricted
    };

    /**
     * type of a designation for a tile
     * \ingroup grp_maps
     */
    enum e_designation
    {
        /// no designation
        designation_no,
        /// dig walls, remove stairs and ramps, gather plants, fell trees. depends on tile type
        designation_default,
        /// dig up/down stairs
        designation_ud_stair,
        /// dig a channel
        designation_channel,
        /// dig ramp out of a wall
        designation_ramp,
        /// dig a stair down
        designation_d_stair,
        /// dig a stair up
        designation_u_stair,
        /// whatever. for completenes I guess
        designation_7
    };
    
    /**
     * type of liquid in a tile
     * \ingroup grp_maps
     */
    enum e_liquidtype
    {
        liquid_water,
        liquid_magma
    };

    /**
     * designation bit field
     * \ingroup grp_maps
     */
    struct naked_designation
    {
        unsigned int flow_size : 3; // how much liquid is here?
        unsigned int pile : 1; // stockpile?
        /// All the different dig designations
        e_designation dig : 3;
        unsigned int smooth : 2;
        unsigned int hidden : 1;

        /**
         * This one is rather involved, but necessary to retrieve the base layer matgloss index
         * @see http://www.bay12games.com/forum/index.php?topic=608.msg253284#msg253284
         */
        unsigned int geolayer_index :4;
        unsigned int light : 1;
        unsigned int subterranean : 1; // never seen the light of day?
        unsigned int skyview : 1; // sky is visible now, it rains in here when it rains

        /**
         * Probably similar to the geolayer_index. Only with a different set of offsets and different data.
         * we don't use this yet
         */
        unsigned int biome : 4;
        /**
         * 0 = water
         * 1 = magma
         */
        e_liquidtype liquid_type : 1;
        unsigned int water_table : 1; // srsly. wtf?
        unsigned int rained : 1; // does this mean actual rain (as in the blue blocks) or a wet tile?
        e_traffic traffic : 2;
        /// the tile is not evaluated when calculating flows?
        unsigned int flow_forbid : 1;
        /// no liquid spreading
        unsigned int liquid_static : 1;
        /// this tile is a part of a local feature. can be combined with 'featstone' tiles
        unsigned int feature_local : 1; 
        /// this tile is a part of a global feature. can be combined with 'featstone' tiles
        unsigned int feature_global : 1;
        unsigned int water_stagnant : 1;
        unsigned int water_salt : 1;
        // e_liquidcharacter liquid_character : 2;
    };
    /**
     * designation bit field wrapper
     * \ingroup grp_maps
     */
    union t_designation
    {
        uint32_t whole;
        naked_designation bits;
    };

    /**
     * occupancy flags (rat,dwarf,horse,built wall,not build wall,etc)
     * \ingroup grp_maps
     */
    struct naked_occupancy //FIXME: THIS IS NOT VALID FOR 31.xx versions!!!!
    {
        // building type... should be an enum?
        // 7 = door
        unsigned int building : 3;
        /// the tile contains a standing? creature
        unsigned int unit : 1;
        /// the tile contains a prone creature
        unsigned int unit_grounded : 1;
        /// the tile contains an item
        unsigned int item : 1;
        /// changed
        unsigned int unknown : 26;
        /*
        /// splatter. everyone loves splatter. this doesn't seem to be used anymore
        unsigned int mud : 1;
        unsigned int vomit :1;
        unsigned int broken_arrows_color :4;
        unsigned int blood_g : 1;
        unsigned int blood_g2 : 1;
        unsigned int blood_b : 1;
        unsigned int blood_b2 : 1;
        unsigned int blood_y : 1;
        unsigned int blood_y2 : 1;
        unsigned int blood_m : 1;
        unsigned int blood_m2 : 1;
        unsigned int blood_c : 1;
        unsigned int blood_c2 : 1;
        unsigned int blood_w : 1;
        unsigned int blood_w2 : 1;
        unsigned int blood_o : 1;
        unsigned int blood_o2 : 1;
        unsigned int slime : 1;
        unsigned int slime2 : 1;
        unsigned int blood : 1;
        unsigned int blood2 : 1;
        unsigned int broken_arrows_variant : 1;
        unsigned int snow : 1;
        */
    };
    /**
     * occupancy flags (rat,dwarf,horse,built wall,not build wall,etc) wrapper
     * \ingroup grp_maps
     */
    union t_occupancy
    {
        uint32_t whole;
        naked_occupancy bits;
    };

    /**
     * map block flags
     * \ingroup grp_maps
     */
    struct naked_blockflags
    {
        /// designated for jobs (digging and stuff like that)
        unsigned int designated : 1;
        /// possibly related to the designated flag
        unsigned int unk_1 : 1;
        /// two flags required for liquid flow.
        unsigned int liquid_1 : 1;
        unsigned int liquid_2 : 1;
        /// rest of the flags is completely unknown
        unsigned int unk_2: 28;
        // there's a possibility that this flags field is shorter than 32 bits
    };

    /**
     * map block flags wrapper
     * \ingroup grp_maps
     */
    union t_blockflags
    {
        uint32_t whole;
        naked_blockflags bits;
    };

    /**
     * 16x16 array of tile types
     * \ingroup grp_maps
     */
    typedef int16_t tiletypes40d [16][16];
    /**
     * 16x16 array used for squashed block materials
     * \ingroup grp_maps
     */
    typedef int16_t t_blockmaterials [16][16];
    /**
     * 16x16 array of designation flags
     * \ingroup grp_maps
     */
    typedef DFHack::t_designation designations40d [16][16];
    /**
     * 16x16 array of occupancy flags
     * \ingroup grp_maps
     */
    typedef DFHack::t_occupancy occupancies40d [16][16];
    /**
     * array of 16 biome indexes valid for the block
     * \ingroup grp_maps
     */
    typedef uint8_t biome_indices40d [16];
    /**
     * 16x16 array of temperatures
     * \ingroup grp_maps
     */
    typedef uint16_t t_temperatures [16][16];
    /**
     * structure for holding whole blocks
     * \ingroup grp_maps
     */
    typedef struct
    {
        /// type of the tiles
        tiletypes40d tiletypes;
        /// flags determining the state of the tiles
        designations40d designation;
        /// flags determining what's on the tiles
        occupancies40d occupancy;
        /// values used for geology/biome assignment
        biome_indices40d biome_indices;
        /// the address where the block came from
        uint32_t origin;
        t_blockflags blockflags;
        /// index into the global feature vector
        int16_t global_feature;
        /// index into the local feature vector... complicated
        int16_t local_feature;
    } mapblock40d;

    class DFContextShared;
    /**
     * The Maps module
     * \ingroup grp_modules
     * \ingroup grp_maps
     */
    class DFHACK_EXPORT Maps : public Module
    {
        public:
        
        Maps(DFHack::DFContextShared * d);
        ~Maps();
        bool Start();
        bool Finish();
        
        // read region surroundings, get their vectors of geolayers so we can do translation (or just hand the translation table to the client)
        // returns an array of 9 vectors of indices into stone matgloss
        /**
            Method for reading the geological surrounding of the currently loaded region.
            assign is a reference to an array of nine vectors of unsigned words that are to be filled with the data
            array is indexed by the BiomeOffset enum

            I omitted resolving the layer matgloss in this API, because it would
            introduce overhead by calling some method for each tile. You have to do it
            yourself.
            
            First get the stuff from ReadGeology and then for each block get the RegionOffsets.
            For each tile get the real region from RegionOffsets and cross-reference it with
            the geology stuff (region -- array of vectors, depth -- vector).
            I'm thinking about turning that Geology stuff into a two-dimensional array
            with static size.

            this is the algorithm for applying matgloss:
            @code
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
            @endcode
         */
        bool ReadGeology( std::vector < std::vector <uint16_t> >& assign );
        std::vector <t_feature> global_features;
        // map between feature address and the read object
        std::map <uint32_t, t_feature> local_feature_store;
        // map between mangled coords and pointer to feature

        bool ReadGlobalFeatures( std::vector <t_feature> & features);
        bool ReadLocalFeatures( std::map <DFCoord, std::vector<t_feature *> > & local_features );
        /*
         * BLOCK DATA
         */

        /// get size of the map in tiles
        void getSize(uint32_t& x, uint32_t& y, uint32_t& z);

        /**
         * Return false/0 on failure, buffer allocated by client app, 256 items long
         */
        bool isValidBlock(uint32_t blockx, uint32_t blocky, uint32_t blockz);

        /**
         * Get the address of a block or 0 if block is not valid
         */
        uint32_t getBlockPtr (uint32_t blockx, uint32_t blocky, uint32_t blockz);

        /// read the whole map block at block coords (see DFTypes.h for the block structure)
        bool ReadBlock40d(uint32_t blockx, uint32_t blocky, uint32_t blockz, mapblock40d * buffer);

        /// read/write block tile types
        bool ReadTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, tiletypes40d *buffer);
        bool WriteTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, tiletypes40d *buffer);

        /// read/write block designations
        bool ReadDesignations(uint32_t blockx, uint32_t blocky, uint32_t blockz, designations40d *buffer);
        bool WriteDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, designations40d *buffer);

        /// read/write temperatures
        bool ReadTemperatures(uint32_t blockx, uint32_t blocky, uint32_t blockz, t_temperatures *temp1, t_temperatures *temp2);
        bool WriteTemperatures (uint32_t blockx, uint32_t blocky, uint32_t blockz, t_temperatures *temp1, t_temperatures *temp2);

        /// read/write block occupancies
        bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);
        bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);

        /// read/write the block dirty bit - this is used to mark a map block so that DF scans it for designated jobs like digging
        bool ReadDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool &dirtybit);
        bool WriteDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool dirtybit);

        /// read/write the block flags
        bool ReadBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz, t_blockflags &blockflags);
        bool WriteBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz, t_blockflags blockflags);
        /// read/write features
        bool ReadFeatures(uint32_t blockx, uint32_t blocky, uint32_t blockz, int16_t & local, int16_t & global);
        bool WriteLocalFeature(uint32_t blockx, uint32_t blocky, uint32_t blockz, int16_t local = -1);
        bool WriteGlobalFeature(uint32_t blockx, uint32_t blocky, uint32_t blockz, int16_t local = -1);

        /// read region offsets of a block - used for determining layer stone matgloss
        bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz,
                               biome_indices40d *buffer);

        /// block event reading - mineral veins, what's under ice, blood smears and mud
        bool ReadVeins(uint32_t x, uint32_t y, uint32_t z,
                       std::vector<t_vein>* veins,
                       std::vector<t_frozenliquidvein>* ices = 0,
                       std::vector<t_spattervein>* splatter = 0,
                       std::vector<t_grassvein>* grass = 0,
                       std::vector<t_worldconstruction>* constructions = 0
                      );
        /// read all plants in this block
        bool ReadVegetation(uint32_t x, uint32_t y, uint32_t z, std::vector<t_tree>* plants);
        private:
        struct Private;
        Private *d;
    };
}
#endif
