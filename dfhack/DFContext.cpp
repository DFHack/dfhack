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

#include "DFCommonInternal.h"

#include "DFProcess.h"
#include "DFProcessEnumerator.h"
#include "DFContext.h"
#include "DFContext.h"
#include "DFError.h"

#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/APIPrivate.h"

#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "modules/Position.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Creatures.h"
#include "modules/Translation.h"
#include "modules/Vegetation.h"
#include "modules/Buildings.h"
#include "modules/Constructions.h"

using namespace DFHack;

Context::Context (Process* p) : d (new DFContextPrivate())
{
    d->p = p;
    d->offset_descriptor = p->getDescriptor();
    d->shm_start = 0;
}

Context::~Context()
{
    Detach();
    delete d;
}

bool Context::isValid()
{
    //FIXME: check for error states here
    if(d->p->isIdentified())
        return true;
    return false;
}

bool Context::Attach()
{
    if (!d->p->attach())
    {
        //throw Error::CantAttach();
        return false;
    }
    d->shm_start = d->p->getSHMStart();
    // process is attached, everything went just fine... hopefully
    return true;
}


bool Context::Detach()
{
    if (!d->p->detach())
    {
        return false;
    }
    d->shm_start = 0;
    // invalidate all modules
    if(d->creatures)
    {
        delete d->creatures;
        d->creatures = 0;
    }
    if(d->maps)
    {
        delete d->maps;
        d->maps = 0;
    }
    if(d->gui)
    {
        delete d->gui;
        d->gui = 0;
    }
    if(d->world)
    {
        delete d->world;
        d->world = 0;
    }
    if(d->position)
    {
        delete d->position;
        d->position = 0;
    }
    if(d->materials)
    {
        delete d->materials;
        d->materials = 0;
    }
    if(d->items)
    {
        delete d->items;
        d->items = 0;
    }
    if(d->translation)
    {
        delete d->translation;
        d->translation = 0;
    }
    if(d->vegetation)
    {
        delete d->vegetation;
        d->vegetation = 0;
    }
    if(d->constructions)
    {
        delete d->constructions;
        d->constructions = 0;
    }
    if(d->translation)
    {
        delete d->translation;
        d->translation = 0;
    }
    return true;
}

bool Context::isAttached()
{
    return d->p->isAttached();
}

bool Context::Suspend()
{
    return d->p->suspend();
}
bool Context::AsyncSuspend()
{
    return d->p->asyncSuspend();
}

bool Context::Resume()
{
    return d->p->resume();
}
bool Context::ForceResume()
{
    return d->p->forceresume();
}
bool Context::isSuspended()
{
    return d->p->isSuspended();
}

void Context::ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    d->p->read (offset, size, target);
}

void Context::WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source)
{
    d->p->write (offset, size, source);
}

memory_info *Context::getMemoryInfo()
{
    return d->offset_descriptor;
}
Process * Context::getProcess()
{
    return d->p;
}

DFWindow * Context::getWindow()
{
    return d->p->getWindow();
}

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/
Creatures * Context::getCreatures()
{
    if(!d->creatures)
        d->creatures = new Creatures(d);
    return d->creatures;
}

Maps * Context::getMaps()
{
    if(!d->maps)
        d->maps = new Maps(d);
    return d->maps;
}

Gui * Context::getGui()
{
    if(!d->gui)
        d->gui = new Gui(d);
    return d->gui;
}

World * Context::getWorld()
{
    if(!d->world)
        d->world = new World(d);
    return d->world;
}

Position * Context::getPosition()
{
    if(!d->position)
        d->position = new Position(d);
    return d->position;
}

Materials * Context::getMaterials()
{
    if(!d->materials)
        d->materials = new Materials(d);
    return d->materials;
}

Items * Context::getItems()
{
    if(!d->items)
        d->items = new Items(d);
    return d->items;
}

Translation * Context::getTranslation()
{
    if(!d->translation)
        d->translation = new Translation(d);
    return d->translation;
}

Vegetation * Context::getVegetation()
{
    if(!d->vegetation)
        d->vegetation = new Vegetation(d);
    return d->vegetation;
}

Buildings * Context::getBuildings()
{
    if(!d->buildings)
        d->buildings = new Buildings(d);
    return d->buildings;
}

