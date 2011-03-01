// This is just a graveyard of old 40d code. Things in here COULD be turned into modules, but it requires research.

bool API::InitReadEffects ( uint32_t & numeffects )
{
    if(d->effectsInited)
        FinishReadEffects();
    int effects = 0;
    try
    {
        effects = d->offset_descriptor->getAddress ("effects_vector");
    }
    catch(Error::AllMemdef)
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
    catch (Error::AllMemdef&)
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
    catch (Error::AllMemdef&)
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

void API::FinishReadNotes()
{
    if(d->p_notes)
    {
        delete d->p_notes;
        d->p_notes = 0;
    }
    d->notesInited = false;
}
