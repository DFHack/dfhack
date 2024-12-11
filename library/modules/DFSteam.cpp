#include "Internal.h"

#include "modules/DFSteam.h"

#include "Debug.h"
#include "PluginManager.h"

#include "df/gamest.h"

namespace DFHack
{
    DBG_DECLARE(core, dfsteam, DebugCategory::LINFO);
}

using namespace DFHack;

static const int DFHACK_STEAM_APPID = 2346660;

static bool g_steam_initialized = false;
static DFLibrary* g_steam_handle = nullptr;
static const std::vector<std::string> STEAM_LIBS {
#ifdef WIN32
    "steam_api64.dll"
#elif defined(_DARWIN)
    "steam_api"  // TODO: validate this on OSX
#else
    "libsteam_api.so"
#endif
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
    char *steam_client_launch = getenv("SteamClientLaunch");
    if (!steam_client_launch || strncmp(steam_client_launch, "1", 2) != 0) {
        DEBUG(dfsteam, out).print("not launched from Steam client; not initializing steam\n");
        return false;
    }

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

#include <process.h>
#include <windows.h>
#include <TlHelp32.h>

static bool is_running_on_wine() {
    typedef const char* (CDECL wine_get_version)(void);
    static wine_get_version* pwine_get_version;
    HMODULE hntdll = GetModuleHandle("ntdll.dll");
    if(!hntdll)
        return false;

    pwine_get_version = (wine_get_version*) GetProcAddress(hntdll, "wine_get_version");
    return !!pwine_get_version;
}

static DWORD findProcess(LPCWSTR name) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return -1;
    }

    do {
        std::wstring executableName(entry.szExeFile);
        if (executableName == name) {
            CloseHandle(snapshot);
            return entry.th32ProcessID;
        }
    }
    while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return -1;
}

static bool launchDFHack(color_ostream& out) {
    if (is_running_on_wine()) {
        DEBUG(dfsteam, out).print("not attempting to re-launch DFHack on wine\n");
        return false;
    }

    if (findProcess(L"launchdf.exe") != -1) {
        DEBUG(dfsteam, out).print("launchdf.exe already running\n");
        return true;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    static LPCWSTR procname = L"hack/launchdf.exe";
    static const char * env = "\0";

    // note that the environment must be explicitly zeroed out and not NULL,
    // otherwise the launched process will inherit this process's environment,
    // and the Steam API in the launchdf process will think it is in DF's context.
    BOOL res = CreateProcessW(procname,
            NULL, NULL, NULL, FALSE, 0, (LPVOID)env, NULL, &si, &pi);

    return !!res;
}

#else

#include <unistd.h>

static bool findProcess(color_ostream& out, std::string name, pid_t &pid) {
    char buf[512];
    std::string command = "pidof -s ";
    command += name;
    FILE *cmd_pipe = popen(command.c_str(), "r");
    if (!cmd_pipe) {
        WARN(dfsteam, out).print("failed to exec '%s' (error: %d)\n",
            command.c_str(), errno);
        return false;
    }

    bool found = fgets(buf, 512, cmd_pipe) != NULL;
    pclose(cmd_pipe);

    pid = 0;
    if (found)
        pid = strtoul(buf, NULL, 10);
    return true;
}

static bool launchDFHack(color_ostream& out) {
    pid_t pid;
    if (!findProcess(out, "launchdf", pid))
        return false;
    if (pid != 0) {
        DEBUG(dfsteam, out).print("launchdf already running\n");
        return true;
    }

    pid = fork();
    if (pid == -1) {
        WARN(dfsteam, out).print("failed to fork (error: %d)\n", errno);
        return false;
    } else if (pid == 0) {
        // child process
        static const char * command = "hack/launchdf";
        unsetenv("SteamAppId");
        execl(command, command, NULL);
        _exit(EXIT_FAILURE);
    }

    // parent process
    return true;
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

    const std::string & cmdline = df::global::game->command_line.original;
    if (cmdline.find("--nosteam-dfhack") != std::string::npos) {
        WARN(dfsteam, out).print("--nosteam-dfhack specified on commandline; not launching DFHack coprocess\n");
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
