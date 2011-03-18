mkdir VC2005
cd VC2005
echo Pre-generating a build folder
cmake ..\.. -G"Visual Studio 8 2005"
cmake-gui .