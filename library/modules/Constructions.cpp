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

#include "ContextShared.h"

#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include "dfhack/DFTypes.h"
#include "dfhack/modules/Constructions.h"
#include "ModuleFactory.h"

using namespace DFHack;

struct Constructions::Private
{
    uint32_t construction_vector;
    // translation
    DfVector <uint32_t> * p_cons;

    DFContextShared *d;
    Process * owner;
    bool Inited;
    bool Started;
};

Module* DFHack::createConstructions(DFContextShared * d)
{
    return new Constructions(d);
}

Constructions::Constructions(DFContextShared * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
    d->p_cons = 0;
    d->Inited = d->Started = false;
    VersionInfo * mem = d->d->offset_descriptor;
    d->construction_vector = mem->getGroup("Constructions")->getAddress ("vector");
    d->Inited = true;
}

Constructions::~Constructions()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Constructions::Start(uint32_t & numconstructions)
{
    d->p_cons = new DfVector <uint32_t> (d->owner, d->construction_vector);
    numconstructions = d->p_cons->size();
    d->Started = true;
    return true;
}


bool Constructions::Read (const uint32_t index, t_construction & construction)
{
    if(!d->Started) return false;

    // read pointer from vector at position
    uint32_t temp = d->p_cons->at (index);

    //read construction from memory
    d->owner->read (temp, sizeof (t_construction), (uint8_t *) &construction);

    // transform
    construction.origin = temp;
    return true;
}

bool Constructions::Finish()
{
    if(d->p_cons)
    {
        delete d->p_cons;
        d->p_cons = NULL;
    }
    d->Started = false;
    return true;
}