Constructions * Context::getConstructions()
{
    if(!d->constructions)
        d->constructions = new Constructions(d);
    return d->constructions;
}

/*
// returns number of buildings, expects v_buildingtypes that will later map t_building.type to its name

bool API::InitReadEffects ( uint32_t & numeffects )
{
    if(d->effectsInited)
        FinishReadEffects();
    int effects = 0;
    try
    {
        effects = d->offset_descriptor->getAddress ("effects_vector");
    }
    catch(Error::MissingMemoryDefinition)
    {
        return false;
    }
    d->effectsInited = true;
    d->p_effect = new DfVector (d->p, effects);
    numeffects = d->p_effect->getSize();
    return true;
}

bool API::ReadEffect(const uint32_t index, t_effect_df40d & effect)
{
    if(!d->effectsInited)
        return false;
    if(index >= d->p_effect->getSize())
        return false;
    
    // read pointer from vector at position
    uint32_t temp = d->p_effect->at (index);
    //read effect from memory
    d->p->read (temp, sizeof (t_effect_df40d), (uint8_t *) &effect);
    return true;
}

// use with care!
bool API::WriteEffect(const uint32_t index, const t_effect_df40d & effect)
{
    if(!d->effectsInited)
        return false;
    if(index >= d->p_effect->getSize())
        return false;
    // read pointer from vector at position
        uint32_t temp = d->p_effect->at (index);
    // write effect to memory
    d->p->write(temp,sizeof(t_effect_df40d), (uint8_t *) &effect);
    return true;
}

void API::FinishReadEffects()
{
    if(d->p_effect)
    {
        delete d->p_effect;
        d->p_effect = NULL;
    }
    d->effectsInited = false;
}

*/
/*
bool API::InitReadNotes( uint32_t &numnotes )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        int notes = minfo->getAddress ("notes");
        d->note_foreground_offset = minfo->getOffset ("note_foreground");
        d->note_background_offset = minfo->getOffset ("note_background");
        d->note_name_offset = minfo->getOffset ("note_name");
        d->note_xyz_offset = minfo->getOffset ("note_xyz");

        d->p_notes = new DfVector (d->p, notes);
        d->notesInited = true;
        numnotes =  d->p_notes->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->notesInited = false;
        numnotes = 0;
        throw;
    }
}
bool API::ReadNote (const int32_t index, t_note & note)
{
    if(!d->notesInited) return false;
    // read pointer from vector at position
    uint32_t temp = d->p_notes->at (index);
    note.symbol = d->p->readByte(temp);
    note.foreground = d->p->readWord(temp + d->note_foreground_offset);
    note.background = d->p->readWord(temp + d->note_background_offset);
    d->p->readSTLString (temp + d->note_name_offset, note.name, 128);
    d->p->read (temp + d->note_xyz_offset, 3*sizeof (uint16_t), (uint8_t *) &note.x);
    return true;
}
bool API::InitReadSettlements( uint32_t & numsettlements )
{
    if(!d->InitReadNames()) return false;
    try
    {
        
        memory_info * minfo = d->offset_descriptor;
        int allSettlements = minfo->getAddress ("settlements");
        int currentSettlement = minfo->getAddress("settlement_current");
        d->settlement_name_offset = minfo->getOffset ("settlement_name");
        d->settlement_world_xy_offset = minfo->getOffset ("settlement_world_xy");
        d->settlement_local_xy_offset = minfo->getOffset ("settlement_local_xy");

        d->p_settlements = new DfVector (d->p, allSettlements);
        d->p_current_settlement = new DfVector(d->p, currentSettlement);
        d->settlementsInited = true;
        numsettlements =  d->p_settlements->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->settlementsInited = false;
        numsettlements = 0;
        throw;
    }
}
bool API::ReadSettlement(const int32_t index, t_settlement & settlement)
{
    if(!d->settlementsInited) return false;
    if(!d->p_settlements->getSize()) return false;
    
    // read pointer from vector at position
    uint32_t temp = d->p_settlements->at (index);
    settlement.origin = temp;
    d->readName(settlement.name, temp + d->settlement_name_offset);
    d->p->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    d->p->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
    return true;
}

bool API::ReadCurrentSettlement(t_settlement & settlement)
{
    if(!d->settlementsInited) return false;
    if(!d->p_current_settlement->getSize()) return false;
    
    uint32_t temp = d->p_current_settlement->at(0);
    settlement.origin = temp;
    d->readName(settlement.name, temp + d->settlement_name_offset);
    d->p->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    d->p->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
    return true;
}

void API::FinishReadSettlements()
{
    if(d->p_settlements)
    {
        delete d->p_settlements;
        d->p_settlements = NULL;
    }
    if(d->p_current_settlement)
    {
        delete d->p_current_settlement;
        d->p_current_settlement = NULL;
    }
    d->settlementsInited = false;
}

bool API::getItemIndexesInBox(vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if(!d->itemsInited) return false;
    indexes.clear();
    uint32_t size = d->p_itm->getSize();
    struct temp2{
        uint16_t coords[3];
        uint32_t flags;
    };
    temp2 temp2;
    for(uint32_t i =0;i<size;i++){
        uint32_t temp = d->p_itm->at(i);
        d->p->read(temp+sizeof(uint32_t),5 * sizeof(uint16_t), (uint8_t *) &temp2);
        if(temp2.flags & (1 << 0)){
            if (temp2.coords[0] >= x1 && temp2.coords[0] < x2)
            {
                if (temp2.coords[1] >= y1 && temp2.coords[1] < y2)
                {
                    if (temp2.coords[2] >= z1 && temp2.coords[2] < z2)
                    {
                        indexes.push_back(i);
                    }
                }
            }
        }
    }
    return true;
}
*/
/*
void API::FinishReadNotes()
{
    if(d->p_notes)
    {
        delete d->p_notes;
        d->p_notes = 0;
    }
    d->notesInited = false;
}
*/

