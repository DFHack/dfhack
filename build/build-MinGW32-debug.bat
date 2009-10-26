mkdir build-real
cd build-real
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Debug
mingw32-make
pause