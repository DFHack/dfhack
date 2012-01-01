call "%VS100COMNTOOLS%vsvars32.bat"
cd VC2010
msbuild /m:4 /p:Platform=Win32 /p:Configuration=Release INSTALL.vcxproj
cd ..