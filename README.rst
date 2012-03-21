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

Currently, only version 0.34.05 is supported. If you need DFHack
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

The stonesense plugin might require some additional libraries on Linux.

If any of the plugins or dfhack itself refuses to load, check the stderr.log file created in your DF folder.

============
Using DFHack
============
DFHack basically extends what DF can do with something similar to the drop-down console found in Quake engine games. On Windows, this is a separate command line window. On linux, the terminal used to launch the dfhack script is taken over (so, make sure you start from a terminal).
Basic interaction with dfhack involves entering commands into the console. For some basic instroduction, use the 'help' command. To list all possible commands, use the 'ls' command.
Many commands have their own help or detailed description. You can use 'command help' or 'command ?' to show that.

The command line has some nice line editing capabilities, including history that's preserved between different runs of DF (use up/down keys to go through the history).

The second way to interact with DFHack is to bind the available commands to in-game hotkeys. The old way to do this is via the hotkey/zoom menu (normally opened with the 'h' key). Binding the commands is done by assigning a command as a hotkey name (with 'n').

A new and more flexible way is the keybinding command in the dfhack console. However, bindings created this way are not automatically remembered between runs of the game, so it becomes necessary to use the dfhack.init file to ensure that they are re-created every time it is loaded.

Interactive commands like 'liquids' cannot be used as hotkeys.

Most of the commands come from plugins. Those reside in 'hack/plugins/'.

=============================
Something doesn't work, help!
=============================
First, don't panic :) Second, dfhack keeps a few log files in DF's folder - stderr.log and stdout.log. You can look at those and possibly find out what's happening.
If you found a bug, you can either report it in the bay12 DFHack thread, the issues tracker on github, contact me (peterix@gmail.com) or visit the #dfhack IRC channel on freenode.

=============
The init file
=============
If your DF folder contains a file named dfhack.init, its contents will be run every time you start DF. This allows setting up keybindings. An example file is provided as dfhack.init-example - you can tweak it and rename to dfhack.init if you want to use this functionality.

========
Commands
========

Almost all the commands support using the 'help <command-name>' built-in command to retrieve further help without having to look at this document. Alternatively, some accept a 'help'/'?' option on their command line.

adv-bodyswap
============
This allows taking control over your followers and other creatures in adventure mode. For example, you can make them pick up new arms and armor and equip them properly.

Usage
-----
 * When viewing unit details, body-swaps into that unit.
 * In the main adventure mode screen, reverts transient swap.

advtools
========
A package of different adventure mode tools (currently just one)

Usage
-----
:list-equipped [all]: List armor and weapons equipped by your companions. If all is specified, also lists non-metal clothing.

changevein
==========
Changes material of the vein under cursor to the specified inorganic RAW material.

follow
======
Makes the game view follow the currently highlighted unit after you exit from current menu/cursor mode. Handy for watching dwarves running around. Deactivated by moving the view manually.

forcepause
==========
Forces DF to pause. This is useful when your FPS drops below 1 and you lose control of the game.

 * Activate with 'forcepause 1'
 * Deactivate with 'forcepause 0'

nopause
=======
Disables pausing (both manual and automatic) with the exception of pause forced by 'reveal hell'.
This is nice for digging under rivers.

die
===
Instantly kills DF without saving.

autodump
========
This utility lets you quickly move all items designated to be dumped.
Items are instantly moved to the cursor position, the dump flag is unset,
and the forbid flag is set, as if it had been dumped normally.
Be aware that any active dump item tasks still point at the item.

Cursor must be placed on a floor tile so the items can be dumped there.

Options
-------
:destroy:            Destroy instead of dumping. Doesn't require a cursor.
:destroy-here:       Destroy items only under the cursor.
:visible:            Only process items that are not hidden.
:hidden:             Only process hidden items.
:forbidden:          Only process forbidden items (default: only unforbidden).

autodump-destroy-here
=====================
Destroy items marked for dumping under cursor. Identical to autodump destroy-here, but intended for use as keybinding.

autodump-destroy-item
=====================
Destroy the selected item. The item may be selected in the 'k' list, or inside a container. If called again before the game is resumed, cancels destroy.

