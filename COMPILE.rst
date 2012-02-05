###############
Building DFHACK
###############

.. contents::

=================
Building on Linux
=================
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
============
DFHack is meant to be installed into an existing DF folder, so get one ready.

For building, you need a 32-bit version of GCC. For example, to build DFHack on
a 64-bit distribution like Arch, you'll need the multilib development tools and libraries.

Before you can build anything, you'll also need ``cmake``. It is advisable to also get
``ccmake`` on distributions that split the cmake package into multiple parts.

For the code generation parts, you need perl and the XML::LibXML and XML::LibXSLT perl packages.
You should be able to find them in your distro repositories (on Arch linux 'perl-xml-libxml' and 'perl-xml-libxslt').

Build
=====
Building is fairly straightforward. Enter the ``build`` folder and start the build like this::
    
    cd build
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=/home/user/DF
    make install

Obviously, replace the install path with path to your DF. This will build the library
along with the normal set of plugins and install them into your DF folder.

Alternatively, you can use ccmake instead of cmake::
    
    cd build
    ccmake ..
    make install

This will show a curses-based interface that lets you set all of the
extra options.

You can also use a cmake-friendly IDE like KDevelop 4 or the cmake-gui
program.

===================
Building on Windows
===================
First, you need ``cmake``. Get the win32 installer version from the official
site: http://www.cmake.org/cmake/resources/software.html

It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

You'll need a copy of Microsoft Visual C++ 2010. The Express version is sufficient.
Grab it from Microsoft's site.

For the code generation parts, you'll need perl and XML::LibXML. You can install them like this:

* download and install strawberry perl from http://strawberryperl.com/
* reboot so that the system can pick up the new binary path
* open a cmd.exe window and run "cpan XML::LibXML" (obviously without the quotes). This can take a while to complete.
* Same with "cpan XML::LibXSLT".

If you already have a different version of perl (for example the one from cygwin), you can run into some trouble. Either remove the other perl install from PATH, or install libxml and libxslt for it instead. Strawberry perl works though and has all the required packages.

Build
=====
There are several different batch files in the ``build`` folder along with a script that's used for picking the DF path.

First, run set_df_path.vbs and point the dialog that pops up at your DF folder that you want to use for development.
Next, run one of the scripts with ``generate`` prefix. These create the MSVC solution file(s):

* ``all`` will create a solution with everything enabled (and the kitchen sink).
* ``gui`` will pop up the cmake gui and let you pick and choose what to build. This is probably what you want most of the time. Set the options you are interested in, then hit configure, then generate. More options can appear after the configure step.
* ``minimal`` will create a minimal solution with just the bare necessities - the main library and standard plugins.

Then you can either open the solution with MSVC or use one of the msbuild scripts:

* Scripts with ``build`` prefix will only build.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

When you open the solution in MSVC, make sure you never use the Debug builds. Those aren't
binary-compatible with DF. If you try to use a debug build with DF, you'll only get crashes.
So pick either Release or RelWithDebInfo build and build the INSTALL target.

The ``debug`` scripts actually do RelWithDebInfo builds.


===========
Build types
===========
``cmake`` allows you to pick a build type by changing this
variable: ``CMAKE_BUILD_TYPE``

::
    
    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid and useful build types include 'Release', 'Debug' and
'RelWithDebInfo'. 'Debug' is not available on Windows.

================================
Using the library as a developer
================================
Currently, the only way to use the library is to write a plugin that can be loaded by it.
All the plugins can be found in the 'plugins' folder. There's no in-depth documentation
on how to write one yet, but it should be easy enough to copy one and just follow the pattern.

The most important parts of DFHack are the Core, Console, Modules and Plugins.

* Core acts as the centerpiece of DFHack - it acts as a filter between DF and SDL and synchronizes the various plugins with DF.
* Console is a thread-safe console that can be used to invoke commands exported by Plugins.
* Modules actually describe the way to access information in DF's memory. You can get them from the Core. Most modules are split into two parts: high-level and low-level. Higl-level is mostly method calls, low-level publicly visible pointers to DF's data structures.
* Plugins are the tools that use all the other stuff to make things happen. A plugin can have a list of commands that it exports and an onupdate function that will be called each DF game tick.

Rudimentary API documentation can be built using doxygen (see build options with ``ccmake`` or ``cmake-gui``).

DFHack consists of variously licensed code, but invariably weak copyleft.
The main license is zlib/libpng, some bits are MIT licensed, and some are BSD licensed.

Feel free to add your own extensions and plugins. Contributing back to
the dfhack repository is welcome and the right thing to do :)

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

---------------
Memory research
---------------
If you want to do memory research, you'll need some tools and some knowledge.
In general, you'll need a good memory viewer and optionally something
to look at machine code without getting crazy :)

Good windows tools include:

* Cheat Engine
* IDA Pro (the free version)

Good linux tools:

* angavrilov's df-structures gui (visit us on IRC for details).
* edb (Evan's Debugger)
* IDA Pro running under wine.
* Some of the tools residing in the ``legacy`` dfhack branch.

Using publicly known information and analyzing the game's data is preferred.
