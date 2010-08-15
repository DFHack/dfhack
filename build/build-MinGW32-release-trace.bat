mkdir build-real
cd build-real
cmake ..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Release --trace > trace-stdout.txt 2> trace-stderr.txt
mingw32-make 2> log.txt
pause
dir file.xxx 