clean
=====
Cleans all the splatter that get scattered all over the map, items and creatures.
In an old fortress, this can significantly reduce FPS lag. It can also spoil your
!!FUN!!, so think before you use it.

Options
-------
:map:          Clean the map tiles. By default, it leaves mud and snow alone.
:units:        Clean the creatures. Will also clean hostiles.
:items:        Clean all the items. Even a poisoned blade.

Extra options for 'map'
-----------------------
:mud:          Remove mud in addition to the normal stuff.
:snow:         Also remove snow coverings.

spotclean
=========
Works like 'clean map snow mud', but only for the tile under the cursor. Ideal if you want to keep that bloody entrance 'clean map' would clean up.

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

Example:
--------
``cleanowned scattered X`` : This will confiscate rotten and dropped food, garbage on the floors and any worn items with 'X' damage and above.

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

df2minecraft
============
This generates a minecraft world out of the currently loaded fortress.
Generated worlds are placed into your DF folder, named "World #".

.. warning::

    * This is experimental! It *will* cause crashes.
    * If it works, the process takes quite a while to complete.
    * Do not use if you have any unsaved progress!
    * Non-square embarks are exported wrong. It's a known bug.

dfusion
=======
This is the DFusion lua plugin system by warmist/darius, running as a DFHack plugin.

See the bay12 thread for details: http://www.bay12forums.com/smf/index.php?topic=69682.15

Confirmed working DFusion plugins:
----------------------------------
:simple_embark:   allows changing the number of dwarves available on embark.

.. note::

    * Some of the DFusion plugins aren't completely ported yet. This can lead to crashes.
    * This is currently working only on Windows.
    * The game will be suspended while you're using dfusion. Don't panic when it doen't respond.

drybuckets
==========
This utility removes water from all buckets in your fortress, allowing them to be safely used for making lye.

fastdwarf
=========
Makes your minions move at ludicrous speeds.

 * Activate with 'fastdwarf 1'
 * Deactivate with 'fastdwarf 0'

feature
=======
Enables management of map features.

* Discovering a magma feature (magma pool, volcano, magma sea, or curious underground structure) permits magma workshops and furnaces to be built.
* Discovering a cavern layer causes plants (trees, shrubs, and grass) from that cavern to grow within your fortress.

Options
-------
:list:         Lists all map features in your current embark by index.
:show X:       Marks the selected map feature as discovered.
:hide X:       Marks the selected map feature as undiscovered.

filltraffic
===========
Set traffic designations using flood-fill starting at the cursor.

Traffic Type Codes:
-------------------
:H:     High Traffic
:N:     Normal Traffic
:L:     Low Traffic
:R:     Restricted Traffic

Other Options:
--------------
:X: Fill accross z-levels.
:B: Include buildings and stockpiles.
:P: Include empty space.

Example:
--------
'filltraffic H' - When used in a room with doors, it will set traffic to HIGH in just that room.

alltraffic
==========
Set traffic designations for every single tile of the map (useful for resetting traffic designations).

Traffic Type Codes:
-------------------
:H:     High Traffic
:N:     Normal Traffic
:L:     Low Traffic
:R:     Restricted Traffic

Example:
--------
'alltraffic N' - Set traffic to 'normal' for all tiles.

fixdiplomats
============
Up to version 0.31.12, Elves only sent Diplomats to your fortress to propose tree cutting quotas due to a bug; once that bug was fixed, Elves stopped caring about excess tree cutting. This command adds a Diplomat position to all Elven civilizations, allowing them to negotiate tree cutting quotas (and allowing you to violate them and potentially start wars) in case you haven't already modified your raws accordingly.

fixmerchants
============
This command adds the Guild Representative position to all Human civilizations, allowing them to make trade agreements (just as they did back in 0.28.181.40d and earlier) in case you haven't already modified your raws accordingly.

fixveins
========
Removes invalid references to mineral inclusions and restores missing ones. Use this if you broke your embark with tools like tiletypes, or if you accidentally placed a construction on top of a valuable mineral floor.

fixwagons
=========
Due to a bug in all releases of version 0.31, merchants no longer bring wagons with their caravans. This command re-enables them for all appropriate civilizations.

