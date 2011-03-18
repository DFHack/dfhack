mkdir VC2010
cd VC2010
echo Pre-generating a build folder
cmake ..\.. -G"Visual Studio 10"
cmake-gui .