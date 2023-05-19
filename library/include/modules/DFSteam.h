#pragma once

#include "ColorText.h"
#include "Export.h"

namespace DFHack
{

/**
 * The DFSteam module - provides access to Steam functions without actually
 * requiring build-time linkage to Steam
 * \ingroup grp_modules
 * \ingroup grp_dfsdl
 */
namespace DFSteam
{

/**
 * Call this on DFHack init so we can load the function pointers. Returns false on
 * failure.
 */
bool init(DFHack::color_ostream& out);

/**
 * Call this when DFHack is being unloaded.
 */
void cleanup(DFHack::color_ostream& out);

DFHACK_EXPORT void launchSteamDFHackIfNecessary(DFHack::color_ostream& out);

}
}
