mkdir build-real
cd build-real
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Release
mingw32-make
pause