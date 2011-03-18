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

This will build the library and its tools and place them in a sub-directory ``bin`` inside your build directory.

Alternatively, you can use ccmake instead of cmake:

    cd build
    ccmake ..
    make

This will show a curses-based interface that lets you set all of the
extra options.

You can also use a cmake-friendly IDE like KDevelop 4 or the cmake-gui
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
    by changing the CMakeLists.txt files and running ``cmake``!

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

================================
Using the library as a developer
================================
DFHack is using the zlib/libpng license. This makes it easy to link to
it, use it in-source or add your own extensions. Contributing back to
the dfhack repository is welcome and the right thing to do :)

Rudimentary API documentation can be built using doxygen.

Contributing to DFHack
======================

Several things should be kept in mind when contributing to DFHack.

------------
Coding style
------------
DFhack uses ANSI formatting and four spaces as indentation. Line
endings are UNIX. The files use UTF-8 encoding. Code not following this
won't make me happy, because I'll have to fix it. There's a good chance
I'll make *you* fix it ;)

-------------------------------
How to get new code into DFHack
-------------------------------
You can send patches or make a clone of the github repo and ask me on
the IRC channel to pull your code in. I'll review it and see if there
are any problems. I'll fix them if they are minor.

Fixes are higher in priority. If you want to work on something, but
don't know what, check out http://github.com/peterix/dfhack/issues --
this is also a good place to dump new ideas and/or bugs that need
fixing.

----------------
Layout for tools
----------------
Tools live in the tools/ folder. There, they are split into three
categories.

distributed
    these tools get distributed with binary releases and are installed
    by doing 'make install' on linux. They are supposed to be stable
    and supported. Experimental, useless, buggy or untested stuff
    doesn't belong here.
examples
    examples are tools that aren't very useful, but show how DF and
    DFHack work. They should use only DFHack API functions. No actual
    hacking or 'magic offsets' are allowed.
playground
    This is a catch-all folder for tools that aren't ready to be
    examples or be distributed in binary releases. All new tools should
    start here. They can contain actual hacking, magic values and other
    nasty business.

------------------------
Modules - what are they?
------------------------
DFHack uses modules to partition sets of features into manageable
chunks. A module can have both client and server side.

Client side is the part that goes into the main library and is
generally written in C++. It is exposed to the users of DFHack.

Server side is used inside DF and serves to accelerate the client
modules. This is written mostly in C style.

There's a Core module that shouldn't be changed, because it defines the
basic commands like reading and writing raw data. The client parts for
the Core module are the various implementations of the Process
interface.

A good example of a module is Maps. Named the same in both client and
server, it allows accelerating the reading of map blocks.

Communication between modules happens by using shared memory. This is
pretty fast, but needs quite a bit of care to not break. 

------------
Dependencies
------------
Internal
    either part of the codebase or statically linked.
External
    linked as dynamic loaded libraries (.dll, .so, etc.)

If you want to add dependencies, think twice about it. All internal
dependencies for core dfhack should be either public domain or require
attribution at most. External dependencies for tools can be either
that, or any Free Software licenses.

Current internal dependencies
-----------------------------
tinyxml
    used by core dfhack to read offset definitions from Memory.xml
md5
    an implementation of the MD5 hash algorithm. Used for identifying
    DF binaries on Linux.
argstream
    Allows reading terminal application arguments. GPL!

Current external dependencies
-----------------------------
wide-character ncurses
    used for the veinlook tool on Linux.
x11 libraries
    used for sending key events on linux

Build-time dependencies
-----------------------
cmake
    you need cmake to generate the build system and some configuration
    headers

=========================
Memory offset definitions
=========================
The files with memory offset definitions used by dfhack can be found in the
data folder.