flows
=====
A tool for checking how many tiles contain flowing liquids. If you suspect that your magma sea leaks into HFS, you can use this tool to be sure without revealing the map.

getplants
=========
This tool allows plant gathering and tree cutting by RAW ID. Specify the types of trees to cut down and/or shrubs to gather by their plant names, separated by spaces.

Options
-------
:-t:        Select trees only (exclude shrubs)
:-s:        Select shrubs only (exclude trees)
:-c:        Clear designations instead of setting them
:-x:        Apply selected action to all plants except those specified (invert selection)

Specifying both -t and -s will have no effect. If no plant IDs are specified, all valid plant IDs will be listed.

tidlers
=======
Toggle between all possible positions where the idlers count can be placed.

twaterlvl
=========
Toggle between displaying/not displaying liquid depth as numbers.

job
===
Command for general job query and manipulation.

Options:
 * no extra options - Print details of the current job. The job can be selected in a workshop, or the unit/jobs screen.
 * list - Print details of all jobs in the selected workshop.
 * item-material <item-idx> <material[:subtoken]> - Replace the exact material id in the job item.
 * item-type <item-idx> <type[:subtype]> - Replace the exact item type id in the job item.

job-material
============
Alter the material of the selected job. Invoked as: job-material <inorganic-token>

Intended to be used as a keybinding:
 * In 'q' mode, when a job is highlighted within a workshop or furnace, changes the material of the job. Only inorganic materials can be used in this mode.
 * In 'b' mode, during selection of building components positions the cursor over the first available choice with the matching material.

job-duplicate
=============
Duplicate the selected job in a workshop:
 * In 'q' mode, when a job is highlighted within a workshop or furnace building, instantly duplicates the job.

keybinding
==========

Manages DFHack keybindings. Currently it supports any combination of Ctrl/Alt/Shift with F1-F9, or A-Z.

Options
-------
:keybinding list <key>: List bindings active for the key combination.
:keybinding clear <key> <key>...: Remove bindings for the specified keys.
:keybinding add <key> "cmdline" "cmdline"...: Add bindings for the specified key.
:keybinding set <key> "cmdline" "cmdline"...: Clear, and then add bindings for the specified key.

When multiple commands are bound to the same key combination, DFHack selects the first applicable one. Later 'add' commands, and earlier entries within one 'add' command have priority. Commands that are not specifically intended for use as a hotkey are always considered applicable.

liquids
=======
Allows adding magma, water and obsidian to the game. It replaces the normal dfhack command line and can't be used from a hotkey.
For more information, refer to the command's internal help.

.. note::

    Spawning and deleting liquids can F up pathing data and
    temperatures (creating heat traps). You've been warned.

liquidsgo
=========
Allows adding magma, water and obsidian to the game. It replaces the normal dfhack command line and can't be used from a hotkey. Settings will be remembered as long as dfhack runs. Intended for use in combination with the command liquidsgo-here (which can be bound to a hotkey).
For more information, refer to the command's internal help. 

liquidsgo-here
==============
Run the liquid spawner with the current/last settings made in liquidsgo (if no settings in liquidsgo were made it paints a point of 7/7 magma by default).
Intended to be used as keybinding. Requires an active in-game cursor.
	
mode
====
This command lets you see and change the game mode directly. Not all combinations are good for every situation and most of them will produce undesirable results.
There are a few good ones though.

.. admonition:: Example

     You are in fort game mode, managing your fortress and paused.
     You switch to the arena game mode, *assume control of a creature* and then switch to adventure game mode(1).
     You just lost a fortress and gained an adventurer.

I take no responsibility of anything that happens as a result of using this tool :P

extirpate
=========
A tool for getting rid of trees and shrubs. By default, it only kills a tree/shrub under the cursor.
The plants are turned into ashes instantly.

Options
-------
:shrubs:            affect all shrubs on the map
:trees:             affect all trees on the map
:all:               affect every plant!

grow
====
Makes all saplings present on the map grow into trees (almost) instantly.

immolate
========
Very similar to extirpate, but additionally sets the plants on fire. The fires can and *will* spread ;)

probe
=====
Can be used to determine tile properties like temperature.

