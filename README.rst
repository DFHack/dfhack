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

Currently, versions 0.34.08 - 0.34.11 are supported. If you need DFHack
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

 * On Windows, first delete SDL.dll and rename SDLreal.dll to SDL.dll. Then
   remove the other DFHack files
 * On Linux, Remove the DFHack files.

The stonesense plugin might require some additional libraries on Linux.

If any of the plugins or dfhack itself refuses to load, check the stderr.log
file created in your DF folder.

============
Using DFHack
============
DFHack basically extends what DF can do with something similar to the drop-down
console found in Quake engine games. On Windows, this is a separate command line
window. On linux, the terminal used to launch the dfhack script is taken over
(so, make sure you start from a terminal). Basic interaction with dfhack
involves entering commands into the console. For some basic instroduction,
use the 'help' command. To list all possible commands, use the 'ls' command.
Many commands have their own help or detailed description. You can use
'command help' or 'command ?' to show that.

The command line has some nice line editing capabilities, including history
that's preserved between different runs of DF (use up/down keys to go through
the history).

The second way to interact with DFHack is to bind the available commands
to in-game hotkeys. The old way to do this is via the hotkey/zoom menu (normally
opened with the 'h' key). Binding the commands is done by assigning a command as
a hotkey name (with 'n').

A new and more flexible way is the keybinding command in the dfhack console.
However, bindings created this way are not automatically remembered between runs
of the game, so it becomes necessary to use the dfhack.init file to ensure that
they are re-created every time it is loaded.

Interactive commands like 'liquids' cannot be used as hotkeys.

Most of the commands come from plugins. Those reside in 'hack/plugins/'.

=============================
Something doesn't work, help!
=============================
First, don't panic :) Second, dfhack keeps a few log files in DF's folder
- stderr.log and stdout.log. You can look at those and possibly find out what's
happening.
If you found a bug, you can either report it in the bay12 DFHack thread,
the issues tracker on github, contact me (peterix@gmail.com) or visit the
#dfhack IRC channel on freenode.

=============
The init file
=============
If your DF folder contains a file named dfhack.init, its contents will be run
every time you start DF. This allows setting up keybindings. An example file
is provided as dfhack.init-example - you can tweak it and rename to dfhack.init
if you want to use this functionality.

========
Commands
========

Almost all the commands support using the 'help <command-name>' built-in command
to retrieve further help without having to look at this document. Alternatively,
some accept a 'help'/'?' option on their command line.

adv-bodyswap
============
This allows taking control over your followers and other creatures in adventure
mode. For example, you can make them pick up new arms and armor and equip them
properly.

Usage
-----
 * When viewing unit details, body-swaps into that unit.
 * In the main adventure mode screen, reverts transient swap.

advtools
========
A package of different adventure mode tools (currently just one)
  


Usage
-----
:list-equipped [all]: List armor and weapons equipped by your companions.
                      If all is specified, also lists non-metal clothing.
:metal-detector [all-types] [non-trader]: Reveal metal armor and weapons in
                                          shops. The options disable the checks
                                          on item type and being in shop.

changelayer
===========
Changes material of the geology layer under cursor to the specified inorganic
RAW material. Can have impact on all surrounding regions, not only your embark!
By default changing stone to soil and vice versa is not allowed. By default
changes only the layer at the cursor position. Note that one layer can stretch
across lots of z levels. By default changes only the geology which is linked
to the biome under the cursor. That geology might be linked to other biomes
as well, though. Mineral veins and gem clusters will stay on the map. Use
'changevein' for them.

tl;dr: You will end up with changing quite big areas in one go, especially if
you use it in lower z levels. Use with care.

Options
-------
:all_biomes:        Change selected layer for all biomes on your map.
                    Result may be undesirable since the same layer can AND WILL
                    be on different z-levels for different biomes. Use the tool
                    'probe' to get an idea how layers and biomes are distributed
                    on your map.
:all_layers:        Change all layers on your map (only for the selected biome
                    unless 'all_biomes' is added). 
                    Candy mountain, anyone? Will make your map quite boring,
                    but tidy. 
:force:             Allow changing stone to soil and vice versa. !!THIS CAN HAVE
                    WEIRD EFFECTS, USE WITH CARE!!
                    Note that soil will not be magically replaced with stone.
                    You will, however, get a stone floor after digging so it
                    will allow the floor to be engraved.
                    Note that stone will not be magically replaced with soil.
                    You will, however, get a soil floor after digging so it
                    could be helpful for creating farm plots on maps with no
                    soil.
:verbose:           Give some details about what is being changed.
:trouble:           Give some advice about known problems.

