@ECHO OFF
rd /S /Q MSVC10
mkdir MSVC10
cd MSVC10
echo CLEANUP DONE

cmake ..\.. -G"Visual Studio 10" -DDFHACK_INSTALL="portable" -DMEMXML_DATA_PATH="." -DBUILD_DFHACK_C_BINDINGS=OFF -DBUILD_DFHACK_DEVEL=OFF -DBUILD_DFHACK_DOXYGEN=OFF -DBUILD_DFHACK_EXAMPLES=OFF -DBUILD_DFHACK_PLAYGROUND=OFF -DBUILD_DFHACK_PYTHON_BINDINGS=OFF > ..\pkg-win32-cmake.log
if errorlevel 1 goto cmakeerror
echo CMAKE OK

call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >  ..\pkg-win32-env.log
if errorlevel 1 goto enverror
echo ENV OK

msbuild PACKAGE.vcxproj /p:Configuration=Release /l:FileLogger,Microsoft.Build.Engine;logfile=..\pkg-win32-msbuild.log;encoding=utf-8 -noconsolelogger > NUL
if errorlevel 1 goto msvcerror
move /Y *.zip ..
echo BUILD OK
set errorlevel=0
goto end

:cmakeerror
echo CMAKE ERROR
set errorlevel=1
goto end

:enverror
echo ENV ERROR
set errorlevel=1
goto end

:msvcerror
echo MSVC ERROR
set errorlevel=1
goto end

:end