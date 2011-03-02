################
Compiling DFHACK
################

============================
Here's how you build dfhack!
============================

.. contents::
    

Dependencies
============
* ``cmake``
* A compiler for building the main lib and the various tools.
* (Linux only) Veinlook requires the wide-character ncurses library (libncursesw)
* (Linux only) You'll need X11 dev libraries.

Building on Linux
=================
To run in the output folder (without installing) building the library
is simple. Enter the build folder, run the tools. Like this::
    
    cd build
    cmake .. -DCMAKE_BUILD_TYPE:string=Release
    make

This will build the library and its tools and place them in ``/output``.
You can also use a cmake-friendly IDE like KDevelop 4 or the cmake GUI
program.

To be installed into the system or packaged::
    
    cd build
    cmake -DCMAKE_BUILD_TYPE:string=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DMEMXML_DATA_PATH:path=/usr/share/dfhack ..
    make
    make install

With this dfhack installs:

* library to ``$CMAKE_INSTALL_PREFIX/lib``
* executables to ``$CMAKE_INSTALL_PREFIX/bin``
* The ``Memory.xml`` file to ``/usr/share/dfhack``

Building on Windows
===================
You need ``cmake``. Get the win32 installer version from the official
site: http://www.cmake.org/cmake/resources/software.html

It has the usual installer wizard thing.

-----------
Using mingw
-----------
You also need a compiler. I build dfhack using mingw. You can get it
from the mingw site: http://www.mingw.org/

Get the automated installer, it will download newest version of mingw
and set things up nicely.

You'll have to add ``C:\MinGW\`` to your PATH variable.

Building
--------
open up cmd and navigate to the ``dfhack\build`` folder, run ``cmake``
and the mingw version of make::
    
    cd build
    cmake .. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE:string=Release
    mingw32-make

----------
Using MSVC
----------
open up ``cmd`` and navigate to the ``dfhack\build`` folder, run
``cmake``::
    
    cd build
    cmake ..

This will generate MSVC solution and project files.

.. note::
    
    You are working in the ``/build`` folder. Files added to
    projects from within MSVC will end up there! (and that's
    wrong). Any changes to the build system should be done
    by changing cmake configs and running ``cmake`` on them!

-------------------------
Using some other compiler
-------------------------
I'm afraid you are on your own. dfhack wasn't tested with any other
compiler.

Try using a different cmake generator that's intended for your tools.

Build targets
=============
dfhack has a few build targets:

* If you're only after the library run ``make dfhack``.
* ``make`` will build everything.
* ``make expbench`` will build the expbench testing program and the
  library.
* Some of the utilities and the doxygen documentation won't be
  normally built. You can enable them by specifying some extra
  CMake variables::

    BUILD_DFHACK_DOCUMENTATION - generate the documentation (really bad)
    BUILD_DFHACK_EXAMPLES      - build tools from tools/examples
    BUILD_DFHACK_PLAYGROUND    - build tools from tools/playground
    
  Example::

    cmake .. -DBUILD_DFHACK_EXAMPLES=ON

Build types
===========
``cmake`` allows you to pick a build type by changing this
variable: ``CMAKE_BUILD_TYPE``

::
    
    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid an useful build types include 'Release', 'Debug' and
'RelWithDebInfo'. There are others, but they aren't really that useful.

Have fun.