Examples:
---------
``changelayer GRANITE``
   Convert layer at cursor position into granite.
``changelayer SILTY_CLAY force``
   Convert layer at cursor position into clay even if it's stone.
``changelayer MARBLE all_biomes all_layers``
   Convert all layers of all biomes which are not soil into marble.

.. note::

    * If you use changelayer and nothing happens, try to pause/unpause the game
      for a while and try to move the cursor to another tile. Then try again.
      If that doesn't help try temporarily changing some other layer, undo your
      changes and try again for the layer you want to change. Saving
      and reloading your map might also help.
    * You should be fine if you only change single layers without the use
      of 'force'. Still it's advisable to save your game before messing with
      the map.
    * When you force changelayer to convert soil to stone you might experience
      weird stuff (flashing tiles, tiles changed all over place etc).
      Try reverting the changes manually or even better use an older savegame.
      You did save your game, right?

changevein
==========
Changes material of the vein under cursor to the specified inorganic RAW
material.

Example:
--------
``changevein NATIVE_PLATINUM``
   Convert vein at cursor position into platinum ore.

changeitem
==========
Allows changing item material and base quality. By default the item currently
selected in the UI will be changed (you can select items in the 'k' list
or inside containers/inventory). By default change is only allowed if materials
is of the same subtype (for example wood<->wood, stone<->stone etc). But since
some transformations work pretty well and may be desired you can override this
with 'force'. Note that some attributes will not be touched, possibly resulting
in weirdness. To get an idea how the RAW id should look like, check some items
with 'info'. Using 'force' might create items which are not touched by
crafters/haulers.

Options
-------
:info:         Don't change anything, print some info instead.
:here:         Change all items at the cursor position. Requires in-game cursor.
:material, m:  Change material. Must be followed by valid material RAW id.
:quality, q:   Change base quality. Must be followed by number (0-5).
:force:        Ignore subtypes, force change to new material.

Examples:
---------
``changeitem m INORGANIC:GRANITE here``
   Change material of all items under the cursor to granite.
``changeitem q 5``
   Change currently selected item to masterpiece quality.

cursecheck
==========
Checks a single map tile or the whole map/world for cursed creatures (ghosts,
vampires, necromancers, werebeasts, zombies).

With an active in-game cursor only the selected tile will be observed.
Without a cursor the whole map will be checked.

By default cursed creatures will be only counted in case you just want to find
out if you have any of them running around in your fort. Dead and passive
creatures (ghosts who were put to rest, killed vampires, ...) are ignored.
Undead skeletons, corpses, bodyparts and the like are all thrown into the curse
category "zombie". Anonymous zombies and resurrected body parts will show
as "unnamed creature". 

Options
-------
:detail:       Print full name, date of birth, date of curse and some status
               info (some vampires might use fake identities in-game, though).
