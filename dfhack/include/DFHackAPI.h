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

    class APIPrivate;
    class memory_info;
    class Process;
    
    // modules
    class Maps;
    class Creatures;
    class Position;
    class Gui;
    class Materials;
    class Translation;
    class Vegetation;
    class Buildings;
    
    class DFHACK_EXPORT API
    {
        APIPrivate * const d;
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
        
        memory_info *getMemoryInfo();
        Process * getProcess();
        DFWindow * getWindow();
        
        /// read/write size bytes of raw data at offset. DANGEROUS, CAN SEGFAULT DF!
        void ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target);
        void WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source);
        
        Creatures * getCreatures();
        Maps * getMaps();
        Gui * getGui();
        Position * getPosition();
        Materials * getMaterials();
        Translation * getTranslation();
        Vegetation * getVegetation();
        Buildings * getBuildings();
        
        /*
         * Constructions (costructed walls, floors, ramps, etc...)
         */
        /*
        /// start reading constructions. numconstructions is an output - total constructions present
        bool InitReadConstructions( uint32_t & numconstructions );
        /// read a construiction at index
        bool ReadConstruction(const int32_t index, t_construction & construction);
        /// cleanup after reading constructions
        void FinishReadConstructions();
*/
        /*
         * Effects like mist, dragonfire or dust
         */
        /*
        bool InitReadEffects ( uint32_t & numeffects );
        bool ReadEffect(const uint32_t index, t_effect_df40d & effect);
        bool WriteEffect(const uint32_t index, const t_effect_df40d & effect);
        void FinishReadEffects();
        */
        /*
         * Trees and shrubs
         */
        /*
        bool InitReadVegetation( uint32_t & numplants );
        bool ReadVegetation(const int32_t index, t_tree_desc & shrubbery);
        void FinishReadVegetation();
        */

        /*
         * Notes placed by the player
         */
        /*
        /// start reading notes. numnotes is an output - total notes present
        bool InitReadNotes( uint32_t & numnotes );
        /// read note from the note vector at index
        bool ReadNote(const int32_t index, t_note & note);
        /// free the note vector
        void FinishReadNotes();
        */
        /*
         * Settlements
         */
        /*
        bool InitReadSettlements( uint32_t & numsettlements );
        bool ReadSettlement(const int32_t index, t_settlement & settlement);
        bool ReadCurrentSettlement(t_settlement & settlement);
        void FinishReadSettlements();
        */
        /*
         * Hotkeys (DF's zoom locations)
         */
        /*
        bool InitReadHotkeys( );
        bool ReadHotkeys(t_hotkey hotkeys[]);
        */
        
        /*
         * Item reading
         */
        /*
        bool InitReadItems(uint32_t & numitems);
        bool getItemIndexesInBox(std::vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2);
        bool ReadItem(const uint32_t index, t_item & item);
        void FinishReadItems();
        */
        /*
         * Get the other API parts for raw access
         */
        
        /*
            // FIXME: BAD!
            bool ReadAllMatgloss(vector< vector< string > > & all);
        */
        //bool ReadItemTypes(std::vector< std::vector< t_itemType > > & itemTypes);
    };
} // namespace DFHack
#endif // SIMPLEAPI_H_INCLUDED
