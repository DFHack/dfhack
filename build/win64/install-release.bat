call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
cd VC2015
msbuild /m /p:Platform=x64 /p:Configuration=Release INSTALL.vcxproj
cd ..