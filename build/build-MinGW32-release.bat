mkdir MINGW32-release
cd MINGW32-release
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Release
mingw32-make 2> log.txt
pause