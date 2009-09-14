Here's how you build dfhack!
----------------------------

First, there is one dependency:
  cmake - it's the build system

building the library is simple. Enter the build folder, run th tools. Like this:
  cd build
  cmake .. -DCMAKE_BUILD_TYPE:string=Release
  make

Changing folder to build and using 'cmake ..' is important!

This will build the library and its tools and place them in /output.
You can also use a cmake-friendly IDE like KDevelop 4 or the cmake GUI program.

Building on Windows:
--------------------

You need cmake. Get the win32 installer version from the official site: http://www.cmake.org/cmake/resources/software.html
It has the usual installer wizard thing.

* Using mingw:

You also need a compiler. I build dfhack using mingw. You can get it from the mingw site:
Get the automated installer, it will download newest version of mingw and set things up nicely.
You'll have to add C:\MinGW\ to your PATH variable.

 - Building:
 open up cmd and navigate to the dfhack\build folder, run cmake and the mingw version of make:
 cd build
 cmake .. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Release
 mingw32-make

* Using some other compiler:

I'm afraid you are on your own. dfhack wasn't tested with any other compiler.
Try using a different cmake generator that's intended for your tools.

Build targets
-------------
dfhack has a few build targets. If you're only after the library run 'make dfhack'.
'make' will build everything.
'make expbench' will build the expbench throughput testing program and the library.

Build types
-----------
cmake allows you to pick a build type by changing this variable: CMAKE_BUILD_TYPE

cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the CMAKE_CXX_FLAGS variable for building.
Valid build types include 'Release' and 'Debug'. There are others, but they aren't really that useful.

Have fun.