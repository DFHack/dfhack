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

In general, the older DF release you use, the less features will work.
DFHack development is always focused on the latest DF version. Go too
far into the past, and only the most basic tools will work.

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
0.31.05 - 0.31.24: spotty support (some things might not work)

0.31.25: full support

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

dfautodump
==========
Automated item dumping tool. All loose items on the floor marked
for dumping are insta-dumped to the position of the in-game cursor.

Cursor must be placed on a floor tile. Instadumped items may not
show up in the cursor description list until you save/reload.

Options:
--------
-d            Destroy instead of dumping. Doesn't require a cursor.

dfcleanmap
==========
Cleans all the splatter that get scattered all over the map.
Only exception is mud. It leaves mud alone.

dfcleanowned
============
Removes the ownership flag from items.
By default, owned food on the floor and rotten items are confistacted and dumped.

Options:
--------
-a            confiscate all owned items
-l            confiscated and dump all items scattered on the floor
-x            confiscate/dump items with wear level 'x' and more
-X            confiscate/dump items with wear level 'X' and more
-d            a dry run. combine with other options to see what will happen without it actually happening.

On Windows:
 * dfremovelitter.bat runs the tool with -lx
 * dfconfiscate.bat runs the tool with -a

dfcleartask
===========
Items marked for a job can get stuck during a abandon/reclaim. This tool aims to fix that.
Best used immediately after a reclaim. Not thoroughly tested, use at your own risk.

dfderamp (by zilpin)
====================
Removes all ramps designated for removal from the map. This is useful for replicating the old channel digging designation.
It also removes any and all 'down ramps' that can remain after a cave-in (you don't have to designate anything for that to happen).

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

dfgrow
======
Makes all saplings present on the map grow into trees (almost) instantly.

dfimmolate
==========
A tool for getting rid of trees and shrubs. By default, it only kills a tree/shrub under the cursor.

Options:
--------
-s            affect all shrubs on the map
-t            affect all trees on the map
-i            instead of just instaburning plants to ashes, produce fire. WILL KILL FPS!

The windows binary releases contain many .bat files that can run the tool with different parameters.

dfstatus (by enjia2000)
=======================
Runs next to DF and shows some vital statistics of the fort, including food and booze.

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

Options:
--------
-a            processes all tiles, even hidden ones.

On windows, it's possible to run dfprospector-all.bat to process all the tiles without messing with terminal windows.
Also on Windos, dfprospector-text.bat will print the output into a file and then show it in a notepad program (whatever opens .txt by default).

dfreveal
========
Reveals the whole map, waits for input and hides it again. If you close
the tool while it waits, the map remains revealed.

dfunreveal
==========
Hides everything and then only reveals the part of the map accessible from the position of DF's cursor. Place the cursor in open space that you want to keep revealed - this should include the surface world.
Can be used to fix maps stuck revealed or hide parts of the fortress blocked off by walls.

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
Simplistic map viewer. Mostly a debug/development thing. Now supported on Windows too!

dfweather
===========
Lets you change the current weather to 'clear sky', 'rainy' or 'snowing'. Fill those ponds without mucking around with dfliquids 
:D Rain can also stop brush fires.

Your tool here
==============
Write one ;)
