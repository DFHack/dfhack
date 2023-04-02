#include <windows.h>

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hpi, PWSTR cmd, int ns)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR steamPath[1024];
    DWORD datasize = 1024;

    LONG retCode = RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"SteamExe", RRF_RT_REG_SZ, NULL, &steamPath, &datasize);

    if (retCode != ERROR_SUCCESS)
    {
        MessageBoxW(NULL, L"Could not find Steam client executable", NULL, 0);
        exit(1);
    }

    WCHAR commandLine[1024] = L"steam.exe -applaunch 975370";

    if (CreateProcessW(steamPath,
        commandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi) == 0)
    {
        MessageBoxW(NULL, L"could not launch Dwarf Fortress", NULL, 0);
        exit(1);
    }

    exit(0);
}