prospect
========
Prints a big list of all the present minerals and plants. By default, only the visible part of the map is scanned.

Options
-------
:all:   Scan the whole map, as if it was revealed.
:value: Show material value in the output. Most useful for gems.
:hell:  Show the Z range of HFS tubes. Implies 'all'.

Pre-embark estimate
-------------------
If called during the embark selection screen, displays an estimate of layer stone availability.
If the 'all' option is specified, also estimates veins. The estimate is computed either
for 1 embark tile of the blinking biome, or for all tiles of the embark rectangle.

Options
-------
:all:            processes all tiles, even hidden ones.

regrass
=======
Regrows all surface grass, restoring outdoor plant growth for pre-0.31.19 worlds.

rename
======
Allows renaming various things.

Options
-------
:rename squad <index> "name": Rename squad by index to 'name'.
:rename hotkey <index> \"name\": Rename hotkey by index. This allows assigning longer commands to the DF hotkeys.
:rename unit "nickname": Rename a unit/creature highlighted in the DF user interface.
:rename unit-profession "custom profession": Change proffession name of the highlighted unit/creature.

reveal
======
This reveals the map. By default, HFS will remain hidden so that the demons don't spawn. You can use 'reveal hell' to reveal everything. With hell revealed, you won't be able to unpause until you hide the map again. If you really want to unpause with hell revealed, use 'reveal demons'.

Reveal also works in adventure mode, but any of its effects are negated once you move. When you use it this way, you don't need to run 'unreveal'.

unreveal
========
Reverts the effects of 'reveal'.

revtoggle
=========
Switches between 'reveal' and 'unreveal'.

revflood
========
This command will hide the whole map and then reveal all the tiles that have a path to the in-game cursor.

revforget
=========
When you use reveal, it saves information about what was/wasn't visible before revealing everything. Unreveal uses this information to hide things again. This command throws away the information. For example, use in cases where you abandoned with the fort revealed and no longer want the data.

lair
====
This command allows you to mark the map as 'monster lair', preventing item scatter on abandon. When invoked as 'lair reset', it does the opposite.

Unlike reveal, this command doesn't save the information about tiles - you won't be able to restore state of real monster lairs using 'lair reset'.

Options
-------
:lair: Mark the map as monster lair
:lair reset: Mark the map as ordinary (not lair)

seedwatch
=========
Tool for turning cooking of seeds and plants on/off depending on how much you have of them.

See 'seedwatch help' for detailed description.

showmood
========
Shows all items needed for the currently active strange mood.

copystock
==========
Copies the parameters of the currently highlighted stockpile to the custom stockpile settings and switches to custom stockpile placement mode, effectively allowing you to copy/paste stockpiles easily.

ssense / stonesense
===================
An isometric visualizer that runs in a second window. This requires working graphics acceleration and at least a dual core CPU (otherwise it will slow down DF).

All the data resides in the 'stonesense' directory. For detailed instructions, see stonesense/README.txt

Compatible with Windows > XP SP3 and most modern Linux distributions.

Older versions, support and extra graphics can be found in the bay12 forum thread:
http://www.bay12forums.com/smf/index.php?topic=43260.0

Some additional resources:
http://df.magmawiki.com/index.php/Utility:Stonesense/Content_repository

tiletypes
=========
Can be used for painting map tiles and is a interactive command, much like liquids.
You can paint tiles by their properties - shape, general material and a few others (paint).
You can also paint only over tiles that match a set of properties (filter)

For more details, see the 'help' command while using this.

tweak
=====
Contains various tweaks for minor bugs (currently just one).

Options
-------
:tweak clear-missing: Remove the missing status from the selected unit. This allows engraving slabs for ghostly, but not yet found, creatures.

tubefill
========
Fills all the adamantine veins again. Veins that were empty will be filled in too, but might still trigger a demon invasion (this is a known bug).

vampcheck
=========
Checks a single map tile or the whole map/world for cursed creatures (vampires).
With an active in-game cursor only the selected tile will be observed. Without a cursor the whole map will be checked.
By default vampires will be only counted in case you just want to find out if you have any of them running around in your fort.

