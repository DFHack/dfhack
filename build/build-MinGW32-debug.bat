mkdir MINGW32-debug
cd MINGW32-debug
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Debug
mingw32-make
pause