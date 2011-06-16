/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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


#pragma once

#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "dfhack/Export.h"
namespace DFHack
{
    class Creatures;
    class Engravings;
    class Maps;
    class Gui;
    class World;
    class Materials;
    class Items;
    class Translation;
    class Vegetation;
    class Buildings;
    class Constructions;
    class VersionInfo;
    class DFContextShared;
    class Process;
    /**
     * This class wraps all the different related objects for a particular Process
     * \ingroup grp_context
     */
    class DFHACK_EXPORT Context
    {
        public:
        Context(Process * p);
        ~Context();

        /// @return true if there's version information for the associated Process
        bool isValid();
        /// attach to the related process. Claims OS debugging resources
        bool Attach();
        /// detach from the related process. Releases OS debugging resources
        bool Detach();
        /// @return true if the process is attached.
        bool isAttached();

        /// stop the tracked process
        bool Suspend();
        /// @return true if the process is stopped
        bool isSuspended();

        /// stop the tracked process, asynchronous
        bool AsyncSuspend();

        /// resume the tracked process
        bool Resume();

        /// forces resume on Windows. This can be a bad thing with multiple tools running!
        bool ForceResume();

        VersionInfo *getMemoryInfo();
        Process* getProcess();

        void ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target);
        void WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source);

        /// get the creatures module
        Creatures * getCreatures();

        /// get the engravings module
        Engravings * getEngravings();

        /// get the maps module
        Maps * getMaps();

        /// get the gui module
        Gui * getGui();

        /// get the world module
        World * getWorld();

        /// get the materials module
        Materials * getMaterials();

        /// get the items module
        Items * getItems();

        /// get the translation module
        Translation * getTranslation();

        /// get the vegetation module
        Vegetation * getVegetation();

        /// get the buildings module
        Buildings * getBuildings();

        /// get the constructions module
        Constructions * getConstructions();

        // DEAD CODE, WAITING TO BE UPDATED TO DF2010
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
    private:
        DFContextShared * d;
    };
}
#endif //CONTEXT_H_INCLUDED

