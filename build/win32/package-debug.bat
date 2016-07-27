@echo off
call "%VS100COMNTOOLS%vsvars32.bat"
cd VC2010
msbuild /m /p:Platform=Win32 /p:Configuration=RelWithDebInfo PACKAGE.vcxproj
cd ..
exit %ERRORLEVEL%