#ifdef WIN32
#  include <process.h>
#  include <windows.h>
#  include <TlHelp32.h>
#else
#  include <unistd.h>
#  include <sys/wait.h>
#endif

#include "steam_api.h"

#include <string>

const uint32 DFHACK_STEAM_APPID = 2346660;
const uint32 DF_STEAM_APPID = 975370;

#ifdef WIN32

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

static LPCWSTR launch_via_steam() {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR steamPath[1024] = L"";
    DWORD datasize = 1024;

    LONG retCode = RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam",
            L"SteamExe", RRF_RT_REG_SZ, NULL, &steamPath, &datasize);

    if (retCode != ERROR_SUCCESS)
        return L"Could not find Steam client executable";

    WCHAR commandLine[1024] = L"steam.exe -applaunch 975370";

    BOOL res = CreateProcessW(steamPath, commandLine,
        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (res) {
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return NULL;
    } else {
        return L"Could not launch Dwarf Fortress";
    }
}

#else

static const char * launch_via_steam() {
    static const char * command = "steam -applaunch 975370";
    if (system(command) != 0)
        return "failed to launch DF via steam";
    return NULL;
}

#endif

#ifdef WIN32

static LPCWSTR launch_direct() {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL res = CreateProcessW(L"Dwarf Fortress.exe",
            NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (res)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return NULL;
    }

    return L"Could not launch via non-steam fallback method";
}

#else

static const char * launch_direct() {
    static const char * command = "./dfhack";
    if (system(command) != 0)
        return "failed to launch DF via ./dfhack launcher";
    return NULL;
}

#endif

#ifdef WIN32

DWORD findDwarfFortressProcess() {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (!Process32FirstW(snapshot, &entry))
    {
        CloseHandle(snapshot);
        return -1;
    }

    do {
        std::wstring executableName(entry.szExeFile);
        if (executableName == L"Dwarf Fortress.exe")
        {
            CloseHandle(snapshot);
            return entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return -1;
}

bool waitForDF() {
    DWORD df_pid = findDwarfFortressProcess();

    if (df_pid == -1)
        return false;

    HANDLE hDF = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, df_pid);

    // in the future open an IPC connection so that we can proxy SteamAPI calls for the DFSteam module

    // this will eventually need to become a loop with a WaitForMultipleObjects call
    WaitForSingleObject(hDF, INFINITE);

    CloseHandle(hDF);

    return true;
}

#else

static pid_t findDwarfFortressProcess() {
    char buf[512];
    static const char * command = "pidof -s dwarfort";
    FILE *cmd_pipe = popen(command, "r");
    if (!cmd_pipe)
        return -1;

    bool success = fgets(buf, 512, cmd_pipe) != NULL;
    pclose(cmd_pipe);

    if (!success)
        return -1;

    return strtoul(buf, NULL, 10);
}

bool waitForDF() {
    pid_t df_pid = findDwarfFortressProcess();

    if (df_pid <= 0)
        return false;

    waitpid(df_pid, NULL, 0);

    return true;
}

#endif

#ifdef WIN32

bool wrap_launch(LPCWSTR (*launch_fn)()) {
    LPCWSTR err = launch_fn();
    if (err != NULL) {
        MessageBoxW(NULL, err, NULL, 0);
        return false;
    }
    return true;
}

#else

void notify(const char * msg) {
    std::string command = "notify-send --app-name=DFHack \"";
    command += msg;
    command += "\"";
    printf("%s\n", msg);
    int result = system(command.c_str());
    // if this fails; it's ok
    (void)result;
}

bool wrap_launch(const char * (*launch_fn)()) {
    const char * err = launch_fn();
    if (err != NULL) {
        notify(err);
        return false;
    }
    return true;
}

#endif

#ifdef WIN32
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
#else
int main(int argc, char* argv[]) {
#endif

    // initialize steam context
    if (SteamAPI_RestartAppIfNecessary(DFHACK_STEAM_APPID)) {
        exit(0);
    }

    if (waitForDF())
        exit(0);

    if (!SteamAPI_Init()) {
        // could not initialize steam context, attempt fallback launch
        exit(wrap_launch(launch_direct) ? 0 : 1);
    }

#ifdef WIN32
    if (is_running_on_wine()) {
        // attempt launch via steam client
        LPCWSTR err = launch_via_steam_posix();

        if (err != NULL)
            // steam client launch failed, attempt fallback launch
            err = launch_direct();

        if (err != NULL)
        {
            MessageBoxW(NULL, err, NULL, 0);
            exit(1);
        }
        exit(0);
    }
#endif

    // steam detected and not running in wine

    if (!SteamApps()->BIsAppInstalled(DF_STEAM_APPID)) {
        // Steam DF is not installed. Assume DF is installed in same directory as DFHack and do a fallback launch
        exit(wrap_launch(launch_direct) ? 0 : 1);
    }

    // obtain DF app path

    char buf[2048] = "";

    int b1 = SteamApps()->GetAppInstallDir(DFHACK_STEAM_APPID, (char*)&buf, 2048);
    std::string dfhack_install_folder = (b1 != -1) ? std::string(buf) : "";

    int b2 = SteamApps()->GetAppInstallDir(DF_STEAM_APPID, (char*)&buf, 2048);
    std::string df_install_folder = (b2 != -1) ? std::string(buf) : "";


    if (df_install_folder != dfhack_install_folder) {
        // DF and DFHack are not installed in the same library
#ifdef WIN32
        MessageBoxW(NULL, L"DFHack and Dwarf Fortress must be installed in the same Steam library.\nAborting.", NULL, 0);
#else
        notify("DFHack and Dwarf Fortress must be installed in the same Steam library.\nAborting.\n");
#endif
        exit(1);
    }

    if (waitForDF())
        exit(0);

    if (!wrap_launch(launch_via_steam))
        exit(1);

    int counter = 0;
    while (!waitForDF()) {
        if (counter++ > 60) {
#ifdef WIN32
            MessageBoxW(NULL, L"Dwarf Fortress took too long to launch, aborting", NULL, 0);
#else
            notify("Dwarf Fortress took too long to launch, aborting\n");
#endif
            exit(1);
        }
#ifdef WIN32
        Sleep(1000);
#else
        usleep(1000000);
#endif
    }

    exit(0);
}
