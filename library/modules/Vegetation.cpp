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
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include "dfhack/Types.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Translation.h"
#include "ModuleFactory.h"
using namespace DFHack;

Module* DFHack::createVegetation(DFContextShared * d)
{
    return new Vegetation(d);
}

Vegetation::Vegetation(DFContextShared * d_)
{
    try
    {
        OffsetGroup * OG_Veg = d_->offset_descriptor->getGroup("Vegetation");
        all_plants = (vector<df_plant *> *) OG_Veg->getAddress ("vector");
    }
    catch(exception &)
    {
        all_plants = 0;
    }
}

Vegetation::~Vegetation()
{
}


