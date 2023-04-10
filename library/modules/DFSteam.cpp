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
void (*g_SteamAPI_Shutdown)() = nullptr;
void* (*g_SteamInternal_FindOrCreateUserInterface)(int, const char*) = nullptr;
bool (*g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck)(void*) = nullptr;

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
    bind(g_steam_handle, SteamAPI_Shutdown);

    // TODO: can we remove this initialization of the Steam API once we move to dfhooks?
    if (!g_SteamAPI_Init || !g_SteamAPI_Shutdown || !g_SteamAPI_Init()) {
        DEBUG(dfsteam, out).print("steam detected but cannot be initialized\n");
        return false;
    }

    bind(g_steam_handle, SteamInternal_FindOrCreateUserInterface);
    bind(g_steam_handle, SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck);
#undef bind

    DEBUG(dfsteam, out).print("steam library linked\n");
    return true;
}

void DFSteam::cleanup() {
    if (!g_steam_handle)
        return;

    if (g_SteamAPI_Shutdown)
        g_SteamAPI_Shutdown();

    ClosePlugin(g_steam_handle);
    g_steam_handle = nullptr;
}

bool DFSteam::DFIsSteamRunningOnSteamDeck() {
    if (!g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck)
        return false;

    if (!g_SteamInternal_FindOrCreateUserInterface)
        return false;

    void* SteamUtils = g_SteamInternal_FindOrCreateUserInterface(0, "SteamUtils010");

    return g_SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck(SteamUtils);
}
