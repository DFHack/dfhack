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
#include "DFHackAPI.h"
#include "DFError.h"

#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/APIPrivate.h"

#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Position.h"
#include "modules/Gui.h"
#include "modules/Creatures.h"

using namespace DFHack;

API::API (const string path_to_xml)
        : d (new APIPrivate())
{
    d->xml = QUOT (MEMXML_DATA_PATH);
    d->xml += "/";
    d->xml += path_to_xml;
    d->pm = NULL;
    d->shm_start = 0;
}

API::~API()
{
    // FIXME: call all finishread* methods here
    Detach();
    delete d;
}

bool API::Attach()
{
    // detach all processes, destroy manager
    if (d->pm == 0)
    {
        d->pm = new ProcessEnumerator (d->xml); // FIXME: handle bad XML better
    }
    else
    {
        d->pm->purge();
    }

    // find a process (ProcessManager can find multiple when used properly)
    if (!d->pm->findProcessess())
    {
        throw Error::NoProcess();
        //cerr << "couldn't find a suitable process" << endl;
        //return false;
    }
    d->p = (*d->pm) [0];
    if (!d->p->attach())
    {
        throw Error::CantAttach();
        //cerr << "couldn't attach to process" << endl;
        //return false; // couldn't attach to process, no go
    }
    d->shm_start = d->p->getSHMStart();
    d->offset_descriptor = d->p->getDescriptor();
    // process is attached, everything went just fine... hopefully
    return true;
}


bool API::Detach()
{
    if(!d->p)
        return false;
    if (!d->p->detach())
    {
        return false;
    }
    if (d->pm != NULL)
    {
        delete d->pm;
    }
    d->pm = NULL;
    d->p = NULL;
    d->shm_start = 0;
    d->offset_descriptor = NULL;
    return true;
}

bool API::isAttached()
{
    return d->p != NULL;
}

bool API::Suspend()
{
    return d->p->suspend();
}
bool API::AsyncSuspend()
{
    return d->p->asyncSuspend();
}

bool API::Resume()
{
    return d->p->resume();
}
bool API::ForceResume()
{
    return d->p->forceresume();
}
bool API::isSuspended()
{
    return d->p->isSuspended();
}

void API::ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    g_pProcess->read (offset, size, target);
}

void API::WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source)
{
    g_pProcess->write (offset, size, source);
}

memory_info *API::getMemoryInfo()
{
    return d->offset_descriptor;
}
Process * API::getProcess()
{
    return d->p;
}

DFWindow * API::getWindow()
{
    return d->p->getWindow();
}

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/
Creatures * API::getCreatures()
{
    if(!d->creatures)
        d->creatures = new Creatures(d);
    return d->creatures;
}

Maps * API::getMaps()
{
    if(!d->maps)
        d->maps = new Maps(d);
    return d->maps;
}

Gui * API::getGui()
{
    if(!d->gui)
        d->gui = new Gui(d);
    return d->gui;
}

Position * API::getPosition()
{
    if(!d->position)
        d->position = new Position(d);
    return d->position;
}

Materials * API::getMaterials()
{
    if(!d->materials)
        d->materials = new Materials(d);
    return d->materials;
}