:nick:         Set the type of curse as nickname (does not always show up
               in-game, some vamps don't like nicknames).
:all:          Include dead and passive cursed creatures (can result in a quite
               long list after having FUN with necromancers).
:verbose:      Print all curse tags (if you really want to know it all).

Examples:
---------
``cursecheck detail all``
   Give detailed info about all cursed creatures including deceased ones (no
   in-game cursor).
``cursecheck nick``
  Give a nickname all living/active cursed creatures on the map(no in-game
  cursor).

.. note::

    * If you do a full search (with the option "all") former ghosts will show up
      with the cursetype "unknown" because their ghostly flag is not set
      anymore. But if you happen to find a living/active creature with cursetype
      "unknown" please report that in the dfhack thread on the modding forum or
      per irc. This is likely to happen with mods which introduce new types
      of curses, for example.

follow
======
Makes the game view follow the currently highlighted unit after you exit from
current menu/cursor mode. Handy for watching dwarves running around. Deactivated
by moving the view manually.

forcepause
==========
Forces DF to pause. This is useful when your FPS drops below 1 and you lose
control of the game.

 * Activate with 'forcepause 1'
 * Deactivate with 'forcepause 0'

nopause
=======
Disables pausing (both manual and automatic) with the exception of pause forced
by 'reveal hell'. This is nice for digging under rivers.

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
Destroy items marked for dumping under cursor. Identical to autodump
destroy-here, but intended for use as keybinding.

autodump-destroy-item
=====================
Destroy the selected item. The item may be selected in the 'k' list, or inside
a container. If called again before the game is resumed, cancels destroy.

burrow
======
Miscellaneous burrow control. Allows manipulating burrows and automated burrow
expansion while digging.

Options
-------
:enable feature ...:
:disable feature ...:    Enable or Disable features of the plugin.
:clear-unit burrow burrow ...:
:clear-tiles burrow burrow ...: Removes all units or tiles from the burrows.
:set-units target-burrow src-burrow ...:
:add-units target-burrow src-burrow ...:
:remove-units target-burrow src-burrow ...: Adds or removes units in source
       burrows to/from the target burrow. Set is equivalent to clear and add.
:set-tiles target-burrow src-burrow ...:
:add-tiles target-burrow src-burrow ...:
:remove-tiles target-burrow src-burrow ...: Adds or removes tiles in source
       burrows to/from the target burrow. In place of a source burrow it is
       possible to use one of the following keywords: ABOVE_GROUND,
       SUBTERRANEAN, INSIDE, OUTSIDE, LIGHT, DARK, HIDDEN, REVEALED

Features
--------
:auto-grow: When a wall inside a burrow with a name ending in '+' is dug
            out, the burrow is extended to newly-revealed adjacent walls.
            This final '+' may be omitted in burrow name args of commands above.
            Digging 1-wide corridors with the miner inside the burrow is SLOW.

catsplosion
===========
Makes cats just *multiply*. It is not a good idea to run this more than once or
twice.

clean
=====
Cleans all the splatter that get scattered all over the map, items and
creatures. In an old fortress, this can significantly reduce FPS lag. It can
also spoil your !!FUN!!, so think before you use it.

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
Works like 'clean map snow mud', but only for the tile under the cursor. Ideal
if you want to keep that bloody entrance 'clean map' would clean up.

cleanowned
==========
Confiscates items owned by dwarfs. By default, owned food on the floor
and rotten items are confistacted and dumped.

Options
-------
:all:          confiscate all owned items
:scattered:    confiscated and dump all items scattered on the floor
:x:            confiscate/dump items with wear level 'x' and more
:X:            confiscate/dump items with wear level 'X' and more
:dryrun:       a dry run. combine with other options to see what will happen
               without it actually happening.

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

* Discovering a magma feature (magma pool, volcano, magma sea, or curious
  underground structure) permits magma workshops and furnaces to be built.
* Discovering a cavern layer causes plants (trees, shrubs, and grass) from
  that cavern to grow within your fortress.

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
Up to version 0.31.12, Elves only sent Diplomats to your fortress to propose
tree cutting quotas due to a bug; once that bug was fixed, Elves stopped caring
about excess tree cutting. This command adds a Diplomat position to all Elven
civilizations, allowing them to negotiate tree cutting quotas (and allowing you
to violate them and potentially start wars) in case you haven't already modified
your raws accordingly.

fixmerchants
============
This command adds the Guild Representative position to all Human civilizations,
allowing them to make trade agreements (just as they did back in 0.28.181.40d
and earlier) in case you haven't already modified your raws accordingly.

fixveins
========
Removes invalid references to mineral inclusions and restores missing ones.
Use this if you broke your embark with tools like tiletypes, or if you
accidentally placed a construction on top of a valuable mineral floor.

fixwagons
=========
Due to a bug in all releases of version 0.31, merchants no longer bring wagons
with their caravans. This command re-enables them for all appropriate
civilizations.

flows
=====
A tool for checking how many tiles contain flowing liquids. If you suspect that
your magma sea leaks into HFS, you can use this tool to be sure without
revealing the map.

getplants
=========
This tool allows plant gathering and tree cutting by RAW ID. Specify the types
of trees to cut down and/or shrubs to gather by their plant names, separated
by spaces.

Options
-------
:-t: Select trees only (exclude shrubs)
:-s: Select shrubs only (exclude trees)
:-c: Clear designations instead of setting them
:-x: Apply selected action to all plants except those specified (invert
     selection)

Specifying both -t and -s will have no effect. If no plant IDs are specified,
all valid plant IDs will be listed.

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
 * no extra options - Print details of the current job. The job can be selected
   in a workshop, or the unit/jobs screen.
 * list - Print details of all jobs in the selected workshop.
 * item-material <item-idx> <material[:subtoken]> - Replace the exact material
   id in the job item.
 * item-type <item-idx> <type[:subtype]> - Replace the exact item type id in
   the job item.

job-material
============
Alter the material of the selected job.

Invoked as: job-material <inorganic-token>

Intended to be used as a keybinding:
 * In 'q' mode, when a job is highlighted within a workshop or furnace,
   changes the material of the job. Only inorganic materials can be used
   in this mode.
 * In 'b' mode, during selection of building components positions the cursor
   over the first available choice with the matching material.

job-duplicate
=============
Duplicate the selected job in a workshop:
 * In 'q' mode, when a job is highlighted within a workshop or furnace building,
   instantly duplicates the job.

keybinding
==========

Manages DFHack keybindings.

Currently it supports any combination of Ctrl/Alt/Shift with F1-F9, or A-Z.

Options
-------
:keybinding list <key>: List bindings active for the key combination.
:keybinding clear <key> <key>...: Remove bindings for the specified keys.
:keybinding add <key> "cmdline" "cmdline"...: Add bindings for the specified
                                              key.
:keybinding set <key> "cmdline" "cmdline"...: Clear, and then add bindings for
                                              the specified key.

When multiple commands are bound to the same key combination, DFHack selects
the first applicable one. Later 'add' commands, and earlier entries within one
'add' command have priority. Commands that are not specifically intended for use
as a hotkey are always considered applicable.

liquids
=======
Allows adding magma, water and obsidian to the game. It replaces the normal
dfhack command line and can't be used from a hotkey. Settings will be remembered
as long as dfhack runs. Intended for use in combination with the command
liquids-here (which can be bound to a hotkey).

For more information, refer to the command's internal help. 

.. note::

    Spawning and deleting liquids can F up pathing data and
    temperatures (creating heat traps). You've been warned.

liquids-here
============
Run the liquid spawner with the current/last settings made in liquids (if no
settings in liquids were made it paints a point of 7/7 magma by default).

Intended to be used as keybinding. Requires an active in-game cursor.

mode
====
This command lets you see and change the game mode directly.
Not all combinations are good for every situation and most of them will
produce undesirable results. There are a few good ones though.

.. admonition:: Example

     You are in fort game mode, managing your fortress and paused.
     You switch to the arena game mode, *assume control of a creature* and then
     switch to adventure game mode(1). 
     You just lost a fortress and gained an adventurer.
     You could also do this.
     You are in fort game mode, managing your fortress and paused at the esc menu.
     You switch to the adventure game mode, then use Dfusion to *assume control of a creature* and then
     save or retire. 
     You just created a returnable mountain home and gained an adventurer.


I take no responsibility of anything that happens as a result of using this tool

extirpate
=========
A tool for getting rid of trees and shrubs. By default, it only kills
a tree/shrub under the cursor. The plants are turned into ashes instantly.

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
Very similar to extirpate, but additionally sets the plants on fire. The fires
can and *will* spread ;)