Options
-------
:detail:           Print full name, date of birth, date of curse (some vampires might use fake identities in-game, though).
:nick:             Set nickname to 'CURSED' (does not always show up in-game, some vamps don't like nicknames).

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
:diag5r:           diag5 rotated 90 degrees
:ladder:           A 'ladder' pattern
:ladderr:          ladder rotated 90 degrees
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
designate the diagonal 5 patter over all hidden tiles:
  * expdig diag5 hidden
apply last used pattern and filter:
  * expdig
Take current designations and replace them with the ladder pattern:
  * expdig ladder designated

digcircle
=========
A command for easy designation of filled and hollow circles.
It has several types of options.

Shape:
--------
:hollow:   Set the circle to hollow (default)
:filled:   Set the circle to filled
:#:        Diameter in tiles (default = 0, does nothing)

Action:
-------
:set:      Set designation (default)
:unset:    Unset current designation
:invert:   Invert designations already present

Designation types:
------------------
:dig:      Normal digging designation (default)
:ramp:     Ramp digging
:ustair:   Staircase up
:dstair:   Staircase down
:xstair:   Staircase up/down
:chan:     Dig channel

After you have set the options, the command called with no options
repeats with the last selected parameters.

Examples:
---------
* 'digcircle filled 3' = Dig a filled circle with radius = 3.
* 'digcircle' = Do it again.

weather
=======
Prints the current weather map by default.

Also lets you change the current weather to 'clear sky', 'rainy' or 'snowing'.

Options:
--------
:snow:   make it snow everywhere.
:rain:   make it rain.
:clear:  clear the sky.

workflow
========
Manage control of repeat jobs.

Usage
-----
``workflow enable [option...], workflow disable [option...]``
   If no options are specified, enables or disables the plugin.
   Otherwise, enables or disables any of the following options:

   - drybuckets: Automatically empty abandoned water buckets.
   - auto-melt: Resume melt jobs when there are objects to melt.
``workflow jobs``
   List workflow-controlled jobs (if in a workshop, filtered by it).
``workflow list``
   List active constraints, and their job counts.
``workflow count <constraint-spec> <cnt-limit> [cnt-gap], workflow amount <constraint-spec> <cnt-limit> [cnt-gap]``
   Set a constraint. The first form counts each stack as only 1 item.
``workflow unlimit <constraint-spec>``
   Delete a constraint.

Function
--------
When the plugin is enabled, it protects all repeat jobs from removal.
If they do disappear due to any cause, they are immediately re-added to their
workshop and suspended.

In addition, when any constraints on item amounts are set, repeat jobs that produce
that kind of item are automatically suspended and resumed as the item amount
goes above or below the limit. The gap specifies how much below the limit
the amount has to drop before jobs are resumed; this is intended to reduce
the frequency of jobs being toggled.


Constraint examples
-------------------
Keep metal bolts within 900-1000, and wood/bone within 150-200.
::
    
    workflow amount AMMO:ITEM_AMMO_BOLTS/METAL 1000 100
    workflow amount AMMO:ITEM_AMMO_BOLTS/WOOD,BONE 200 50

Keep the number of prepared food & drink stacks between 90 and 120
::
    
    workflow count FOOD 120 30
    workflow count DRINK 120 30

Make sure there are always 25-30 empty bins/barrels/bags.
::
    
    workflow count BIN 30
    workflow count BARREL 30
    workflow count BOX/CLOTH,SILK,YARN 30

Make sure there are always 15-20 coal and 25-30 copper bars.
::
    
    workflow count BAR//COAL 20
    workflow count BAR//COPPER 30

Collect 15-20 sand bags and clay boulders.
::
    
    workflow count POWDER_MISC/SAND 20
    workflow count BOULDER/CLAY 20

Make sure there are always 80-100 units of dimple dye.
::
    
    workflow amount POWDER_MISC//MUSHROOM_CUP_DIMPLE:MILL 100 20

  In order for this to work, you have to set the material of the PLANT input on
  the Mill Plants job to MUSHROOM_CUP_DIMPLE using the 'job item-material' command.

mapexport
=========
Export the current loaded map as a file. This will be eventually usable with visualizers.

dwarfexport
===========
Export dwarves to RuneSmith-compatible XML.
