============
Introduction
============

DFHack is a Dwarf Fortress memory access library and a set of basic
tools that use it. Tools come in the form of plugins or (not yet) 
external tools. It is an attempt to unite the various ways tools
access DF memory and allow for easier development of new tools.

.. contents::
    
==============
Getting DFHack
==============
The project is currently hosted on github_, for both source and
binaries at  http://github.com/peterix/dfhack

.. _github: http://www.github.com/

Releases can be downloaded from here: https://github.com/peterix/dfhack/downloads

All new releases are announced in the bay12 thread: http://tinyurl.com/dfhack-ng

=============
Compatibility
=============
DFHack works on Windows XP, Vista, 7 or any modern Linux distribution.
OSX is not supported due to lack of developers with a Mac.

Currently, only the 31.25 version is supported. If you need DFHack
for older versions, look for older releases.

On Windows, you have to use the SDL version of DF.

It is possible to use the Windows DFHack under wine/OSX. 

====================
Installation/Removal
====================
Installing DFhack involves copying files into your DF folder.
Copy the files from a release archive so that:

 * On Windows, SDL.dll is replaced
 * On Linux, the 'dfhack' script is placed in the same folder as the 'df' script

Uninstalling is basically the same, in reverse:

 * On Windows, first delete SDL.dll and rename SDLreal.dll to SDL.dll. Then remove the other DFHack files
 * On Linux, Remove the DFHack files.

The stonesense plugin might require some additional libraries on Linux:
 * libjpeg 8

If it refuses to load, check the stderr.log file created in your DF folder.

============
Using DFHack
============
DFHack basically extends what DF can do with something similar to a quake console. On Windows, this is a separate command line window. On linux, the terminal used to launch the dfhack script is taken over (so, make sure you start from a terminal).
Basic interaction with dfhack involves entering commands into the console. For some basic instroduction, use the 'help' command. To list all possible commands, use the 'ls' command.
Many commands have their own help or detailed description. You can use 'command help' or 'command ?' to show that.

The command line has some nice line editing capabilities, including history that's preserved between different runs of DF (use up/down keys to go through the history).

The second way to interact with DFHack is to bind the available commands to in-game hotkeys. This is done in the hotkey/zoom menu (normally opened with the 'h' key). Binding the commands is done by assigning a command as a hotkey name (with 'n').
Some commands can't be used from hotkeys - this includes interactive commands like 'liquids' and commands that have names longer than 9 characters.

Most of the commands come from plugins. Those reside in 'DF/plugins/'.

=============================
Something doesn't work, help!
=============================
First, don't panic :) Second, dfhack keeps a few log files in DF's folder - stderr.log and stdout.log. You can look at those and possibly find out what's happening.
If you found a bug, you can either report it in the bay12 DFHack thread, the issues tracker on github, contact me (peterix@gmail.com) or visit the #dfhack IRC channel on freenode.

========
Commands
========

autodump
========
Automated item dumping tool. All loose items on the floor marked
for dumping are insta-dumped to the position of the in-game cursor.

Cursor must be placed on a floor tile. Instadumped items may not
show up in the cursor description list until you save/reload.

Options
-------
:destroy:            Destroy instead of dumping. Doesn't require a cursor.

cleanmap
========
Cleans all the splatter that get scattered all over the map.
By default, it leaves mud and snow alone.

cleanowned
==========
Confiscates items owned by dwarfs.
By default, owned food on the floor and rotten items are confistacted and dumped.

Options
-------
:all:          confiscate all owned items
:scattered:    confiscated and dump all items scattered on the floor
:x:            confiscate/dump items with wear level 'x' and more
:X:            confiscate/dump items with wear level 'X' and more
:dryrun:       a dry run. combine with other options to see what will happen without it actually happening.

colonies
========
Allows listing all the vermin colonies on the map and optionally turning them into honey bee colonies.

Options
-------
:bees: turn colonies into honey bee colonies


