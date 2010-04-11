/*******************************************************************************
                                    M A P S
                            Read and write DF's map
*******************************************************************************/
#ifndef CL_MOD_MAPS
#define CL_MOD_MAPS

#include "Export.h"
namespace DFHack
{
    /***************************************************************************
                                    T Y P E S
    ***************************************************************************/

    struct t_vein
    {
        uint32_t vtable;
        int32_t type;
        int16_t assignment[16];
        uint32_t flags;
        uint32_t address_of; // this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
    };
    // stores what tiles should appear when the ice melts
    struct t_frozenliquidvein
    {
        uint32_t vtable;
        int16_t tiles[16][16];
        uint32_t address_of; // this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
    };
    
    struct t_spattervein
    {
        uint32_t vtable;
        uint16_t mat1;
        uint16_t unk1;
        uint32_t mat2;
        uint16_t mat3;
        uint8_t intensity[16][16];
        uint32_t address_of; // this is NOT part of the DF vein, but an address of the vein as seen by DFhack.
    };
    
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
    
    enum e_traffic
    {
        traffic_normal,
        traffic_low,
        traffic_high,
        traffic_restricted
    };

    enum e_designation
    {
        designation_no,
        designation_default, // dig walls, remove stairs and ramps, gather plants, fell trees
        designation_ud_stair, // dig up/down stairs
        designation_channel, // dig a channel
        designation_ramp, // dig ramp out of a wall
        designation_d_stair, // dig a stair down
        designation_u_stair, // dig a stair up
        designation_7 // whatever
    };

    enum e_liquidtype
    {
        liquid_water,
        liquid_magma
    };

    struct naked_designation
    {
        unsigned int flow_size : 3; // how much liquid is here?
        unsigned int pile : 1; // stockpile?
        /*
         * All the different dig designations... needs more info, probably an enum
         */
        e_designation dig : 3;
        unsigned int smooth : 2;
        unsigned int hidden : 1;

        /*
         * This one is rather involved, but necessary to retrieve the base layer matgloss index
         * see http://www.bay12games.com/forum/index.php?topic=608.msg253284#msg253284 for details
         */
        unsigned int geolayer_index :4;
        unsigned int light : 1;
        unsigned int subterranean : 1; // never seen the light of day?
        unsigned int skyview : 1; // sky is visible now, it rains in here when it rains

        /*
         * Probably similar to the geolayer_index. Only with a different set of offsets and different data.
         * we don't use this yet
         */
        unsigned int biome : 4;
        /*
         * 0 = water
         * 1 = magma
         */
        e_liquidtype liquid_type : 1;
        unsigned int water_table : 1; // srsly. wtf?
        unsigned int rained : 1; // does this mean actual rain (as in the blue blocks) or a wet tile?
        e_traffic traffic : 2; // needs enum
        unsigned int flow_forbid : 1; // what?
        unsigned int liquid_static : 1;
        unsigned int moss : 1;// I LOVE MOSS
        unsigned int feature_present : 1; // another wtf... is this required for magma pipes to work?
        unsigned int liquid_character : 2; // those ripples on streams?
    };

    union t_designation
    {
        uint32_t whole;
        naked_designation bits;
    };

    // occupancy flags (rat,dwarf,horse,built wall,not build wall,etc)
    struct naked_occupancy
    {
        unsigned int building : 3;// building type... should be an enum?
        // 7 = door
        unsigned int unit : 1;
        unsigned int unit_grounded : 1;
        unsigned int item : 1;
        // splatter. everyone loves splatter.
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
    };

    struct naked_occupancy_grouped
    {
        unsigned int building : 3;// building type... should be an enum?
        // 7 = door
        unsigned int unit : 1;
        unsigned int unit_grounded : 1;
        unsigned int item : 1;
        // splatter. everyone loves splatter.
        unsigned int splatter : 26;
    };

    union t_occupancy
    {
        uint32_t whole;
        naked_occupancy bits;
        naked_occupancy_grouped unibits;
    };

    // map block flags
    struct naked_blockflags
    {
        unsigned int designated : 1;// designated for jobs (digging and stuff like that)
        unsigned int unk_1 : 1; // possibly related to the designated flag
        // two flags required for liquid flow. no idea why
        unsigned int liquid_1 : 1;
        unsigned int liquid_2 : 1;
        unsigned int unk_2: 28; // rest of the flags is completely unknown
        // there's a possibility that this flags field is shorter than 32 bits
    };

    union t_blockflags
    {
        uint32_t whole;
        naked_blockflags bits;
    };

    typedef int16_t tiletypes40d [16][16];
    typedef DFHack::t_designation designations40d [16][16];
//    typedef DFHack::t_occupancy occupancies40d [16][16];
    typedef uint8_t biome_indices40d [16];

    typedef struct
    {
        tiletypes40d tiletypes;
        designations40d designation;
//        occupancies40d occupancy;
        biome_indices40d biome_indices;
        uint32_t origin; // the address where it came from
        t_blockflags blockflags;
    } mapblock40d;

    /***************************************************************************
                             C L I E N T   M O D U L E
    ***************************************************************************/
    
    class APIPrivate;
    struct t_viewscreen;
    class DFHACK_EXPORT Maps
    {
        public:
        
        Maps(DFHack::APIPrivate * d);
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
        bool ReadGeology( std::vector < std::vector <uint16_t> >& assign );

        /*
         * BLOCK DATA
         */
        /*
        /// allocate and read pointers to map blocks
        bool InitMap();
        /// destroy the mapblock cache
        bool DestroyMap();
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

        /*
        /// read/write block occupancies
        bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);
        bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);
*/
        /// read/write the block dirty bit - this is used to mark a map block so that DF scans it for designated jobs like digging
        bool ReadDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool &dirtybit);
        bool WriteDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool dirtybit);

        /// read/write the block flags
        bool ReadBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz,
                            t_blockflags &blockflags);
        bool WriteBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz,
                             t_blockflags blockflags);

        /// read region offsets of a block - used for determining layer stone matgloss
        bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz,
                               biome_indices40d *buffer);

        /// block event reading - mineral veins, what's under ice, blood smears and mud
        bool ReadVeins(uint32_t x, uint32_t y, uint32_t z,
                       std::vector<t_vein>* veins,
                       std::vector<t_frozenliquidvein>* ices = 0,
                       std::vector<t_spattervein>* splatter = 0);
       
        private:
        struct Private;
        Private *d;
    };
}
#endif