probe
=====
Can be used to determine tile properties like temperature.

prospect
========
Prints a big list of all the present minerals and plants. By default, only
the visible part of the map is scanned.

Options
-------
:all:   Scan the whole map, as if it was revealed.
:value: Show material value in the output. Most useful for gems.
:hell:  Show the Z range of HFS tubes. Implies 'all'.

Pre-embark estimate
-------------------
If called during the embark selection screen, displays an estimate of layer
stone availability. If the 'all' option is specified, also estimates veins.
The estimate is computed either for 1 embark tile of the blinking biome, or
for all tiles of the embark rectangle.

Options
-------
:all:            processes all tiles, even hidden ones.

regrass
=======
Regrows grass. Not much to it ;)

rename
======
Allows renaming various things.

Options
-------
:rename squad <index> "name": Rename squad by index to 'name'.
:rename hotkey <index> \"name\": Rename hotkey by index. This allows assigning
                                 longer commands to the DF hotkeys.
:rename unit "nickname": Rename a unit/creature highlighted in the DF user
                         interface.
:rename unit-profession "custom profession": Change proffession name of the
                                             highlighted unit/creature.

reveal
======
This reveals the map. By default, HFS will remain hidden so that the demons
don't spawn. You can use 'reveal hell' to reveal everything. With hell revealed,
you won't be able to unpause until you hide the map again. If you really want
to unpause with hell revealed, use 'reveal demons'.

Reveal also works in adventure mode, but any of its effects are negated once
you move. When you use it this way, you don't need to run 'unreveal'.

unreveal
========
Reverts the effects of 'reveal'.

revtoggle
=========
Switches between 'reveal' and 'unreveal'.

revflood
========
This command will hide the whole map and then reveal all the tiles that have
a path to the in-game cursor.

revforget
=========
When you use reveal, it saves information about what was/wasn't visible before
revealing everything. Unreveal uses this information to hide things again.
This command throws away the information. For example, use in cases where
you abandoned with the fort revealed and no longer want the data.

lair
====
This command allows you to mark the map as 'monster lair', preventing item
scatter on abandon. When invoked as 'lair reset', it does the opposite.

Unlike reveal, this command doesn't save the information about tiles - you
won't be able to restore state of real monster lairs using 'lair reset'.

Options
-------
:lair: Mark the map as monster lair
:lair reset: Mark the map as ordinary (not lair)

seedwatch
=========
Tool for turning cooking of seeds and plants on/off depending on how much you
have of them.

