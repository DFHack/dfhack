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
#include "ModuleFactory.h"
#include "Core.h"
#include "modules/Notes.h"
using namespace DFHack;

Module* DFHack::createNotes()
{
    return new Notes();
}

Notes::Notes()
{
    Core & c = Core::getInstance();

    notes = NULL;
    VersionInfo * mem = c.vinfo;
    try
    {
        OffsetGroup * OG_Notes = mem->getGroup("Notes");

        notes = (std::vector<t_note*>*) OG_Notes->getAddress("vector");
    }
    catch(DFHack::Error::AllMemdef &e)
    {
        notes = NULL;
        cerr << "Notes not available... " << e.what() << endl;
    }
}
