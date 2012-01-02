/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#pragma once
/*
* kitchen settings
*/
#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "VersionInfo.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "Core.h"
/**
 * \defgroup grp_kitchen Kitchen settings
 * @ingroup grp_modules
 */

namespace DFHack
{
namespace Kitchen
{
typedef uint8_t t_exclusionType;

const unsigned int seedLimit = 400; // a limit on the limits which can be placed on seeds
const t_itemSubtype organicSubtype = -1; // seems to fixed
const t_exclusionType cookingExclusion = 1; // seems to be fixed
const t_itemType limitType = 0; // used to store limit as an entry in the exclusion list. 0 = BAR
const t_itemSubtype limitSubtype = 0; // used to store limit as an entry in the exclusion list
const t_exclusionType limitExclusion = 4; // used to store limit as an entry in the exclusion list

/**
 * Kitchen exclusions manipulator class. Currently geared towards plants and seeds.
 * @ingroup grp_kitchen
 */
class DFHACK_EXPORT Exclusions
{
public:
    /// ctor
    Exclusions(DFHack::Core& core_);
    /// dtor
    ~Exclusions();

    /// print the exclusion list, with the material index also translated into its token (for organics) - for debug really
    void debug_print() const;

    /// remove this material from the exclusion list if it is in it
    void allowPlantSeedCookery(t_materialIndex materialIndex);

    /// add this material to the exclusion list, if it is not already in it
    void denyPlantSeedCookery(t_materialIndex materialIndex);

    /// fills a map with info from the limit info storage entries in the exclusion list
    void fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap) const;

    /// removes a limit info storage entry from the exclusion list if it's present
    void removeLimit(t_materialIndex materialIndex);

    /// add a limit info storage item to the exclusion list, or alters an existing one
    void setLimit(t_materialIndex materialIndex, unsigned int limit);

    /// clears all limit info storage items from the exclusion list
    void clearLimits();

    /// the size of the exclusions vectors (they are all the same size - if not, there is a problem!)
    std::size_t size() const;
private:
    class Private;
    Private* d;

};

}
}
