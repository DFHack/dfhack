IF EXIST DF_PATH.txt SET /P _DF_PATH=<DF_PATH.txt
IF NOT EXIST DF_PATH.txt SET _DF_PATH=%CD%\DF
echo Pre-generating a build folder
cmake ..\.. -G"Visual Studio 17 2022" -A x64 -B VC2022 -DCMAKE_INSTALL_PREFIX="%_DF_PATH%"
cmake-gui VC2022