/*
// returns number of buildings, expects v_buildingtypes that will later map t_building.type to its name
bool API::InitReadBuildings ( uint32_t& numbuildings )
{
    int buildings = 0;
    try
    {
        buildings = d->offset_descriptor->getAddress ("buildings");
    }
    catch(Error::MissingMemoryDefinition)
    {
        return false;
    }
    d->buildingsInited = true;
    d->p_bld = new DfVector (d->p,buildings, 4);
    numbuildings = d->p_bld->getSize();
    return true;
}


// read one building
bool API::ReadBuilding (const int32_t index, t_building & building)
{
    if(!d->buildingsInited) return false;

    t_building_df40d bld_40d;

    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_bld->at (index);
    //d->p_bld->read(index,(uint8_t *)&temp);

    //read building from memory
    g_pProcess->read (temp, sizeof (t_building_df40d), (uint8_t *) &bld_40d);

    // transform
    int32_t type = -1;
    d->offset_descriptor->resolveObjectToClassID (temp, type);
    building.origin = temp;
    building.vtable = bld_40d.vtable;
    building.x1 = bld_40d.x1;
    building.x2 = bld_40d.x2;
    building.y1 = bld_40d.y1;
    building.y2 = bld_40d.y2;
    building.z = bld_40d.z;
    building.material = bld_40d.material;
    building.type = type;

    return true;
}


void API::FinishReadBuildings()
{
    if(d->p_bld)
    {
        delete d->p_bld;
        d->p_bld = NULL;
    }
    d->buildingsInited = false;
}

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
    d->p_effect = new DfVector (d->p, effects, 4);
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
    uint32_t temp = * (uint32_t *) d->p_effect->at (index);
    //read effect from memory
    g_pProcess->read (temp, sizeof (t_effect_df40d), (uint8_t *) &effect);
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
        uint32_t temp = * (uint32_t *) d->p_effect->at (index);
    // write effect to memory
    g_pProcess->write(temp,sizeof(t_effect_df40d), (uint8_t *) &effect);
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


//TODO: maybe do construction reading differently - this could go slow with many of them.
// returns number of constructions, prepares a vector, returns total number of constructions
bool API::InitReadConstructions(uint32_t & numconstructions)
{
    int constructions = 0;
    try
    {
        constructions = d->offset_descriptor->getAddress ("constructions");
    }
    catch(Error::MissingMemoryDefinition)
    {
        return false;
    }
    d->p_cons = new DfVector (d->p,constructions, 4);
    d->constructionsInited = true;
    numconstructions = d->p_cons->getSize();
    return true;
}


bool API::ReadConstruction (const int32_t index, t_construction & construction)
{
    if(!d->constructionsInited) return false;
    t_construction_df40d c_40d;

    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_cons->at (index);

    //read construction from memory
    g_pProcess->read (temp, sizeof (t_construction_df40d), (uint8_t *) &c_40d);

    // transform
    construction.x = c_40d.x;
    construction.y = c_40d.y;
    construction.z = c_40d.z;
    construction.material = c_40d.material;

    return true;
}


void API::FinishReadConstructions()
{
    if(d->p_cons)
    {
        delete d->p_cons;
        d->p_cons = NULL;
    }
    d->constructionsInited = false;
}


bool API::InitReadVegetation(uint32_t & numplants)
{
    try 
    {
        int vegetation = d->offset_descriptor->getAddress ("vegetation");
        d->tree_offset = d->offset_descriptor->getOffset ("tree_desc_offset");

        d->vegetationInited = true;
        d->p_veg = new DfVector (d->p, vegetation, 4);
        numplants = d->p_veg->getSize();
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->vegetationInited = false;
        numplants = 0;
        throw;
    }
}


bool API::ReadVegetation (const int32_t index, t_tree_desc & shrubbery)
{
    if(!d->vegetationInited)
        return false;
    // read pointer from vector at position
    uint32_t temp = * (uint32_t *) d->p_veg->at (index);
    //read construction from memory
    g_pProcess->read (temp + d->tree_offset, sizeof (t_tree_desc), (uint8_t *) &shrubbery);
    // FIXME: this is completely wrong. type isn't just tree/shrub but also different kinds of trees. stuff that grows around ponds has its own type ID
    if (shrubbery.material.type == 3) shrubbery.material.type = 2;
    return true;
}


void API::FinishReadVegetation()
{
    if(d->p_veg)
    {
        delete d->p_veg;
        d->p_veg = 0;
    }
    d->vegetationInited = false;
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

        d->p_notes = new DfVector (d->p, notes, 4);
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
    uint32_t temp = * (uint32_t *) d->p_notes->at (index);
    note.symbol = g_pProcess->readByte(temp);
    note.foreground = g_pProcess->readWord(temp + d->note_foreground_offset);
    note.background = g_pProcess->readWord(temp + d->note_background_offset);
    d->p->readSTLString (temp + d->note_name_offset, note.name, 128);
    g_pProcess->read (temp + d->note_xyz_offset, 3*sizeof (uint16_t), (uint8_t *) &note.x);
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

        d->p_settlements = new DfVector (d->p, allSettlements, 4);
        d->p_current_settlement = new DfVector(d->p, currentSettlement,4);
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
    uint32_t temp = * (uint32_t *) d->p_settlements->at (index);
    settlement.origin = temp;
    d->readName(settlement.name, temp + d->settlement_name_offset);
    g_pProcess->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    g_pProcess->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
    return true;
}

bool API::ReadCurrentSettlement(t_settlement & settlement)
{
    if(!d->settlementsInited) return false;
    if(!d->p_current_settlement->getSize()) return false;
    
    uint32_t temp = * (uint32_t *) d->p_current_settlement->at(0);
    settlement.origin = temp;
    d->readName(settlement.name, temp + d->settlement_name_offset);
    g_pProcess->read(temp + d->settlement_world_xy_offset, 2 * sizeof(int16_t), (uint8_t *) &settlement.world_x);
    g_pProcess->read(temp + d->settlement_local_xy_offset, 4 * sizeof(int16_t), (uint8_t *) &settlement.local_x1);
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


bool API::InitReadHotkeys( )
{
    try
    {
        memory_info * minfo = d->offset_descriptor;
        d->hotkey_start = minfo->getAddress("hotkey_start");
        d->hotkey_mode_offset = minfo->getOffset ("hotkey_mode");
        d->hotkey_xyz_offset = minfo->getOffset("hotkey_xyz");
        d->hotkey_size = minfo->getHexValue("hotkey_size");

        d->hotkeyInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->hotkeyInited = false;
        throw;
    }
}
bool API::ReadHotkeys(t_hotkey hotkeys[])
{
    if (!d->hotkeyInited) return false;
    uint32_t currHotkey = d->hotkey_start;
    for(uint32_t i = 0 ; i < NUM_HOTKEYS ;i++)
    {
        d->p->readSTLString(currHotkey,hotkeys[i].name,10);
        hotkeys[i].mode = g_pProcess->readWord(currHotkey+d->hotkey_mode_offset);
        g_pProcess->read (currHotkey + d->hotkey_xyz_offset, 3*sizeof (int32_t), (uint8_t *) &hotkeys[i].x);
        currHotkey+=d->hotkey_size;
    }
    return true;
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
        uint32_t temp = *(uint32_t *) d->p_itm->at(i);
        g_pProcess->read(temp+sizeof(uint32_t),5 * sizeof(uint16_t), (uint8_t *) &temp2);
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
bool API::InitReadNameTables(vector<vector<string> > & translations , vector<vector<string> > & foreign_languages) //(map< string, vector<string> > & nameTable)
{
    try
    {
        int genericAddress = d->offset_descriptor->getAddress ("language_vector");
        int transAddress = d->offset_descriptor->getAddress ("translation_vector");
        int word_table_offset = d->offset_descriptor->getOffset ("word_table");
        int sizeof_string = d->offset_descriptor->getHexValue ("sizeof_string");

        DfVector genericVec (d->p, genericAddress, 4);
        DfVector transVec (d->p, transAddress, 4);

        translations.resize(10);
        for (uint32_t i = 0;i < genericVec.getSize();i++)
        {
            uint32_t genericNamePtr = * (uint32_t *) genericVec.at (i);
            for(int i=0; i<10;i++)
            {
                string word = d->p->readSTLString (genericNamePtr + i * sizeof_string);
                translations[i].push_back (word);
            }
        }

        foreign_languages.resize(transVec.getSize());
        for (uint32_t i = 0; i < transVec.getSize();i++)
        {
            uint32_t transPtr = * (uint32_t *) transVec.at (i);
            //string transName = d->p->readSTLString (transPtr);
            DfVector trans_names_vec (d->p, transPtr + word_table_offset, 4);
            for (uint32_t j = 0;j < trans_names_vec.getSize();j++)
            {
                uint32_t transNamePtr = * (uint32_t *) trans_names_vec.at (j);
                string name = d->p->readSTLString (transNamePtr);
                foreign_languages[i].push_back (name);
            }
        }
        d->nameTablesInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->nameTablesInited = false;
        throw;
    }
}

string API::TranslateName(const DFHack::t_name &name,const std::vector< std::vector<std::string> > & translations ,const std::vector< std::vector<std::string> > & foreign_languages, bool inEnglish)
{
    string out;
    assert (d->nameTablesInited);
    map<string, vector<string> >::const_iterator it;

    if(!inEnglish) 
    {
        if(name.words[0] >=0 || name.words[1] >=0)
        {
            if(name.words[0]>=0) out.append(foreign_languages[name.language][name.words[0]]);
            if(name.words[1]>=0) out.append(foreign_languages[name.language][name.words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name.words[5] >=0) 
        {
            string word;
            for(int i=2;i<=5;i++)
                if(name.words[i]>=0) word.append(foreign_languages[name.language][name.words[i]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
        if(name.words[6] >=0) 
        {
            string word;
            word.append(foreign_languages[name.language][name.words[6]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
    } 
    else
    {
        if(name.words[0] >=0 || name.words[1] >=0)
        {
            if(name.words[0]>=0) out.append(translations[name.parts_of_speech[0]+1][name.words[0]]);
            if(name.words[1]>=0) out.append(translations[name.parts_of_speech[1]+1][name.words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name.words[5] >=0) 
        {
            if(out.length() > 0)
                out.append(" the");
            else
                out.append("The");
            string word;
            for(int i=2;i<=5;i++)
            {
                if(name.words[i]>=0)
                {
                    word = translations[name.parts_of_speech[i]+1][name.words[i]];
                    word[0] = toupper(word[0]);
                    out.append(" " + word);
                }
            }
        }
        if(name.words[6] >=0) 
        {
            if(out.length() > 0)
                out.append(" of");
            else
                out.append("Of");
            string word;
            word.append(translations[name.parts_of_speech[6]+1][name.words[6]]);
            word[0] = toupper(word[0]);
            out.append(" " + word);
        }
    }
    return out;
}

void API::FinishReadNameTables()
{
    d->nameTablesInited = false;
}

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

        d->p_itm = new DfVector (d->p, items, 4);
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
    uint32_t temp = * (uint32_t *) d->p_itm->at (index);

    //read building from memory
    g_pProcess->read (temp, sizeof (t_item_df40d), (uint8_t *) &item_40d);

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
    g_pProcess->read (temp + d->item_material_offset, sizeof (t_matglossPair), (uint8_t *) &item.material);
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
        DfVector p_temp (d->p, matgloss_address + i*matgloss_skip,4);
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