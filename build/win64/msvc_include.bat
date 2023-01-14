@ECHO OFF
setlocal enabledelayedexpansion

FOR /F "usebackq tokens=*" %%F IN (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -requires Microsoft.Component.MSBuild -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -version [17.0^,] -products * -find MSBuild\**\Bin\MSBuild.exe`) DO (
    endlocal & set "MSBUILD="%%F""
    goto :EOF
)
echo "Cannot find a Visual Studio 2022/Build Tools installation"
exit 1