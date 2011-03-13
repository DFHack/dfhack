mkdir build-real
cd build-real
cmake ..\.. -G"Visual Studio 10"
msbuild ALL_BUILD.vcxproj /p:Configuration=Release
echo FINISHED_BUILD