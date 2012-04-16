@echo off
call "%VS100COMNTOOLS%vsvars32.bat"
cd VC2010
msbuild /m /p:Platform=Win32 /p:Configuration=Release PACKAGE.vcxproj
cd ..
exit %ERRORLEVEL%