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

#include "dfhack/VersionInfo.h"
#include "dfhack/Types.h"
#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "ModuleFactory.h"
#include "dfhack/Core.h"
#include "dfhack/modules/Notes.h"
using namespace DFHack;

struct Notes::Private
{
    uint32_t notes_vector;
    Process * owner;
    bool Inited;
    bool Started;
};

Module* DFHack::createNotes()
{
    return new Notes();
}

Notes::Notes()
{
    Core & c = Core::getInstance();

    d = new Private;
    d->owner = c.p;
    d->Inited = d->Started = false;
    VersionInfo * mem = c.vinfo;
    d->Inited = true;
    try
    {
        OffsetGroup * OG_Notes = mem->getGroup("Notes");

        d->notes_vector = OG_Notes->getAddress("vector");
    }
    catch(DFHack::Error::AllMemdef &e)
    {
        c.con << "Notes not available... " << e.what() << endl;
        d->Inited = false;
    }
}

Notes::~Notes()
{
    delete d;
}

std::vector<t_note*>* Notes::getNotes()
{
    if (!d->Inited)
    {
        Core & c = Core::getInstance();
        c.con << "Notes not available... " << endl;
        return NULL;
    }

    uint32_t ptr = d->notes_vector;

    if ( *( (uint32_t*) ptr) == 0)
        // Notes vector not set up yet.
        return NULL;

    return (std::vector<t_note*>*) ptr;
}

bool Notes::Finish()
{
    return true;
}
