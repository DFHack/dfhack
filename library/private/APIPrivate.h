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

/*
* WARNING: Only include from API modules
*/

#ifndef APIPRIVATE_H_INCLUDED
#define APIPRIVATE_H_INCLUDED

namespace DFHack
{
    class Materials;
    class Gui;
    class World;
    class Position;
    class Maps;
    class Creatures;
    class Items;
    class Translation;
    class Buildings;
    class ProcessEnumerator;
    class Process;
    class Vegetation;
    class Constructions;
    class memory_info;
    struct t_name;
    class DFContextPrivate
    {
    public:
        DFContextPrivate();
        ~DFContextPrivate();
        
        // names, used by a few other modules.
        void readName(t_name & name, uint32_t address);
        // get the name offsets
        bool InitReadNames();
        uint32_t name_firstname_offset;
        uint32_t name_nickname_offset;
        uint32_t name_words_offset;
        bool namesInited;
        
        ProcessEnumerator* pm;
        Process* p;
        char * shm_start;
        memory_info* offset_descriptor;
        string xml;
        
        // Modules
        Creatures * creatures;
        Maps * maps;
        Position * position;
        Gui * gui;
        World * world;
        Materials * materials;
        Items * items;
        Translation * translation;
        Vegetation * vegetation;
        Buildings * buildings;
        Constructions * constructions;

        /*
        uint32_t item_material_offset;

        uint32_t note_foreground_offset;
        uint32_t note_background_offset;
        uint32_t note_name_offset;
        uint32_t note_xyz_offset;

        uint32_t settlement_name_offset;
        uint32_t settlement_world_xy_offset;
        uint32_t settlement_local_xy_offset;
        
        uint32_t dwarf_lang_table_offset;

        DfVector *p_effect;
        DfVector *p_itm;
        DfVector *p_notes;
        DfVector *p_settlements;
        DfVector *p_current_settlement;
        */
    };
}
#endif

