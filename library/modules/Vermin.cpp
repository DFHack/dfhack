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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "MemAccess.h"
#include "modules/Vermin.h"
#include "ModuleFactory.h"
#include "Core.h"
using namespace DFHack;

struct Vermin::Private
{
    void * spawn_points_vector;
    uint32_t race_offset;
    uint32_t type_offset;
    uint32_t position_offset;
    uint32_t in_use_offset;
    uint32_t unknown_offset;
    uint32_t countdown_offset;
    Process * owner;
    bool Inited;
    bool Started;
};

Module* DFHack::createVermin()
{
    return new Vermin();
}

#include <stdio.h>

Vermin::Vermin()
{
    Core & c = Core::getInstance();

    d = new Private;
    d->owner = c.p;
    d->Inited = d->Started = false;
    VersionInfo * mem = c.vinfo;
    OffsetGroup * OG_vermin = mem->getGroup("Vermin");
    OffsetGroup * OG_spawn  = OG_vermin->getGroup("Spawn Points");
    d->Inited = true;
    try
    {
        d->spawn_points_vector = OG_spawn->getAddress("vector");

        d->race_offset      = OG_spawn->getOffset("race");
        d->type_offset      = OG_spawn->getOffset("type");
        d->position_offset  = OG_spawn->getOffset("position");
        d->in_use_offset    = OG_spawn->getOffset("in_use");
        d->unknown_offset   = OG_spawn->getOffset("unknown");
        d->countdown_offset = OG_spawn->getOffset("countdown");
    }
    catch(DFHack::Error::AllMemdef &e)
    {
        cerr << "Vermin not available... " << e.what() << endl;
        d->Inited = false;
    }
}

bool Vermin::Finish()
{
    return true;
}

Vermin::~Vermin()
{
    delete d;
}

// NOTE: caller must call delete on result when done.
SpawnPoints* Vermin::getSpawnPoints()
{
    if (!d->Inited)
    {
        cerr << "Couldn't get spawn points: Vermin module not inited" << endl;
        return NULL;
    }
    return new SpawnPoints(this);
}

SpawnPoints::SpawnPoints(Vermin* v_)
{
    v    = v_;
    p_sp = NULL;

    if (!v->d->Inited)
    {
        cerr << "Couldn't get spawn points: Vermin module not inited" << endl;
        return;
    }
    p_sp = (vector <void*>*) (v->d->spawn_points_vector);
}

SpawnPoints::~SpawnPoints()
{
}

size_t SpawnPoints::size()
{
    if (!isValid())
        return 0;

    return p_sp->size();
}

bool SpawnPoints::Read (const uint32_t index, t_spawnPoint & sp)
{
    if(!isValid())
        return false;

    // read pointer from vector at position
    void * temp = p_sp->at (index);

    sp.origin    = temp;
    sp.race      = v->d->owner->readWord(temp + v->d->race_offset);
    sp.type      = v->d->owner->readWord(temp + v->d->type_offset);
    sp.in_use    = v->d->owner->readByte(temp + v->d->in_use_offset);
    sp.unknown   = v->d->owner->readByte(temp + v->d->unknown_offset);
    sp.countdown = v->d->owner->readDWord(temp + v->d->countdown_offset);

    // Three consecutive 16 bit numbers for x/y/z
    v->d->owner->read(temp + v->d->position_offset, 6, (uint8_t*) &sp.x);

    return true;
}

bool SpawnPoints::Write (const uint32_t index, t_spawnPoint & sp)
{
    if(!isValid())
        return false;

    // read pointer from vector at position
    void * temp = p_sp->at (index);

    v->d->owner->writeWord(temp + v->d->race_offset, sp.race);
    v->d->owner->writeWord(temp + v->d->type_offset, sp.type);
    v->d->owner->writeByte(temp + v->d->in_use_offset, sp.in_use);
    v->d->owner->writeByte(temp + v->d->unknown_offset, sp.unknown);
    v->d->owner->writeDWord(temp + v->d->countdown_offset, sp.countdown);

    // Three consecutive 16 bit numbers for x/y/z
    v->d->owner->write(temp + v->d->position_offset, 6, (uint8_t*) &sp.x);

    return true;
}

bool SpawnPoints::isWildColony(t_spawnPoint & point)
{
    return (point.type == TYPE_WILD_COLONY);
}

bool SpawnPoints::isValid()
{
  return (v != NULL && v->d->Inited && p_sp != NULL);
}