See 'seedwatch help' for detailed description.

showmood
========
Shows all items needed for the currently active strange mood.

copystock
==========
Copies the parameters of the currently highlighted stockpile to the custom
stockpile settings and switches to custom stockpile placement mode, effectively
allowing you to copy/paste stockpiles easily.

ssense / stonesense
===================
An isometric visualizer that runs in a second window. This requires working
graphics acceleration and at least a dual core CPU (otherwise it will slow
down DF).

All the data resides in the 'stonesense' directory. For detailed instructions,
see stonesense/README.txt

Compatible with Windows > XP SP3 and most modern Linux distributions.

Older versions, support and extra graphics can be found in the bay12 forum
thread: http://www.bay12forums.com/smf/index.php?topic=43260.0

Some additional resources:
http://df.magmawiki.com/index.php/Utility:Stonesense/Content_repository

tiletypes
=========
Can be used for painting map tiles and is an interactive command, much like
liquids.

The tool works with two set of options and a brush. The brush determines which
tiles will be processed. First set of options is the filter, which can exclude
some of the tiles from the brush by looking at the tile properties. The second
set of options is the paint - this determines how the selected tiles are
changed.

Both paint and filter can have many different properties including things like
general shape (WALL, FLOOR, etc.), general material (SOIL, STONE, MINERAL,
etc.), state of 'designated', 'hidden' and 'light' flags.

The properties of filter and paint can be partially defined. This means that
you can for example do something like this:

::  

        filter material STONE
        filter shape FORTIFICATION
        paint shape FLOOR

This will turn all stone fortifications into floors, preserving the material.

Or this:
::  

        filter shape FLOOR
        filter material MINERAL
        paint shape WALL

Turning mineral vein floors back into walls.

The tool also allows tweaking some tile flags:

Or this:

::  

        paint hidden 1
        paint hidden 0

This will hide previously revealed tiles (or show hidden with the 0 option).

Any paint or filter option (or the entire paint or filter) can be disabled entirely by using the ANY keyword:

::  

        paint hidden ANY
        paint shape ANY
        filter material any
        filter shape any
        filter any

You can use several different brushes for painting tiles:
 * Point. (point)
 * Rectangular range. (range)
 * A column ranging from current cursor to the first solid tile above. (column)
 * DF map block - 16x16 tiles, in a regular grid. (block)

Example:

::  

        range 10 10 1

This will change the brush to a rectangle spanning 10x10 tiles on one z-level.
The range starts at the position of the cursor and goes to the east, south and
up.

For more details, see the 'help' command while using this.

tiletypes-commands
==================
Runs tiletypes commands, separated by ;. This makes it possible to change
tiletypes modes from a hotkey.

tiletypes-here
==============
Apply the current tiletypes options at the in-game cursor position, including
the brush. Can be used from a hotkey.

tiletypes-here-point
====================
Apply the current tiletypes options at the in-game cursor position to a single
tile. Can be used from a hotkey.

tweak
=====
Contains various tweaks for minor bugs (currently just one).

Options
-------
:clear-missing:  Remove the missing status from the selected unit.
                 This allows engraving slabs for ghostly, but not yet
                 found, creatures.
:clear-ghostly:  Remove the ghostly status from the selected unit and mark
                 it as dead. This allows getting rid of bugged ghosts
                 which do not show up in the engraving slab menu at all,
                 even after using clear-missing. It works, but is
                 potentially very dangerous - so use with care. Probably
                 (almost certainly) it does not have the same effects like
                 a proper burial. You've been warned.
