mkdir linux-remote
cd linux-remote
cmake ..\.. -G"Visual Studio 10"
msbuild ALL_BUILD.vcxproj /p:Configuration=Release
echo FINISHED_BUILD