/*
bool API::InitReadItems(uint32_t & numitems)
{
    try
    {
        int items = d->offset_descriptor->getAddress ("items");
        d->item_material_offset = d->offset_descriptor->getOffset ("item_materials");

        d->p_itm = new DfVector (d->p, items);
        d->itemsInited = true;
        numitems = d->p_itm->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->itemsInited = false;
        numitems = 0;
        throw;
    }
}
bool API::ReadItem (const uint32_t index, t_item & item)
{
    if (!d->itemsInited) return false;
    
    t_item_df40d item_40d;

    // read pointer from vector at position
    uint32_t temp = d->p_itm->at (index);

    //read building from memory
    d->p->read (temp, sizeof (t_item_df40d), (uint8_t *) &item_40d);

    // transform
    int32_t type = -1;
    d->offset_descriptor->resolveObjectToClassID (temp, type);
    item.origin = temp;
    item.vtable = item_40d.vtable;
    item.x = item_40d.x;
    item.y = item_40d.y;
    item.z = item_40d.z;
    item.type = type;
    item.ID = item_40d.ID;
    item.flags.whole = item_40d.flags;

    //TODO  certain item types (creature based, threads, seeds, bags do not have the first matType byte, instead they have the material index only located at 0x68
    d->p->read (temp + d->item_material_offset, sizeof (t_matglossPair), (uint8_t *) &item.material);
    //for(int i = 0; i < 0xCC; i++){  // used for item research
    //    uint8_t byte = MreadByte(temp+i);
    //    item.bytes.push_back(byte);
    //}
    return true;
}
void API::FinishReadItems()
{
    if(d->p_itm)
    {
        delete d->p_itm;
        d->p_itm = NULL;
    }
    d->itemsInited = false;
}
*/
/*
bool API::ReadItemTypes(vector< vector< t_itemType > > & itemTypes)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress("matgloss");
    int matgloss_skip = minfo->getHexValue("matgloss_skip");
    int item_type_name_offset = minfo->getOffset("item_type_name");
    for(int i = 8;i<20;i++)
    {
        DfVector p_temp (d->p, matgloss_address + i*matgloss_skip);
        vector< t_itemType > typesForVec;
        for(uint32_t j =0; j<p_temp.getSize();j++)
        {
            t_itemType currType;
            uint32_t temp = *(uint32_t *) p_temp[j];
           // Mread(temp+40,sizeof(name),(uint8_t *) name);
            d->p->readSTLString(temp+4,currType.id,128);
            d->p->readSTLString(temp+item_type_name_offset,currType.name,128);
            //stringsForVec.push_back(string(name));
            typesForVec.push_back(currType);
        }
        itemTypes.push_back(typesForVec);
    }
    return true;
}
*/

