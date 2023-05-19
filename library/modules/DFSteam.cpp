#include "Internal.h"

#include "modules/DFSteam.h"

#include "Debug.h"
#include "PluginManager.h"

namespace DFHack
{
DBG_DECLARE(core, dfsteam, DebugCategory::LINFO);
}

using namespace DFHack;

static const int DFHACK_STEAM_APPID = 2346660;

static bool g_steam_initialized = false;
static DFLibrary* g_steam_handle = nullptr;
static const std::vector<std::string> STEAM_LIBS {
    "steam_api64.dll",
        "steam_api", // TODO: validate this on OSX
        "libsteam_api.so" // TODO: validate this on Linux
};

bool (*g_SteamAPI_Init)() = nullptr;
void (*g_SteamAPI_Shutdown)() = nullptr;
int (*g_SteamAPI_GetHSteamUser)() = nullptr;
bool (*g_SteamAPI_RestartAppIfNecessary)(uint32_t unOwnAppID) = nullptr;
void* (*g_SteamInternal_FindOrCreateUserInterface)(int, const char*) = nullptr;
bool (*g_SteamAPI_ISteamApps_BIsAppInstalled)(void *iSteamApps, uint32_t appID) = nullptr;


static void bind_all(color_ostream& out, DFLibrary* handle) {
#define bind(name) \
        if (!handle) { \
            g_##name = nullptr; \
        } else { \
            g_##name = (decltype(g_##name))LookupPlugin(handle, #name); \
            if (!g_##name) { \
                WARN(dfsteam, out).print("steam library function not found: " #name "\n"); \
            } \
        }

    bind(SteamAPI_Init);
    bind(SteamAPI_Shutdown);
    bind(SteamAPI_GetHSteamUser);
    bind(SteamInternal_FindOrCreateUserInterface);
    bind(SteamAPI_RestartAppIfNecessary);
    bind(SteamInternal_FindOrCreateUserInterface);
    bind(SteamAPI_ISteamApps_BIsAppInstalled);
#undef bind
}

bool DFSteam::init(color_ostream& out) {
    for (auto& lib_str : STEAM_LIBS) {
        if ((g_steam_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_steam_handle) {
        DEBUG(dfsteam, out).print("steam library not found; stubbing calls\n");
        return false;
    }

    bind_all(out, g_steam_handle);

    if (!g_SteamAPI_Init || !g_SteamAPI_Shutdown || !g_SteamAPI_Init()) {
        DEBUG(dfsteam, out).print("steam detected but cannot be initialized\n");
        return false;
    }

    DEBUG(dfsteam, out).print("steam library linked\n");
    g_steam_initialized = true;
    return true;
}

void DFSteam::cleanup(color_ostream& out) {
    if (!g_steam_handle)
        return;

    if (g_SteamAPI_Shutdown)
        g_SteamAPI_Shutdown();

    ClosePlugin(g_steam_handle);
    g_steam_handle = nullptr;

    bind_all(out, nullptr);
    g_steam_initialized = false;
}

#ifdef WIN32
#include <windows.h>
static bool is_running_on_wine() {
    typedef const char* (CDECL wine_get_version)(void);
    static wine_get_version* pwine_get_version;
    HMODULE hntdll = GetModuleHandle("ntdll.dll");
    if(!hntdll)
        return false;

    pwine_get_version = (wine_get_version*) GetProcAddress(hntdll, "wine_get_version");
    return !!pwine_get_version;
}

static bool launchDFHack(color_ostream& out) {
    if (is_running_on_wine()) {
        DEBUG(dfsteam, out).print("not attempting to re-launch DFHack on wine\n");
        return false;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // note that the enviornment must be explicitly zeroed out and not NULL,
    // otherwise the launched process will inherit this process's environment,
    // and the Steam API in the launchdf process will think it is in DF's context.
    BOOL res = CreateProcessW(L"hack/launchdf.exe",
            NULL, NULL, NULL, FALSE, 0, "\0", NULL, &si, &pi);

    return !!res;
}
#else
static bool launchDFHack(color_ostream& out) {
    // TODO once we have a non-Windows build to work with
    return false;
}
#endif

void DFSteam::launchSteamDFHackIfNecessary(color_ostream& out) {
    if (!g_steam_initialized ||
            !g_SteamAPI_GetHSteamUser ||
            !g_SteamInternal_FindOrCreateUserInterface ||
            !g_SteamAPI_ISteamApps_BIsAppInstalled) {
        DEBUG(dfsteam, out).print("required Steam API calls are unavailable\n");
        return;
    }

    if (strncmp(getenv("SteamClientLaunch"), "1", 2)) {
        DEBUG(dfsteam, out).print("not launched from Steam client\n");
        return;
    }

    void* iSteamApps = g_SteamInternal_FindOrCreateUserInterface(g_SteamAPI_GetHSteamUser(), "STEAMAPPS_INTERFACE_VERSION008");
    if (!iSteamApps) {
        DEBUG(dfsteam, out).print("cannot obtain iSteamApps interface\n");
        return;
    }

    bool isDFHackInstalled = g_SteamAPI_ISteamApps_BIsAppInstalled(iSteamApps, DFHACK_STEAM_APPID);
    if (!isDFHackInstalled) {
        DEBUG(dfsteam, out).print("player has not installed DFHack through Steam\n");
        return;
    }

    bool ret = launchDFHack(out);
    DEBUG(dfsteam, out).print("launching DFHack via Steam: %s\n", ret ? "successful" : "unsuccessful");
}