:fixmigrant:     Remove the resident/merchant flag from the selected unit.
                 Intended to fix bugged migrants/traders who stay at the
                 map edge and don't enter your fort. Only works for
                 dwarves (or generally the player's race in modded games).
                 Do NOT abuse this for 'real' caravan merchants (if you
                 really want to kidnap them, use 'tweak makeown' instead,
                 otherwise they will have their clothes set to forbidden etc).
:makeown:        Force selected unit to become a member of your fort.
                 Can be abused to grab caravan merchants and escorts, even if
                 they don't belong to the player's race. Foreign sentients
                 (humans, elves) can be put to work, but you can't assign rooms
                 to them and they don't show up in DwarfTherapist because the
                 game treats them like pets. Grabbing draft animals from
                 a caravan can result in weirdness (animals go insane or berserk
                 and are not flagged as tame), but you are allowed to mark them
                 for slaughter. Grabbing wagons results in some funny spam, then
                 they are scuttled.

tubefill
========
Fills all the adamantine veins again. Veins that were empty will be filled in
too, but might still trigger a demon invasion (this is a known bug).

digv
====
Designates a whole vein for digging. Requires an active in-game cursor placed
over a vein tile. With the 'x' option, it will traverse z-levels (putting stairs
between the same-material tiles).

digvx
=====
A permanent alias for 'digv x'.

digl
====
Designates layer stone for digging. Requires an active in-game cursor placed
over a layer stone tile. With the 'x' option, it will traverse z-levels
(putting stairs between the same-material tiles). With the 'undo' option it
will remove the dig designation instead (if you realize that digging out a 50
z-level deep layer was not such a good idea after all).

diglx
=====
A permanent alias for 'digl x'.

digexp
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

In addition, when any constraints on item amounts are set, repeat jobs that
produce that kind of item are automatically suspended and resumed as the item
amount goes above or below the limit. The gap specifies how much below the limit
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

  In order for this to work, you have to set the material of the PLANT input
  on the Mill Plants job to MUSHROOM_CUP_DIMPLE using the 'job item-material'
  command.

mapexport
=========
Export the current loaded map as a file. This will be eventually usable
with visualizers.

dwarfexport
===========
Export dwarves to RuneSmith-compatible XML.

zone
====
Helps a bit with managing activity zones (pens, pastures and pits) and cages.

Options:
--------
:set:          Set zone or cage under cursor as default for future assigns.
:assign:       Assign unit(s) to the pen or pit marked with the 'set' command.
               If no filters are set a unit must be selected in the in-game ui.
               Can also be followed by a valid zone id which will be set
               instead.
:unassign:     Unassign selected creature from it's zone.
:nick:         Mass-assign nicknames, must be followed by the name you want
               to set.
:remnick:      Mass-remove nicknames.
:tocages:      Assign unit(s) to cages inside a pasture.
:uinfo:        Print info about unit(s). If no filters are set a unit must
               be selected in the in-game ui.
:zinfo:        Print info about zone(s). If no filters are set zones under
               the cursor are listed.
:verbose:      Print some more info.
:filters:      Print list of valid filter options.
:examples:     Print some usage examples.
:not:          Negates the next filter keyword.

Filters:
--------
:all:           Process all units (to be used with additional filters).
:count:         Must be followed by a number. Process only n units (to be used
                with additional filters). 
:unassigned:    Not assigned to zone, chain or built cage.
:minage:        Minimum age. Must be followed by number.
:maxage:        Maximum age. Must be followed by number.
:race:          Must be followed by a race RAW ID (e.g. BIRD_TURKEY, ALPACA,
                etc). Negatable.
:caged:         In a built cage. Negatable.
:own:           From own civilization. Negatable.
:merchant:      Is a merchant / belongs to a merchant. Should only be used for
                pitting, not for stealing animals (slaughter should work).
:war:           Trained war creature. Negatable.
:hunting:       Trained hunting creature. Negatable.
:tamed:         Creature is tame. Negatable.
:trained:       Creature is trained. Finds war/hunting creatures as well as
                creatures who have a training level greater than 'domesticated'.
                If you want to specifically search for war/hunting creatures use
                'war' or 'hunting' Negatable.
:trainablewar:  Creature can be trained for war (and is not already trained for
                war/hunt). Negatable.
:trainablehunt: Creature can be trained for hunting (and is not already trained
                for war/hunt). Negatable.
:male:          Creature is male. Negatable.
:female:        Creature is female. Negatable.
:egglayer:      Race lays eggs. Negatable.
:grazer:        Race is a grazer. Negatable.
:milkable:      Race is milkable. Negatable.

Usage with single units
-----------------------
One convenient way to use the zone tool is to bind the command 'zone assign' to
a hotkey, maybe also the command 'zone set'. Place the in-game cursor over
a pen/pasture or pit, use 'zone set' to mark it. Then you can select units
on the map (in 'v' or 'k' mode), in the unit list or from inside cages
and use 'zone assign' to assign them to their new home. Allows pitting your
own dwarves, by the way.

Usage with filters
------------------
All filters can be used together with the 'assign' command.

Restrictions: It's not possible to assign units who are inside built cages
or chained because in most cases that won't be desirable anyways.
It's not possible to cage owned pets because in that case the owner
uncages them after a while which results in infinite hauling back and forth.

Usually you should always use the filter 'own' (which implies tame) unless you
want to use the zone tool for pitting hostiles. 'own' ignores own dwarves unless
you specify 'race DWARF' (so it's safe to use 'assign all own' to one big
pasture if you want to have all your animals at the same place). 'egglayer' and
'milkable' should be used together with 'female' unless you have a mod with
egg-laying male elves who give milk or whatever. Merchants and their animals are
ignored unless you specify 'merchant' (pitting them should be no problem,
but stealing and pasturing their animals is not a good idea since currently they
are not properly added to your own stocks; slaughtering them should work).

Most filters can be negated (e.g. 'not grazer' -> race is not a grazer).

Mass-renaming
-------------
Using the 'nick' command you can set the same nickname for multiple units.
If used without 'assign', 'all' or 'count' it will rename all units in the
current default target zone. Combined with 'assign', 'all' or 'count' (and
further optional filters) it will rename units matching the filter conditions. 

Cage zones
----------
Using the 'tocages' command you can assign units to a set of cages, for example
a room next to your butcher shop(s). They will be spread evenly among available
cages to optimize hauling to and butchering from them. For this to work you need
to build cages and then place one pen/pasture activity zone above them, covering
all cages you want to use. Then use 'zone set' (like with 'assign') and use
'zone tocages filter1 filter2 ...'. 'tocages' overwrites 'assign' because it
would make no sense, but can be used together with 'nick' or 'remnick' and all
the usual filters.

Examples
--------
``zone assign all own ALPACA minage 3 maxage 10``
   Assign all own alpacas who are between 3 and 10 years old to the selected
   pasture.
``zone assign all own caged grazer nick ineedgrass``
   Assign all own grazers who are sitting in cages on stockpiles (e.g. after
   buying them from merchants) to the selected pasture and give them
   the nickname 'ineedgrass'.
``zone assign all own not grazer not race CAT``
   Assign all own animals who are not grazers, excluding cats.
``zone assign count 5 own female milkable``
   Assign up to 5 own female milkable creatures to the selected pasture.
``zone assign all own race DWARF maxage 2``
   Throw all useless kids into a pit :)
