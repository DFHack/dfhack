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

#include "DataDefs.h"
#include "df/world.h"
#include "df/vermin.h"
using namespace df::enums;
using df::global::world;

uint32_t Vermin::getNumVermin()
{
    return df::vermin::get_vector().size();
}

bool Vermin::Read (const uint32_t index, t_vermin & sp)
{
    df::vermin *verm = df::vermin::find(index);
    if (!verm) return false;

    sp.origin    = verm;
    sp.race      = verm->race;
    sp.caste      = verm->caste;
    sp.visible    = verm->visible;
    sp.countdown = verm->countdown;
    sp.x = verm->pos.x;
    sp.y = verm->pos.y;
    sp.z = verm->pos.z;
    sp.is_colony = verm->flags.bits.is_colony;
    return true;
}

bool Vermin::Write (const uint32_t index, t_vermin & sp)
{
    df::vermin *verm = df::vermin::find(index);
    if (!verm) return false;

    verm->race = sp.race;
    verm->caste = sp.caste;
    verm->visible = sp.visible;
    verm->countdown = sp.countdown;
    verm->pos.x = sp.x;
    verm->pos.y = sp.y;
    verm->pos.z = sp.z;
    verm->flags.bits.is_colony = sp.is_colony;
    return true;
}