deramp (by zilpin)
==================
Removes all ramps designated for removal from the map. This is useful for replicating the old channel digging designation.
It also removes any and all 'down ramps' that can remain after a cave-in (you don't have to designate anything for that to happen).

dfusion
=======
This is the DFusion lua plugin system by warmist/darius, running as a DFHack plugin.

See the bay12 thread for details: http://www.bay12forums.com/smf/index.php?topic=69682.15

Confirmed working DFusion plugins:
----------------------------------
:simple_embark:allows changing the number of dwarves available on embark.

.. note::
    
    * Some of the DFusion plugins aren't completely ported yet. This can lead to crashes.
    * This is currently working only on Windows.
    * The game will be suspended while you're using dfusion. Don't panic when it doen't respond.

flows
=====
A tool for checking how many tiles contain flowing liquids. If you suspect that your magma sea leaks into HFS, you can use this tool to be sure without revealing the map.

grow
====
Makes all saplings present on the map grow into trees (almost) instantly.

extirpate
=========
A tool for getting rid of trees and shrubs. By default, it only kills a tree/shrub under the cursor.
The plants are turned into ashes instantly.

Options
-------
:shrubs:            affect all shrubs on the map
:trees:             affect all trees on the map
:all:               affect every plant!

immolate
========
Very similar to extirpate, but additionally sets the plants on fire. The fires can and *will* spread ;)


liquids
=======
Allows adding magma, water and obsidian to the game. It replaces the normal dfhack command line and can't be used from a hotkey.
For more information, refer to the command's internal help.

.. note::
    
    Spawning and deleting liquids can F up pathing data and
    temperatures (creating heat traps). You've been warned.

mode
======
This command lets you see and change the game mode directly. Not all combinations are good for every situation and most of them will produce undesirable results.
There are a few good ones though.

.. admonition:: Example

     You are in fort game mode, managing your fortress and paused.
     You switch to the arena game mode, *assume control of a creature* and then switch to adventure game mode(1).
     You just lost a fortress and gained an adventurer.

I take no responsibility of anything that happens as a result of using this tool :P

forcepause
==========
Forces DF to pause. This is useful when your FPS drops below 1 and you lose control of the game.

die
===
Instantly kills DF without saving.

probe
=====
Can be used to determine tile properties like temperature.

prospector
============
Lists all available minerals on the map and how much of them there is. By default, only processes the already discovered part of the map.

Options
-------
:all:            processes all tiles, even hidden ones.

reveal
======
This reveals the map. By default, HFS will remain hidden so that the demons don't spawn. You can use 'reveal hell' to reveal everything. With hell revealed, you won't be able to unpause until you hide the map again.

unreveal
========
Reverts the effects of 'reveal'.

revtoggle
=========
Switches between 'reveal' and 'unreveal'.

revflood
========
This command will hide the whole map and then reveal all the tiles that have a path to the in-game cursor.

ssense / stonesense
===================
This is an isometric visualizer that is runs in a second window.

This requires working graphics acceleration, at least a dual core CPU (otherwise it will slow down DF) and on Linux, the allegro 5 libraries installed (look for 'allegro5' in your package manager app).

All the data resides in the 'stonesense' directory.

Older versions, support and extra graphics can be found in the bay12 forum thread:
http://www.bay12forums.com/smf/index.php?topic=43260.0

Some additional resources:
http://df.magmawiki.com/index.php/Utility:Stonesense/Content_repository

tubefill
==========
Fills all the adamantine veins again. Veins that were empty will be filled in too, but might still trigger a demon invasion (this is a known bug).

vdig
====
Designates a whole vein for digging. Requires an active in-game cursor placed over a vein tile. With the 'x' option, it will traverse z-levels (putting stairs between the same-material tiles).

vdigx
=====
A permanent alias for 'vdig x'.

expdig
======
This command can be used for exploratory mining.

See: http://df.magmawiki.com/index.php/DF2010:Exploratory_mining

There are two variables that can be set: pattern and filter.

Patterns:
---------
:diag5:            diagonals separated by 5 tiles
:ladder:           A 'ladder' pattern
:clear:            Just remove all dig designations
:cross:            A cross, exactly in the middle of the map.

Filters:
--------
:all:              designate whole z-level
:hidden:           designate only hidden tiles of z-level (default)
:designated:       Take current designation and apply pattern to it.

After you have a pattern set, you can use 'expdig' to apply it again.

Examples:
---------
* 'expdig diag5 hidden' = designate the diagonal 5 patter over all hidden tiles.
* 'expdig' = apply last used pattern and filter.
* 'expdig ladder designated' = Take current designations and replace them with the ladder pattern.

weather
=======
Lets you change the current weather to 'clear sky', 'rainy' or 'snowing'.
Fill those ponds without mucking around with dfliquids :D Rain can also stop brush fires.