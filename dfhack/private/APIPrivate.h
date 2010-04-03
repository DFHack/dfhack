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

// we connect to those
#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>

#define SHMCMD(num) ((shm_cmd *)d->shm_start)[num]->pingpong
#define SHMHDR ((shm_core_hdr *)d->shm_start)
#define SHMDATA(type) ((type *)(d->shm_start + SHM_HEADER))

namespace DFHack
{

class APIPrivate
{
public:
    APIPrivate();
    void readName(t_name & name, uint32_t address);
    // get the name offsets
    bool InitReadNames();
    
    #include "../modules/Creatures-data.h"
    #include "../modules/Maps-data.h"
    #include "../modules/Position-data.h"
    #include "../modules/Gui-data.h"
    #include "../modules/Materials-data.h"
    
    uint32_t name_firstname_offset;
    uint32_t name_nickname_offset;
    uint32_t name_words_offset;
    
    ProcessEnumerator* pm;
    Process* p;
    char * shm_start;
    memory_info* offset_descriptor;
    string xml;

    /*
    uint32_t item_material_offset;

    uint32_t note_foreground_offset;
    uint32_t note_background_offset;
    uint32_t note_name_offset;
    uint32_t note_xyz_offset;
    uint32_t hotkey_start;
    uint32_t hotkey_mode_offset;
    uint32_t hotkey_xyz_offset;
    uint32_t hotkey_size;

    uint32_t settlement_name_offset;
    uint32_t settlement_world_xy_offset;
    uint32_t settlement_local_xy_offset;
    
    uint32_t dwarf_lang_table_offset;

    bool constructionsInited;
    bool buildingsInited;
    bool effectsInited;
    bool vegetationInited;
    
    
    bool itemsInited;
    bool notesInited;
    bool namesInited;
    bool hotkeyInited;
    bool settlementsInited;
    bool nameTablesInited;

    uint32_t tree_offset;
    
    DfVector *p_cons;
    DfVector *p_bld;
    DfVector *p_effect;
    DfVector *p_veg;
    DfVector *p_itm;
    DfVector *p_notes;
    DfVector *p_settlements;
    DfVector *p_current_settlement;
    */
};
}
#endif