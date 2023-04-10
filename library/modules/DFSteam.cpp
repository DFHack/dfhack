#include "Internal.h"

#include "modules/DFSteam.h"

#include "Debug.h"
#include "PluginManager.h"

namespace DFHack
{
DBG_DECLARE(core, dfsteam, DebugCategory::LINFO);
}

using namespace DFHack;

static DFLibrary* g_steam_handle = nullptr;
static const std::vector<std::string> STEAM_LIBS {
    "steam_api64.dll",
        "steam_api", // TODO: validate this on OSX
        "libsteam_api.so" // TODO: validate this on Linux
};

bool (*g_SteamAPI_Init)() = nullptr;
bool (*g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck)() = nullptr;

bool DFSteam::init(color_ostream& out) {
    for (auto& lib_str : STEAM_LIBS) {
        if ((g_steam_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_steam_handle) {
        DEBUG(dfsteam, out).print("steam library not found; stubbing calls\n");
        return false;
    }

#define bind(handle, name) \
        g_##name = (decltype(g_##name))LookupPlugin(handle, #name); \
        if (!g_##name) { \
            WARN(dfsteam, out).print("steam library function not found: " #name "\n"); \
        }

    bind(g_steam_handle, SteamAPI_Init);

    // TODO: can we remove this initialization of the Steam API once we move to dfhooks?
    if (!g_SteamAPI_Init || !g_SteamAPI_Init())
        return false;

    bind(g_steam_handle, SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck);
#undef bind

    DEBUG(dfsteam, out).print("steam library linked\n");
    return true;
}

void DFSteam::cleanup() {
    if (!g_steam_handle)
        return;

    ClosePlugin(g_steam_handle);
    g_steam_handle = nullptr;
}

bool DFSteam::DFIsSteamRunningOnSteamDeck() {
    if (!g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck)
        return false;
    return g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck();
}
