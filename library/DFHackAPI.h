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

#include "Tranquility.h"
#include "Export.h"
#include <string>
#include <vector>
#include <map>
#include "integers.h"
#include "DFTileTypes.h"
#include "DFTypes.h"
#include "DFWindow.h"

namespace DFHack
{
    class memory_info;
    class Process;
    class DFHACK_EXPORT API
    {
        class Private;
        Private * const d;
    public:
        API(const std::string path_to_xml);
        ~API();
        /*
         * Basic control over DF's process state
         */
        
        bool Attach();
        bool Detach();
        bool isAttached();
        
        /// stop DF from executing
        bool Suspend();
        bool isSuspended();
        
        /// stop DF from executing, asynchronous, use with polling
        bool AsyncSuspend();
        
        /// resume DF
        bool Resume();
        
        /// forces resume on Windows. This can be a bad thing with multiple DF tools running!
        bool ForceResume();

        /*
         * Query the DF's GUI state
         */
        ///true if paused, false if not
        bool ReadPauseState(); 
        /// read the DF menu view state (stock screen, unit screen, other screens
        bool ReadViewScreen(t_viewscreen &);
        /// read the DF menu state (designation menu ect)
        uint32_t ReadMenuState();

        
        /*
         * Matgloss. next four methods look very similar. I could use two and move the processing one level up...
         * I'll keep it like this, even with the code duplication as it will hopefully get more features and separate data types later.
         * Yay for nebulous plans for a rock survey tool that tracks how much of which metal could be smelted from available resorces
         */
        bool ReadStoneMatgloss(std::vector<t_matgloss> & output);
        bool ReadWoodMatgloss (std::vector<t_matgloss> & output);
        bool ReadMetalMatgloss(std::vector<t_matgloss> & output);
        bool ReadPlantMatgloss(std::vector<t_matgloss> & output);
        bool ReadPlantMatgloss (std::vector<t_matglossPlant> & plants);
        bool ReadCreatureMatgloss(std::vector<t_matgloss> & output);

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
        
        /// read/write block occupancies
        bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);
        bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, occupancies40d *buffer);
        
        /// read/write the block dirty bit - this is used to mark a map block so that DF scans it for designated jobs like digging
        bool ReadDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool &dirtybit);
        bool WriteDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool dirtybit);
        
        /// read/write the block flags
        bool ReadBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz, t_blockflags &blockflags);
        bool WriteBlockFlags(uint32_t blockx, uint32_t blocky, uint32_t blockz, t_blockflags blockflags);
        
        /// read region offsets of a block - used for determining layer stone matgloss
        bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz, biome_indices40d *buffer);
        
        /// read aggregated veins of a block
        bool ReadVeins(uint32_t blockx, uint32_t blocky, uint32_t blockz, std::vector <t_vein> & veins, std::vector <t_frozenliquidvein>& ices);
        
        /*
         * Constructions (costructed walls, floors, ramps, etc...)
         */
        /// start reading constructions. numconstructions is an output - total constructions present
        bool InitReadConstructions( uint32_t & numconstructions );
        /// read a construiction at index
        bool ReadConstruction(const int32_t index, t_construction & construction);
        /// cleanup after reading constructions
        void FinishReadConstructions();

        /*
         * Buildings - also includes zones and stockpiles
         */
        bool InitReadBuildings ( uint32_t & numbuildings );
        bool ReadBuilding(const int32_t index, t_building & building);
        void FinishReadBuildings();
        
        /*
         * Effects like mist, dragonfire or dust
         */
        bool InitReadEffects ( uint32_t & numeffects );
        bool ReadEffect(const uint32_t index, t_effect_df40d & effect);
        bool WriteEffect(const uint32_t index, const t_effect_df40d & effect);
        void FinishReadEffects();
        
        /*
         * Trees and shrubs
         */
        bool InitReadVegetation( uint32_t & numplants );
        bool ReadVegetation(const int32_t index, t_tree_desc & shrubbery);
        void FinishReadVegetation();
        
        /*
         * Creatures
         */
        bool InitReadCreatures( uint32_t & numcreatures );
        /**
         * Read creatures in a box, starting with index. Returns -1 if no more creatures
         * found. Call repeatedly do get all creatures in a specified box (uses tile coords)
         */
        int32_t ReadCreatureInBox(const int32_t index, t_creature & furball,
                                  const uint16_t x1, const uint16_t y1,const uint16_t z1,
                                  const uint16_t x2, const uint16_t y2,const uint16_t z2);
        bool ReadCreature(const int32_t index, t_creature & furball);
        void FinishReadCreatures();
        
        /// read/write size bytes of raw data at offset. DANGEROUS, CAN SEGFAULT DF!
        void ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target);
        void WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source);
        /// write labors of a creature (for Dwarf Therapist)
        void WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);
        
        /*
         * Notes placed by the player
         */
        
        /// start reading notes. numnotes is an output - total notes present
        bool InitReadNotes( uint32_t & numnotes );
        /// read note from the note vector at index
        bool ReadNote(const int32_t index, t_note & note);
        /// free the note vector
        void FinishReadNotes();
        
        /*
         * Settlements
         */
        bool InitReadSettlements( uint32_t & numsettlements );
        bool ReadSettlement(const int32_t index, t_settlement & settlement);
        bool ReadCurrentSettlement(t_settlement & settlement);
        void FinishReadSettlements();

        /*
         * Hotkeys (DF's zoom locations)
         */
        bool InitReadHotkeys( );
        bool ReadHotkeys(t_hotkey hotkeys[]);
        
        /*
         * Cursor, and view coords
         */
        bool InitViewAndCursor();
        bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);
        
        bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);

        /// get the creature vector index of the creature currently under DF' cursor
        bool getCurrentCursorCreature (uint32_t & creature_index);
        
        /*
         * Window size in tiles
         */
        bool InitViewSize();
        bool getWindowSize(int32_t & width, int32_t & height);
        
        /*
         * DF translation tables and name translation
         */
        bool InitReadNameTables (std::vector< std::vector<std::string> > & translations , std::vector< std::vector<std::string> > & foreign_languages);
        void FinishReadNameTables();
        std::string TranslateName(const t_name & name,const std::vector< std::vector<std::string> > & translations ,const std::vector< std::vector<std::string> > & foreign_languages, bool inEnglish=true);
        
        
        /*
         * Item reading
         */
        bool InitReadItems(uint32_t & numitems);
        bool getItemIndexesInBox(std::vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2);
        bool ReadItem(const uint32_t index, t_item & item);
        void FinishReadItems();
        
        /*
         * Get the other API parts for raw access
         */
        memory_info *getMemoryInfo();
        Process * getProcess();
        DFWindow * getWindow();
        /*
            // FIXME: BAD!
            bool ReadAllMatgloss(vector< vector< string > > & all);
        */
        bool ReadItemTypes(std::vector< std::vector< t_itemType > > & itemTypes);
    };
} // namespace DFHack
#endif // SIMPLEAPI_H_INCLUDED
