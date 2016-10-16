IF EXIST DF_PATH.txt SET /P _DF_PATH=<DF_PATH.txt
IF NOT EXIST DF_PATH.txt SET _DF_PATH=%CD%\DF
mkdir VC2015_32
cd VC2015_32
echo Pre-generating a build folder
cmake ..\..\.. -G"Visual Studio 14" -DCMAKE_INSTALL_PREFIX="%_DF_PATH%"
cmake-gui .
