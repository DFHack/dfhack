mkdir VC2008
cd VC2008
echo Pre-generating a build folder
cmake ..\.. -G"Visual Studio 9 2008"
cmake-gui .