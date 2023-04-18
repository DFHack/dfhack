#include <process.h>
#include <windows.h>
#include "steam_api.h"

const uint32 DFHACK_STEAM_APPID = 2346660;

static BOOL is_running_on_wine() {
    typedef const char* (CDECL wine_get_version)(void);
    static wine_get_version* pwine_get_version;
    HMODULE hntdll = GetModuleHandle("ntdll.dll");
    if(!hntdll)
        return FALSE;

    pwine_get_version = (wine_get_version*) GetProcAddress(hntdll, "wine_get_version");
    return !!pwine_get_version;
}

static LPCWSTR launch_via_steam_posix() {
    const char* argv[] = { "/bin/sh", "-c", "\"steam -applaunch 975370\"", NULL };

    // does not return on success
    _execv(argv[0], argv);

    return L"Could not launch Dwarf Fortress";
}

static LPCWSTR launch_via_steam_windows() {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR steamPath[1024];
    DWORD datasize = 1024;

    LONG retCode = RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam",
            L"SteamExe", RRF_RT_REG_SZ, NULL, &steamPath, &datasize);

    if (retCode != ERROR_SUCCESS)
        return L"Could not find Steam client executable";

    WCHAR commandLine[1024] = L"steam.exe -applaunch 975370";

    if (CreateProcessW(steamPath, commandLine,
            NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == 0)
        return L"Could not launch Dwarf Fortress";

    return NULL;
}

// this method doesn't properly attribute Steam playtime metrics to DF,
// but that's better than not having DF start at all.
static BOOL launch_direct() {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    return CreateProcessW(L"Dwarf Fortress.exe",
            NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hpi, PWSTR cmd, int ns) {

    if (SteamAPI_RestartAppIfNecessary(DFHACK_STEAM_APPID)) // Replace with your App ID
    {
        return 1;
    }

        if (!SteamAPI_Init())
        {
            printf("Fatal Error - Steam must be running to play this game (SteamAPI_Init() failed).\n");
            return 1;
        }

        return 0;

    LPCWSTR err =  is_running_on_wine() ? launch_via_steam_posix() : launch_via_steam_windows();

    if (err && !launch_direct()) {
        MessageBoxW(NULL, err, NULL, 0);
        exit(1);
    }

    exit(0);
}
