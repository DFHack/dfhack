call "%VS140COMNTOOLS%vsvars32.bat"
cd VC2015_32
msbuild /m /p:Platform=Win32 /p:Configuration=Release INSTALL.vcxproj
cd ..