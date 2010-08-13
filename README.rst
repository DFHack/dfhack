============
Introduction
============

DFHack is a Dwarf Fortress memory access library and a set of basic
tools using this library. The library is a work in progress, so things
might change as more tools are written for it.

It is an attempt to unite the various ways tools access DF memory and
allow for easier development of new tools.

.. contents::
    

==============
Getting DFHack
==============
The project is currently hosted on github_, for both source and
binaries at  http://github.com/peterix/dfhack

.. _github: http://www.github.com/

Packages
========
The library and tools are packaged for Archlinux and are available both
in AUR and the arch-games repository.

The package name is dfhack-git :)

=============
Compatibility
=============
DFHack works on Windows XP, Vista, 7 or any modern Linux distribution.

OSX is not supported due to lack of developers with a Mac.

Currently supported Dwarf Fortress versions are Windows and Linux.

Windows
=======
.. note::
    
    Windows 2000 is currently *not supported* due to missing OS
    functionality. If you know how to easily suspend processes, you can
    fix it :)

0.31.01 - 0.31.03 legacy
0.31.04 - 0.31.12 SDL

Linux
=====
0.31.05 - 0.31.12 native.
There are missing offsets but Map tools should be OK. Linux support is
a bit lacking, I'm working on it. All supported Windows versions
running in wine can be used with native DFHack binaries.

=====
Tools
=====
All the DFHack tools are terminal programs. This might seem strange to Windows
users, but these are meant mostly as examples for developers. Still, they can
be useful and are cross-platform just like the library itself.

dfcleanmap
==========
Cleans all the splatter that get scattered all over the map.
Only exception is mud. It leaves mud alone.

dfliquids
=========
A command prompt for liquid creation and manipulation (the Moses
effect included!) Also allows painting obsidian walls directly.

.. note::
    
    Spawning and deleting liquids can F up pathing data and
    temperatures (creating heat traps). You've been warned.

dfposition
==========
Prints the current DF window properties and cursor position.

dfprospector
============
Lists all available minerals on the map and how much of them there is.

dfreveal
========
Reveals the whole map, waits for input and hides it again. If you close
the tool while it waits, the map remains revealed.

dfunstuck
=========
Use if you prematurely close any of the tools and DF appears to be
stuck.

dfvdig
======
Designates a whole vein for digging. Point the cursor at a vein and run
this thing :)

dfflows
=======
A tool for checking how many liquid tiles are actively checked for
flows.

dfattachtest
============
Test of the process attach/detach mechanism.

dfsuspend
=========
Test of the process suspend/resume mechanism.

dfexpbench
==========
Just a simple benchmark of the data export speed.

dfdoffsets
==========
Dumps the offsets for the currently running DF version into the terminal.

Your tool here
==============
Write one ;)

================================
Using the library as a developer
================================
The library is compilable under Linux with GCC and under Windows with
MinGW32 and MSVC compilers. It is using the cmake build system. See
COMPILE for details.

DFHack is using the zlib/libpng license. This makes it easy to link to
it, use it in-source or add your own extensions. Contributing back to
the dfhack repository is welcome and the right thing to do :)

At the time of writing there's no API reference or documentation. The
code does have a lot of comments though (and getting better all the
time).

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