``zone nick donttouchme``
   Nicknames all units in the current default zone or cage to 'donttouchme'.
   Mostly intended to be used for special pastures or cages which are not marked
   as rooms you want to protect from autobutcher.
``zone tocages count 50 own tame male not grazer``
   Stuff up to 50 owned tame male animals who are not grazers into cages built
   on the current default zone.
   
autonestbox
===========
Assigns unpastured female egg-layers to nestbox zones. Requires that you create
pen/pasture zones above nestboxes. If the pen is bigger than 1x1 the nestbox
must be in the top left corner. Only 1 unit will be assigned per pen, regardless
of the size. The age of the units is currently not checked, most birds grow up
quite fast. Egglayers who are also grazers will be ignored, since confining them
to a 1x1 pasture is not a good idea. Only tame and domesticated own units are
processed since pasturing half-trained wild egglayers could destroy your neat
nestbox zones when they revert to wild. When called without options autonestbox
will instantly run once.

Options:
--------
:start:        Start running every X frames (df simulation ticks).
               Default: X=6000, which would be every 60 seconds at 100fps.
:stop:         Stop running automatically.
:sleep:        Must be followed by number X. Changes the timer to sleep X
               frames between runs.

autobutcher
===========
Assigns lifestock for slaughter once it reaches a specific count. Requires that
you add the target race(s) to a watch list. Only tame units will be processed.

Named units will be completely ignored (to protect specific animals from
autobutcher you can give them nicknames with the tool 'rename unit' for single
units or with 'zone nick' to mass-rename units in pastures and cages).

Creatures trained for war or hunting will be ignored as well.

Creatures assigned to cages will be ignored if the cage is defined as a room
(to avoid butchering unnamed zoo animals).

Once you have too much adults, the oldest will be butchered first.
Once you have too much kids, the youngest will be butchered first.
If you don't set any target count the following default will be used:
1 male kid, 5 female kids, 1 male adult, 5 female adults.

Options:
--------
:start:        Start running every X frames (df simulation ticks).
               Default: X=6000, which would be every 60 seconds at 100fps.
:stop:         Stop running automatically.
:sleep:        Must be followed by number X. Changes the timer to sleep
               X frames between runs.
:watch R:      Start watching a race. R can be a valid race RAW id (ALPACA,
               BIRD_TURKEY, etc) or a list of ids seperated by spaces or
               the keyword 'all' which affects all races on your current
               watchlist.
:unwatch R:    Stop watching race(s). The current target settings will be
               remembered. R can be a list of ids or the keyword 'all'.
:forget R:     Stop watching race(s) and forget it's/their target settings.
               R can be a list of ids or the keyword 'all'.
:autowatch:    Automatically adds all new races (animals you buy from merchants,
               tame yourself or get from migrants) to the watch list using
               default target count. 
:noautowatch:  Stop auto-adding new races to the watchlist.
:list:         Print the current status and watchlist.
:list_export:  Print status and watchlist in a format which can be used
               to import them to another savegame (see notes).
