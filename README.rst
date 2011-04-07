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

0.31.04 - 0.31.25 SDL

Linux
=====
Native linux DF
---------------
0.31.05 - 0.31.19: spotty support (some things might not work)

0.31.22 - 0.31.25: full support

Windows DF under wine
---------------------
0.31.01 - 0.31.03 legacy

0.31.04 - 0.31.25 SDL

=====
Tools
=====
All the DFHack tools are terminal programs. This might seem strange to Windows
users, but these are meant mostly as examples for developers. Still, they can
be useful and are cross-platform just like the library itself.

dfattachtest
============
Test of the process attach/detach mechanism.

dfcleanmap
==========
Cleans all the splatter that get scattered all over the map.
Only exception is mud. It leaves mud alone.

dfderamp (by zilpin)
====================
Removes all ramps designated for removal from the map. This is useful for replicating the old channel digging designation.

dfdoffsets
==========
Dumps the offsets for the currently running DF version into the terminal.

dfexpbench
==========
Just a simple benchmark of the data export speed.

dfflows
=======
A tool for checking how many liquid tiles are actively checked for flows.

dfincremental
=============
A simple memory search tool for DF. Requires arcane knowledge ;)

dflair
======
Marks all of the map as a lair, preventing item scatter on abandon/reclaim.

dfliquids
=========
A command prompt for liquid creation and manipulation (the Moses
effect included!) Also allows painting obsidian walls directly.

.. note::
    
    Spawning and deleting liquids can F up pathing data and
    temperatures (creating heat traps). You've been warned.

dfmode
======
This tool lets you change the game mode directly. Not all combinations are good for every situation and most of them will produce undesirable results.
There are a few good ones though.

.. admonition:: Example

     You are in fort game mode (0 game mode), managing your fortress (0 control mode) and paused.
     You switch to the arena game mode, *assume control of a creature* and the switch to adventure game mode(1).
     You just lost a fortress and gained an adventurer.

I take no responsibility of anything that happens as a result of using this tool :P

dfpause
=======
Forces DF to pause. This is useful when your FPS drops below 1 and you lose control of the game.

dfposition
==========
Prints the game mode, current DF window properties and cursor position in both local and world coordinates.

dfprobe
=======
Can be used to determine tile properties like temperature.

dfprospector
============
Lists all available minerals on the map and how much of them there is.
It has two parameters:
-a processes all tiles, even hidden ones.
-b includes layer rocks into the count.

dfreveal
========
Reveals the whole map, waits for input and hides it again. If you close
the tool while it waits, the map remains revealed.

dfsuspend
=========
Test of the process suspend/resume mechanism. If this doesn't work as expected, it's not safe to use DFHack.

dftubefill
==========
Fills all the 'candy stores' with 'delicious candy'. No need to fear the clowns. Don't use if you haven't seen the hidden fun stuff 
yet ;)

dfunstuck
=========
Use if you prematurely close any of the tools and DF appears to be stuck. Mostly only needed on Windows.

dfvdig
======
Designates a whole vein for digging. Point the cursor at a vein and run this thing :)
Running 'dfXvdig' on Windows or using the '-x' parameter will dig stairs between z-levels to follow the veins.

dfveinlook
==========
Simplistic map viewer for linux. Mostly a debug/development thing.

dfweather
===========
Lets you change the current weather to 'clear sky', 'rainy' or 'snowing'. Fill those ponds without mucking around with dfliquids 
:D Rain can also stop brush fires.

Your tool here
==============
Write one ;)
