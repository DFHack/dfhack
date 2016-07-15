@echo off
IF EXIST DF_PATH.txt SET /P _DF_PATH=<DF_PATH.txt
IF NOT EXIST DF_PATH.txt SET _DF_PATH=%CD%\DF
mkdir VC2010
cd VC2010
echo Pre-generating a build folder
cmake ..\.. -G"Visual Studio 10" -DCMAKE_INSTALL_PREFIX="%_DF_PATH%" || goto :error
cmake-gui ..\..
:error
set err=%ERRORLEVEL%
cd ..
exit /b %err%