:target fk mk fa ma R: Set target count for specified race(s).
                 fk = number of female kids,
                 mk = number of male kids,
                 fa = number of female adults,
                 ma = number of female adults.
                 R can be a list of ids or the keyword 'all' or 'new'.
                 R = 'all': change target count for all races on watchlist
                 and set the new default for the future. R = 'new': don't touch
                 current settings on the watchlist, only set the new default
                 for future entries.
:example:      Print some usage examples.

Examples:
---------
You want to keep max 7 kids (4 female, 3 male) and max 3 adults (2 female,
1 male) of the race alpaca. Once the kids grow up the oldest adults will get
slaughtered. Excess kids will get slaughtered starting with the youngest
to allow that the older ones grow into adults. Any unnamed cats will
be slaughtered as soon as possible.
::  

     autobutcher target 4 3 2 1 ALPACA BIRD_TURKEY
     autobutcher target 0 0 0 0 CAT
     autobutcher watch ALPACA BIRD_TURKEY CAT
     autobutcher start
    
Automatically put all new races onto the watchlist and mark unnamed tame units
for slaughter as soon as they arrive in your fort. Settings already made
for specific races will be left untouched.
::  

     autobutcher target 0 0 0 0 new
     autobutcher autowatch
     autobutcher start

Stop watching the races alpaca and cat, but remember the target count
settings so that you can use 'unwatch' without the need to enter the
values again. Note: 'autobutcher unwatch all' works, but only makes sense
if you want to keep the plugin running with the 'autowatch' feature or manually
add some new races with 'watch'. If you simply want to stop it completely use
'autobutcher stop' instead.
::  

     autobutcher unwatch ALPACA CAT
    
Note:
-----
Settings and watchlist are stored in the savegame, so that you can have
different settings for each world. If you want to copy your watchlist to
another savegame you can use the command list_export:
::  

     Load savegame where you made the settings.
     Start a CMD shell and navigate to the df directory. Type the following into the shell:
     dfhack-run autobutcher list_export > autobutcher.bat
     Load the savegame where you want to copy the settings to, run the batch file (from the shell):
     autobutcher.bat


autolabor
=========
Automatically manage dwarf labors.

When enabled, autolabor periodically checks your dwarves and enables or
disables labors. It tries to keep as many dwarves as possible busy but
also tries to have dwarves specialize in specific skills.

.. note::

    Warning: autolabor will override any manual changes you make to labors
    while it is enabled.

For detailed usage information, see 'help autolabor'.


growcrops
=========
Instantly grow seeds inside farming plots.

With no argument, this command list the various seed types currently in
use in your farming plots.
With a seed type, the script will grow 100 of these seeds, ready to be
harvested. You can change the number with a 2nd argument.

For exemple, to grow 40 plump helmet spawn:
:: 

    growcrops plump 40


removebadthoughts
=================
This script remove negative thoughts from your dwarves. Very useful against
tantrum spirals.

With a selected unit in 'v' mode, will clear this unit's mind, otherwise
clear all your fort's units minds.

Individual dwarf happiness may not increase right after this command is run,
but in the short term your dwarves will get much more joyful.
The thoughts are set to be very old, and the game will remove them soon when
you unpause.

With the optional ``-v`` parameter, the script will dump the negative thoughts
it removed.


slayrace
========
Kills any unit of a given race.

With no argument, lists the available races.

With the special argument 'him', targets only the selected creature.

Any non-dead non-caged unit of the specified race gets its ``blood_count``
set to 0, which means immediate death at the next game tick. For creatures
such as vampires, also set animal.vanish_countdown to 2.

An alternate mode is selected by adding a 2nd argument to the command,
``magma``. In this case, a column of 7/7 magma is generated on top of the
targets until they die (Warning: do not call on magma-safe creatures. Also,
using this mode for birds is not recommanded.)

Will target any unit on a revealed tile of the map, including ambushers.

Ex:
:: 
    slayrace gob

To kill a single creature, select the unit with the 'v' cursor and:
:: 
    slayrace him

To purify all elves on the map with fire (may have side-effects):
:: 
    slayrace elve magma


magmasource
===========
Create an infinite magma source on a tile.

This script registers a map tile as a magma source, and every 12 game ticks
that tile receives 1 new unit of flowing magma.

Place the game cursor where you want to create the source (must be a
flow-passable tile, and not too high in the sky) and call
:: 
    magmasource here

To add more than 1 unit everytime, call the command again.

To delete one source, place the cursor over its tile and use ``delete-here``.
To remove all placed sources, call ``magmasource stop``.

With no argument, this command shows an help message and list existing sources.

