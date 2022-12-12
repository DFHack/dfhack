call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
cd VC2022
msbuild /m /p:Platform=x64 /p:Configuration=RelWithDebInfo ALL_BUILD.vcxproj
cd ..
