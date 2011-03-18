mkdir MINGW32-debug
cd MINGW32-debug
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Debug
cmake-gui .
mingw32-make 2> ..\mingw-build-log.txt
pause