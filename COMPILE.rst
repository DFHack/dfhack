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

See the section on the shared memory hook library (SHM).

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
    projects will end up there! (and that's wrong). Any changes to the
    build system should be done by changing cmake configs and running
    ``cmake`` on them!

Also, you'll have to copy the ``Memory.xml`` file to the build output
folders MSVC generates. For example from ``output/`` to
``output/Release/``

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

Building the shared memory hook library (SHM)
=============================================
Unlike the rest of DFHack, The SHM needs special treatment when it
comes to compilation. Because it shares the memory space with DF
itself, it has to be built with the same tools as DF and use the same C
and C++/STL libraries.

For DF 31.01 - 31.10 on Windows, use MSVC 2008. You can get the Express
edition for free from Microsoft.

Windows dependencies can be determined by a tool like ``depends.exe``
(`google it`_). Both the fake ``SDL.dll`` and DF have to use the same
version of the C runtime (MSVCRT). The SHM can only be debugged using a
RelWithDebInfo build!

Linux dependencies can be determined by setting the LD_DEBUG variable
and running ./df::
    
    export LD_DEBUG=versions
    ./df

Example of (a part of a) relevant output from a working SHM
installation::
    
    24472:     checking for version `GLIBC_2.0' in file /opt/lib32/lib/libpthread.so.0 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GCC_3.0' in file ./libs/libgcc_s.so.1 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.0' in file ./libs/libgcc_s.so.1 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.1' in file /opt/lib32/lib/libm.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.0' in file /opt/lib32/lib/libm.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.1.3' in file /opt/lib32/lib/libc.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.3.4' in file /opt/lib32/lib/libc.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.4' in file /opt/lib32/lib/libc.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBC_2.0' in file /opt/lib32/lib/libc.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBCXX_3.4.9' in file ./libs/libstdc++.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `CXXABI_1.3' in file ./libs/libstdc++.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `GLIBCXX_3.4' in file ./libs/libstdc++.so.6 [0] required by file ./dwarfort.exe [0]
    24472:     checking for version `CXXABI_1.3' in file ./libs/libstdc++.so.6 [0] required by file ./libs/libdfconnect.so [0]
    24472:     checking for version `GLIBCXX_3.4' in file ./libs/libstdc++.so.6 [0] required by file ./libs/libdfconnect.so [0]
    24472:     checking for version `GLIBC_2.1.3' in file /opt/lib32/lib/libc.so.6 [0] required by file ./libs/libdfconnect.so [0]
    24472:     checking for version `GLIBC_2.2' in file /opt/lib32/lib/libc.so.6 [0] required by file ./libs/libdfconnect.so [0]
    24472:     checking for version `GLIBC_2.3.4' in file /opt/lib32/lib/libc.so.6 [0] required by file ./libs/libdfconnect.so [0]
    24472:     checking for version `GLIBC_2.0' in file /opt/lib32/lib/libc.so.6 [0] required by file ./libs/libdfconnect.so [0]

libdfconnect is the SHM. Both are compiled against the same C++ library
and share the same CXXABI version.

Precompiled SHM libraries are provided in binary releases.

.. _google it: http://www.google.com/search?q=depends.exe

Checking strings support
========================
Strings are one of the important C++ types and a great indicator that
the SHM works. Tools like Dwarf Therapist depend on string support.
Reading of strings can be checked by running any of the tools that deal
with materials.

String writing is best tested with a fresh throw-away fort and
``dfrenamer``.

Embark, give one dwarf a very long name using dfrenamer and save/exit.
If DF crashes during the save sequence, your SHM is not compatible with
DF and the throw-away fort is most probably lost.
