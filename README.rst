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

0.31.04 - 0.31.21 SDL

Linux
=====
0.31.05 - 0.31.19 native.

There are missing offsets but Map tools should be OK. Linux support is
a bit lacking, I'm working on it. Slowly. All supported Windows versions
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

dfprobe
============
Can be used to determine tile properties.

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

dfcleartask
===========
Solves the problem of unusable items after reclaim by clearing the 'in_job' bit of all items.

dfweather
===========
Lets you change the current weather to 'clear sky', 'rainy' or 'snowing'. Fill those ponds without mucking around with dfliquids 
:D Rain can also stop brush fires.

dfmode
===========
This tool lets you change the game mode directly. Not all combinations are good for every situation and most of them will produce undesirable results.
There are a few good ones though.

.. admonition:: Example

     You are in fort game mode (0 game mode), managing your fortress (0 control mode) and paused.
     You switch to the arena game mode, *assume control of a creature* and the switch to adventure game mode(1).
     You just lost a fortress and gained an adventurer.

I take no responsibility of anything that happens as a result of using this tool :P

Your tool here
==============
Write one ;)
