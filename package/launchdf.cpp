#ifdef WIN32
#  include <process.h>
#  include <windows.h>
#  include <TlHelp32.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif

#include "steam_api.h"

#include <string>
#include <filesystem>
#include <fstream>

#define xstr(s) str(s)
#define str(s) #s

#define DFHACK_STEAM_APPID 2346660
#define DF_STEAM_APPID 975370

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
    const char* argv[] = { "/bin/sh", "-c", "\"steam -applaunch " xstr(DF_STEAM_APPID) "\"", NULL };

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

    WCHAR commandLine[1024] = L"steam.exe -applaunch " xstr(DF_STEAM_APPID);

    BOOL res = CreateProcessW(steamPath, commandLine,
        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (res) {
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return NULL;
    }

    return L"Could not launch Dwarf Fortress";
}

#else

static const char * launch_via_steam() {
    static const char * command = "steam -applaunch " xstr(DF_STEAM_APPID);
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

    if (res) {
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

bool waitForDF(bool nowait) {
    DWORD df_pid = findDwarfFortressProcess();

    if (df_pid == -1)
        return false;

    if (nowait)
        return true;

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

bool waitForDF(bool nowait) {
    pid_t df_pid = findDwarfFortressProcess();

    if (df_pid <= 0)
        return false;

    if (nowait)
        return true;

    char cmd[64];
    snprintf(cmd, 64, "tail --pid=%i -f /dev/null", df_pid);
    return 0 == system(cmd);
}

#endif

constexpr const char* old_filelist[] {
    "hack",
    "stonesense",
#ifdef WIN32
    "binpatch.exe",
    "dfhack-run.exe",
    "allegro-5.2.dll",
    "allegro_color-5.2.dll",
    "allegro_font-5.2.dll",
    "allegro_image-5.2.dll",
    "allegro_primitives-5.2.dll",
    "allegro_ttf-5.2.dll",
    "allegro-5.2.dll",
    "dfhack-client.dll",
    "dfhooks_dfhack.dll",
    "lua53.dll",
    "protobuf-lite.dll"
#else
    "binpatch",
    "dfhack-run",
    "liballegro-5.2.so",
    "liballegro_color-5.2.so",
    "liballegro_font-5.2.so",
    "liballegro_image-5.2.so",
    "liballegro_primitives-5.2.so",
    "liballegro_ttf-5.2.so",
    "liballegro-5.2.so",
    "libdfhack-client.so",
    "libdfhooks_dfhack.so",
    "liblua53.so",
    "libprotobuf-lite.so"
#endif
};

bool check_for_old_install(std::filesystem::path df_path)
{
    for (auto file : old_filelist)
    {
        std::filesystem::path p = df_path / file;
        bool exists = std::filesystem::exists(p);
//        std::wstring message = L"Checking for legacy files:\n" + p.wstring() + L": " + (exists ? L"found" : L"not found");
//        MessageBoxW(NULL, message.c_str(), L"Checking for legacy files", 0);
        if (exists)
            return true;
    }
    return false;
}

void remove_old_install(std::filesystem::path df_path)
{
    std::string message{
        "Removing legacy files:"
    };

    for (auto file : old_filelist)
    {
        std::error_code ec;

        std::filesystem::path p = df_path / file;

        if (std::filesystem::is_directory(p))
            std::filesystem::remove_all(p, ec);
        else if (std::filesystem::is_regular_file(p))
            std::filesystem::remove(p, ec);
        else
            continue;

        message += "\n" + p.string() + ": " + (ec ? "failed to remove - " + ec.message() : "removed successfully");
    }
#ifdef WIN32
    MessageBoxW(NULL, std::wstring(message.begin(), message.end()).c_str(), L"Legacy Install Cleanup", 0);
#endif
}

#ifdef WIN32
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
#else
int main(int argc, char* argv[]) {
#endif
    // initialize steam context
    if (SteamAPI_RestartAppIfNecessary(DFHACK_STEAM_APPID)) {
        exit(0);
    }

    bool nowait = false;
#ifdef WIN32
    std::wstring cmdline(lpCmdLine);
    if (cmdline.find(L"--nowait") != std::wstring::npos)
        nowait = true;
#else
    for (int idx = 0; idx < argc; ++idx) {
        if (strcmp(argv[idx], "--nowait") == 0)
            nowait = true;
    }
#endif

    if (waitForDF(nowait))
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
    std::filesystem::path dfhack_install_folder = (b1 != -1) ? std::string(buf) : "";

    int b2 = SteamApps()->GetAppInstallDir(DF_STEAM_APPID, (char*)&buf, 2048);
    std::filesystem::path df_install_folder = (b2 != -1) ? std::string(buf) : "";

    if (df_install_folder != dfhack_install_folder)
    {
#ifdef WIN32
        constexpr auto dfhooks_dll_name = "dfhooks.dll";
        constexpr auto dfhook_dfhack_dll_name = "dfhooks_dfhack.dll";
#else
        constexpr auto dfhooks_dll_name = "libdfhooks.so";
        constexpr auto dfhook_dfhack_dll_name = "libdfhooks_dfhack.so";
#endif
        // DF and DFHack are not co-installed (modern case)
        // inject dfhooks.dll and dfhooks_dfhack.ini into DF install folder
        std::filesystem::path dfhooks_dll_src = dfhack_install_folder / dfhooks_dll_name;
        std::filesystem::path dfhooks_dll_dst = df_install_folder / dfhooks_dll_name;
        std::filesystem::path dfhooks_ini_dst = df_install_folder / "dfhooks_dfhack.ini";
        std::filesystem::path dfhooks_dfhack_dll_src = dfhack_install_folder / "hack" / dfhook_dfhack_dll_name;

        std::error_code ec;

        std::filesystem::copy(dfhooks_dll_src, dfhooks_dll_dst, std::filesystem::copy_options::update_existing, ec);
        if (!ec)
        {
            std::string indirection;
            if (std::filesystem::exists(dfhooks_ini_dst))
            {
                std::ifstream ini(dfhooks_ini_dst);
                std::getline(ini, indirection);
            }

            if (indirection != dfhooks_dfhack_dll_src.string())
            {
                std::ofstream ini(dfhooks_ini_dst);
                ini << dfhooks_dfhack_dll_src.string() << std::endl;
            }
        }
        else
        {
#ifdef WIN32
            std::wstring message{
                L"Failed to inject DFHack into Dwarf Fortress\n\n"
                L"Details:\n" + std::filesystem::relative(dfhooks_dll_src).wstring() +
                L" -> " + std::filesystem::relative(dfhooks_dll_dst).wstring() +
                L"\n\nError code: " + std::to_wstring(ec.value()) +
                L"\nError message: " + std::filesystem::relative(ec.message()).wstring()
            };

            MessageBoxW(NULL, message.c_str(), NULL, 0);
#else
            std::string message{
                "Failed to inject DFHack into Dwarf Fortress\n\n"
                "Details:\n" + std::filesystem::relative(dfhooks_dll_src).string() +
                " -> " + std::filesystem::relative(dfhooks_dll_dst).string() +
                "\n\nError code: " + std::to_string(ec.value()) +
                "\nError message: " + std::filesystem::relative(ec.message()).string()
            };

            notify(message.c_str());
#endif
            exit(1);
        }
        bool dirty = check_for_old_install(df_install_folder);
        if (dirty)
        {
#ifdef WIN32
            int ok = MessageBoxW(NULL, L"A legacy install of DFHack has been detected in the Dwarf Fortress folder. This likely means that you have installed DFHack with the old Steam client (or manually). This legacy installation will almost certainly interfere with using DFHack. Do you want to remove the old files now? (recommended)", L"Legacy DFHack Install Detected", MB_OKCANCEL);

            if (ok == IDOK)
                remove_old_install(df_install_folder);
#else
            int response = 0;
            std::string filelist;
            for (auto file : old_filelist)
                if (std::filesystem::exists(df_install_folder / file))
                    filelist += (filelist.empty() ? "" : std::string(",")) + file;

            std::string message{
                "A legacy install of DFHack has been detected in the Dwarf Fortress directory.This likely means that you have installed DFHack with the old Steam client (or manually).This installation will almost certainly interfere with using DFHack. \n\n"
                "To remove these files, run the following command: rm -r " + df_install_folder.string() + "/{ " + filelist + "}\n\n"
            };

            notify(message.c_str());
#endif
        }
    }

    if (!wrap_launch(launch_via_steam))
        exit(1);

    int counter = 0;
    while (!waitForDF(nowait)) {
        if (counter++ > 60) {
#ifdef WIN32
            MessageBoxW(NULL, L"Dwarf Fortress took too long to launch, aborting", NULL, 0);
#else
            notify("Dwarf Fortress took too long to launch, aborting");
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
