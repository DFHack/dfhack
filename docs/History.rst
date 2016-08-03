:orphan:

.. _HISTORY:

########################
HISTORY - old changelogs
########################

This file is where old changelogs live, so the current `changelog`
in ``NEWS.rst`` doesn't get too long.

.. contents::
   :depth: 2


DFHack 0.40.23-r1
=================

Internals
- plugins will not be loaded if globals they specify as required are not located (should prevent some crashes)

Fixes
-----
- Fixed numerous (mostly Lua-related) crashes on OS X by including a more up-to-date libstdc++
- :kbd:`Alt` should no longer get stuck on Windows (and perhaps other platforms as well)
- `gui/advfort` works again
- `autobutcher`: takes sexualities into account
- devel/export-dt-ini: Updated for 0.40.20+
- `digfort`: now checks file type and existence
- `exportlegends`: Fixed map export
- `full-heal`: Fixed a problem with selecting units in the GUI
- `gui/hack-wish`: Fixed restrictive material filters
- `mousequery`: Changed box-select key to Alt+M
- `dwarfmonitor`: correct date display (month index, separator)
- `putontable`: added to the readme
- `siren` should work again
- stderr.log: removed excessive debug output on OS X
- `trackstop`: No longer prevents cancelling the removal of a track stop or roller.
- Fixed a display issue with ``PRINT_MODE:TEXT``
- Fixed a symbol error (MapExtras::BiomeInfo::MAX_LAYERS) when compiling DFHack in Debug mode

New Plugins
-----------
- `fortplan`: designate construction of (limited) buildings from .csv file, quickfort-style

New Scripts
-----------
- `gui/stockpiles`: an in-game interface for saving and loading stockpile settings files.
- `position`: Reports the current date, time, month, and season, plus some location info.  Port/update of position.py
- `hfs-pit`: Digs a hole to hell under the cursor.  Replaces needs_porting/hellhole.cpp

Removed
-------
- embark.lua: Obsolete, use `embark-tools`

New tweaks
----------
- `eggs-fertile <tweak>`: Displays an egg fertility indicator on nestboxes
- `max-wheelbarrow <tweak>`: Allows assigning more than 3 wheelbarrows to a stockpile

Misc Improvements
-----------------
- `embark-tools`: Added basic mouse support on the local map
- Made some adventure mode keybindings in :file:`dfhack.init-example` only work in adventure mode
- `gui/companion-order`: added a default keybinding
- further work on needs_porting


DFHack 0.40.19-r1
=================

Fixes
-----
- `modtools/reaction-trigger`: fixed typo
- `modtools/item-trigger`: should now work with item types

New plugins
-----------
- `savestock, loadstock <stocksettings>`: save and load stockpile settings across worlds and saves

New scripts
-----------
- `remove-stress`: set selected or all units unit to -1,000,000 stress (this script replaces removebadthoughts)

Misc improvements
-----------------
- `command-prompt`: can now access selected items, units, and buildings
- `autolabor`: add an optional talent pool parameter


DFHack 0.40.16-r1
=================

Internals
---------
- `EventManager` should handle INTERACTION triggers a little better. It still can get confused about who did what but only rarely.
- `EventManager` should no longer trigger REPORT events for old reports after loading a save.
- lua/persist-table: a convenient way of using persistent tables of arbitrary structure and dimension in Lua

Fixes
-----
- `mousequery`: Disabled when linking levers
- `stocks`: Melting should work now
- `full-heal`: Updated with proper argument handling
- `modtools/reaction-trigger-transition`: should produce the correct syntax now
- `superdwarf`: should work better now
- `forum-dwarves`: update for new df-structures changes

New Scripts
-----------
- `adaptation`: view or set the cavern adaptation level of your citizens
- `add-thought`: allows the user to add thoughts to creatures.
- `gaydar`: detect the sexual orientation of units on the map
- `markdown`: Save a copy of a text screen in markdown (for reddit among others).
- devel/all-bob: renames everyone Bob to help test interaction-trigger

Misc Improvements
-----------------
- `autodump`: Can now mark a stockpile for auto-dumping (similar to `automelt` and `autotrade`)
- `buildingplan`: Can now auto-allocate rooms to dwarves with specific positions (e.g. expedition leader, mayor)
- `dwarfmonitor`: now displays a weather indicator and date
- lua/syndrome-util, `modtools/add-syndrome`: now you can remove syndromes by SYN_CLASS
- No longer write empty :file:`.history` files


DFHack 0.40.15-r1
=================

Fixes
-----
- mousequery: Fixed behavior when selecting a tile on the lowest z-level

Misc Improvements
-----------------
- `EventManager`: deals with frame_counter getting reset properly now.
- `modtools/item-trigger`: fixed equip/unequip bug and corrected minor documentation error
- `teleport`: Updated with proper argument handling and proper unit-at-destination handling.
- `autotrade`: Removed the newly obsolete :guilabel:`Mark all` functionality.
- `search-plugin`: Adapts to the new trade screen column width
- `tweak fast-trade <tweak>`: Switching the fast-trade keybinding to Shift-Up/Shift-Down, due to Select All conflict


DFHack 0.40.14-r1
=================

Internals
---------
- The DFHack console can now be disabled by setting the DFHACK_DISABLE_CONSOLE environment variable: ``DFHACK_DISABLE_CONSOLE=1 ./dfhack``

Fixes
-----
- Stopped duplicate load/unload events when unloading a world
- Stopped ``-e`` from being echoed when DFHack quits on Linux
- `automelt`: now uses a faster method to locate items
- `autotrade`: "Mark all" no longer double-marks bin contents
- `drain-aquifer`: new script replaces the buggy plugin
- `embark-tools`: no longer conflicts with keys on the notes screen
- `fastdwarf`: Fixed problems with combat/attacks
- `forum-dwarves`: should work now
- `manipulator`: now uses a stable sort, allowing sorting by multiple categories
- `rendermax`: updated to work with 0.40

New Plugins
-----------
- `trackstop`: Shows track stop friction and dump direction in its :kbd:`q` menu

New Tweaks
----------
- farm-plot-select: Adds "Select all" and "Deselect all" options to farm plot menus
- import-priority-category: Allows changing the priority of all goods in a category when discussing an import agreement with the liaison
- manager-quantity: Removes the limit of 30 jobs per manager order
- civ-view-agreement: Fixes overlapping text on the "view agreement" screen
- nestbox-color: Fixes the color of built nestboxes

Misc Improvements
-----------------
- `exportlegends`: can now handle site maps


DFHack 0.40.13-r1
=================

Internals
---------
- unified spatter structs
- added ruby df.print_color(color, string) method for dfhack console

Fixes
-----
- no more ``-e`` after terminating
- fixed `superdwarf`


DFHack 0.40.12-r1
=================

Internals
---------
- support for global `onLoad.init` and `onUnload.init` files, called when loading and unloading a world
- Close file after loading a `binary patch <binpatches>`.

New Plugins
-----------
- `hotkeys`: Shows ingame viewscreen with all dfhack keybindings active in current mode.
- `automelt`: allows marking stockpiles so any items placed in them will be designated for melting

Fixes
-----
- possible crash fixed for `gui/hack-wish`
- `search-plugin`: updated to not conflict with BUILDJOB_SUSPEND
- `workflow`: job_material_category -> dfhack_material_category

Misc Improvements
-----------------
- now you can use ``@`` to print things in interactive Lua with subtley different semantics
- optimizations for stockpiles for `autotrade` and `stockflow`
- updated `exportlegends` to work with new maps, dfhack 40.11 r1+


DFHack 0.40.11-r1
=================

Internals
- Plugins on OS X now use ``.plug.dylib` as an extension instead of ``.plug.so``

Fixes
-----
- `3dveins`: should no longer hang/crash on specific maps
- `autotrade`, `search-plugin`: fixed some layout issues
- `deathcause`: updated
- `gui/hack-wish`: should work now
- `reveal`: no longer allocates data for nonexistent map blocks
- Various documentation fixes and updates


DFHack v0.40.10-r1
==================

A few bugfixes.

DFHack v0.40.08-r2
==================

Internals
---------
- supported per save script folders
- Items module: added createItem function
- Sorted CMakeList for plugins and plugins/devel
- `diggingInvaders` no longer builds if plugin building is disabled
- `EventManager`: EQUIPMENT_CHANGE now triggers for new units.  New events::

            ON_REPORT
            UNIT_ATTACK
            UNLOAD
            INTERACTION

New Scripts
-----------
- lua/repeat-util: makes it easier to make things repeat indefinitely
- lua/syndrome-util: makes it easier to deal with unit syndromes
- `forum-dwarves`: helps copy df viewscreens to a file
- `full-heal`: fully heal a unit
- `remove-wear`: removes wear from all items in the fort
- `repeat`: repeatedly calls a script or a plugin
- ShowUnitSyndromes: shows syndromes affecting units and other relevant info
- `teleport`: teleports units
- `devel/print-args`
- `fix/blood-del`: makes it so civs don't bring barrels full of blood ichor or goo
- `fix/feeding-timers`: reset the feeding timers of all units
- `gui/hack-wish`: creates items out of any material
- `gui/unit-info-viewer`: displays information about units
- `modtools/add-syndrome`: add a syndrome to a unit or remove one
- `modtools/anonymous-script`: execute an lua script defined by a string. Useful for the ``*-trigger`` scripts.
- `modtools/force`: forces events: caravan, migrants, diplomat, megabeast, curiousbeast, mischievousbeast, flier, siege, nightcreature
- `modtools/item-trigger`: triggers commands based on equipping, unequipping, and wounding units with items
- `modtools/interaction-trigger`: triggers commands when interactions happen
- `modtools/invader-item-destroyer`: destroys invaders' items when they die
- `modtools/moddable-gods`: standardized version of Putnam's moddable gods script
- `modtools/projectile-trigger`: standardized version of projectileExpansion
- `modtools/reaction-trigger`: trigger commands when custom reactions complete; replaces autoSyndrome
- `modtools/reaction-trigger-transition`: a tool for converting mods from autoSyndrome to reaction-trigger
- `modtools/random-trigger`: triggers random scripts that you register
- `modtools/skill-change`: for incrementing and setting skills
- `modtools/spawn-flow`: creates flows, like mist or dragonfire
- `modtools/syndrome-trigger`: trigger commands when syndromes happen
- `modtools/transform-unit`: shapeshifts a unit, possibly permanently

Misc improvements
-----------------
- new function in utils.lua for standardized argument processing

Removed
-------
- digmat.rb: digFlood does the same functionality with less FPS impact
- invasionNow: `modtools/force` does it better
- autoSyndrome replaced with `modtools/reaction-trigger`
- syndromeTrigger replaced with `modtools/syndrome-trigger`
- devel/printArgs plugin converted to `devel/print-args`
- outsideOnly plugin replaced by `modtools/outside-only`


DFHack v0.40.08-r1
==================

Was a mistake. Don't use it.

DFHack v0.34.11-r5
==================

Internals
---------
- support for calling a lua function via a protobuf request (demonstrated by dfhack-run --lua).
- support for basic filesystem operations (e.g. chdir, mkdir, rmdir, stat) in C++ and Lua
- Lua API for listing files in directory. Needed for `gui/mod-manager`
- Lua API for creating unit combat reports and writing to gamelog.
- Lua API for running arbitrary DFHack commands
- support for multiple ``raw/init.d/*.lua`` init scripts in one save.
- eventful now has a more friendly way of making custom sidebars
- on Linux and OS X the console now supports moving the cursor back and forward by a whole word.

New scripts
-----------
- `gui/mod-manager`: allows installing/uninstalling mods into df from ``df/mods`` directory.
- `gui/clone-uniform`: duplicates the currently selected uniform in the military screen.
- `fix/build-location`: partial work-around for :bug:`5991` (trying to build wall while standing on it)
- `undump-buildings`: removes dump designation from materials used in buildings.
- `exportlegends`: exports data from legends mode, allowing a set-and-forget export of large worlds.
- log-region: each time a fort is loaded identifying information will be written to the gamelog.
- `dfstatus <gui/dfstatus>`: show an overview of critical stock quantities, including food, drinks, wood, and bars.
- `command-prompt`: a dfhack command prompt in df.

New plugins
-----------
- `rendermax`: replace the renderer with something else, eg ``rendermax light``- a lighting engine
- `automelt`: allows marking stockpiles for automelt (i.e. any items placed in stocpile will be designated for melting)
- `embark-tools`: implementations of Embark Anywhere, Nano Embark, and a few other embark-related utilities
- `building-hacks`: Allows to add custom functionality and/or animations to buildings.
- `petcapRemover`: triggers pregnancies in creatures so that you can effectively raise the default pet population cap
- `plant create <plant>`: spawn a new shrub under the cursor

New tweaks
----------
- craft-age-wear: make crafted items wear out with time like in old versions (:bug:`6003`)
- adamantine-cloth-wear: stop adamantine clothing from wearing out (:bug:`6481`)
- confirm-embark: adds a prompt before embarking (on the "prepare carefully" screen)

Misc improvements
-----------------
- `plant`: move the 'grow', 'extirpate' and 'immolate' commands as 'plant' subcommands
- `digfort`: improved csv parsing, add start() comment handling
- `exterminate`: allow specifying a caste (exterminate gob:male)
- `createitem`: in adventure mode it now defaults to the controlled unit as maker.
- `autotrade`: adds "(Un)mark All" options to both panes of trade screen.
- `mousequery`: several usability improvements; show live overlay (in menu area) of what's on the tile under the mouse cursor.
- `search`: workshop profile search added.
- `dwarfmonitor`: add screen to summarise preferences of fortress dwarfs.
- `getplants`: add autochop function to automate woodcutting.
- `stocks`: added more filtering and display options.

- `siege-engine`:

    - engine quality and distance to target now affect accuracy
    - firing the siege engine at a target produces a combat report
    - improved movement speed computation for meandering units
    - operators in Prepare To Fire mode are released from duty once hungry/thirsty if there is a free replacement


DFHack v0.34.11-r4
==================

New commands
------------
- `diggingInvaders` - allows invaders to dig and/or deconstruct walls and buildings in order to get at your dwarves.
- `digFlood` - automatically dig out specified veins as they are revealed
- `enable, disable <enable>` - Built-in commands that can be used to enable/disable many plugins.
- `restrictice` - Restrict traffic on squares above visible ice.
- `restrictliquids` - Restrict traffic on every visible square with liquid.
- treefarm - automatically chop trees and dig obsidian

New Scripts
-----------
- `autobutcher`: A GUI front-end for the autobutcher plugin.
- invasionNow: trigger an invasion, or many
- `locate-ore`: scan the map for unmined ore veins
- `masspit`: designate caged creatures in a zone for pitting
- `multicmd`: run a sequence of dfhack commands, separated by ';'
- `startdwarf`: change the number of dwarves for a new embark
- digmat: dig veins/layers tile by tile, as discovered

Misc improvements
-----------------
- autoSyndrome:

    - disable by default
    - reorganized special tags
    - minimized error spam
    - reset policies: if the target already has an instance of the syndrome you can skip,
      add another instance, reset the timer, or add the full duration to the time remaining

- core: fix SC_WORLD_(UN)LOADED event for arena mode
- `exterminate`: renamed from slayrace, add help message, add butcher mode
- `fastdwarf`: fixed bug involving fastdwarf and teledwarf being on at the same time
- magmasource: rename to `source`, allow water/magma sources/drains
- Add df.dfhack_run "somecommand" to Ruby
- syndromeTrigger: replaces and extends trueTransformation. Can trigger things when syndromes are added for any reason.
- `tiletypes`: support changing tile material to arbitrary stone.
- `workNow`: can optionally look for jobs when jobs are completed

New tweaks
----------
- hive-crash: Prevent crash if bees die in a hive with ungathered products (:bug:`6368`).

New plugins
-----------
- `3dveins`: Reshapes all veins on the map in a way that flows between Z levels. May be unstable. Backup before using.
- `autotrade`: Automatically send items in marked stockpiles to trade depot, when trading is possible.
- `buildingplan`: Place furniture before it's built
- `dwarfmonitor`: Records dwarf activity to measure fort efficiency
- `mousequery`: Look and poke at the map elements with the mouse.
- outsideOnly: make raw-specified buildings impossible to build inside
- `resume`: A plugin to help display and resume suspended constructions conveniently
- `stocks`: An improved stocks display screen.

Internals
---------
- Core: there is now a per-save dfhack.init file for when the save is loaded, and another for when it is unloaded
- EventManager: fixed job completion detection, fixed removal of TICK events, added EQUIPMENT_CHANGE event
- Lua API for a better `random number generator <lua_api_random>` and perlin noise functions.
- Once: easy way to make sure something happens once per run of DF, such as an error message


DFHack v0.34.11-r3
==================

Internals
---------
- support for displaying active keybindings properly.
- support for reusable widgets in lua screen library.
- Maps::canStepBetween: returns whether you can walk between two tiles in one step.
- EventManager: monitors various in game events centrally so that individual plugins
  don't have to monitor the same things redundantly.
- Now works with OS X 10.6.8

Notable bugfixes
----------------
- `autobutcher` can be re-enabled again after being stopped.
- stopped `Dwarf Manipulator <manipulator>` from unmasking vampires.
- `stonesense` is now fixed on OS X

Misc improvements
-----------------
- `fastdwarf`: new mode using debug flags, and some internal consistency fixes.
- added a small stand-alone utility for applying and removing `binary patches <binpatches>`.
- removebadthoughts: add --dry-run option
- `superdwarf`: work in adventure mode too
- `tweak` stable-cursor: carries cursor location from/to Build menu.
- `deathcause`: allow selection from the unitlist screen
- slayrace: allow targetting undeads
- `workflow` plugin:

    - properly considers minecarts assigned to routes busy.
    - code for deducing job outputs rewritten in lua for flexibility.
    - logic fix: collecting webs produces silk, and ungathered webs are not thread.
    - items assigned to squads are considered busy, even if not in inventory.
    - shearing and milking jobs are supported, but only with generic MILK or YARN outputs.
    - workflow announces when the stock level gets very low once a season.

- Auto syndrome plugin: A way of automatically applying boiling rock syndromes and calling dfhack commands controlled by raws.
- `infiniteSky` plugin: Create new z-levels automatically or on request.
- True transformation plugin: A better way of doing permanent transformations that allows later transformations.
- `workNow` plugin: Makes the game assign jobs every time you pause.

New tweaks
----------
- tweak military-training: speed up melee squad training up to 10x (normally 3-5x).

New scripts
-----------
- `binpatch`: the same as the stand-alone binpatch.exe, but works at runtime.
- region-pops: displays animal populations of the region and allows tweaking them.
- `lua`: lua interpreter front-end converted to a script from a native command.
- dfusion: misc scripts with a text based menu.
- embark: lets you embark anywhere.
- `lever`: list and pull fort levers from the dfhack console.
- `stripcaged`: mark items inside cages for dumping, eg caged goblin weapons.
- soundsense-season: writes the correct season to gamelog.txt on world load.
- create-items: spawn items
- fix/cloth-stockpile: fixes :bug:`5739`; needs to be run after savegame load every time.

New GUI scripts
---------------
- `gui/guide-path`: displays the cached path for minecart Guide orders.
- `gui/workshop-job`: displays inputs of a workshop job and allows tweaking them.
- `gui/workflow`: a front-end for the workflow plugin (part inspired by falconne).
- `gui/assign-rack`: works together with a binary patch to fix weapon racks.
- `gui/gm-editor`: an universal editor for lots of dfhack things.
- `gui/companion-order`: a adventure mode command interface for your companions.
- `gui/advfort`: a way to do jobs with your adventurer (e.g. build fort).

New binary patches
------------------
(for use with `binpatch`)

- armorstand-capacity: doubles the capacity of armor stands.
- custom-reagent-size: lets custom reactions use small amounts of inputs.
- deconstruct-heapfall: stops some items still falling on head when deconstructing.
- deconstruct-teleport: stops items from 16x16 block teleporting when deconstructing.
- hospital-overstocking: stops hospital overstocking with supplies.
- training-ammo: lets dwarves with quiver full of combat-only ammo train.
- weaponrack-unassign: fixes bug that negates work done by gui/assign-rack.

New Plugins
-----------
- `fix-armory`: Together with a couple of binary patches and the `gui/assign-rack` script, this plugin makes weapon racks, armor stands, chests and cabinets in properly designated barracks be used again for storage of squad equipment.
- `search`: Adds an incremental search function to the Stocks, Trading, Stockpile and Unit List screens.
- `automaterial`: Makes building constructions (walls, floors, fortifications, etc) a little bit easier by saving you from having to trawl through long lists of materials each time you place one.
- Dfusion: Reworked to make use of lua modules, now all the scripts can be used from other scripts.
- Eventful: A collection of lua events, that will allow new ways to interact with df world.

DFHack v0.34.11-r2
==================

Internals
---------
- full support for Mac OS X.
- a plugin that adds scripting in `ruby <rb>`.
- support for interposing virtual methods in DF from C++ plugins.
- support for creating new interface screens from C++ and lua.
- added various other API functions.

Notable bugfixes
----------------
- better terminal reset after exit on linux.
- `seedwatch` now works on reclaim.
- the sort plugin won't crash on cages anymore.

Misc improvements
-----------------
- `autodump`: can move items to any walkable tile, not just floors.
- `stripcaged`: by default keep armor, new dumparmor option.
- `zone`: allow non-domesticated birds in nestboxes.
- `workflow`: quality range in constraints.
- cleanplants: new command to remove rain water from plants.
- `liquids`: can paint permaflow, i.e. what makes rivers power water wheels.
- `prospect`: pre-embark prospector accounts for caves & magma sea in its estimate.
- `rename`: supports renaming stockpiles, workshops, traps, siege engines.
- `fastdwarf`: now has an additional option to make dwarves teleport to their destination.
- `autolabor`:

    - can set nonidle hauler percentage.
    - broker excluded from all labors when needed at depot.
    - likewise, anybody with a scheduled diplomat meeting.

New commands
------------
- misery: multiplies every negative thought gained (2x by default).
- `digtype`: designates every tile of the same type of vein on the map for 'digging' (any dig designation).

New tweaks
----------
- tweak stable-cursor: keeps exact cursor position between d/k/t/q/v etc menus.
- tweak patrol-duty: makes Train orders reduce patrol timer, like the binary patch does.
- tweak readable-build-plate: fix unreadable truncation in unit pressure plate build ui.
- tweak stable-temp: fixes bug 6012; may improve FPS by 50-100% on a slow item-heavy fort.
- tweak fast-heat: speeds up item heating & cooling, thus making stable-temp act faster.
- tweak fix-dimensions: fixes subtracting small amounts from stacked liquids etc.
- tweak advmode-contained: fixes UI bug in custom reactions with container inputs in advmode.
- tweak fast-trade: Shift-Enter for selecting items quckly in Trade and Move to Depot screens.
- tweak military-stable-assign: Stop rightmost list of military->Positions from jumping to top.
- tweak military-color-assigned: In same list, color already assigned units in brown & green.

New scripts
-----------
- `fixnaked`: removes thoughts about nakedness.
- `setfps`: set FPS cap at runtime, in case you want slow motion or speed-up.
- `siren`: wakes up units, stops breaks and parties - but causes bad thoughts.
- `fix/population-cap`: run after every migrant wave to prevent exceeding the cap.
- `fix/stable-temp`: counts items with temperature updates; does instant one-shot stable-temp.
- `fix/loyaltycascade`: fix units allegiance, eg after ordering a dwarf merchant kill.
- `deathcause`: shows the circumstances of death for a given body.
- `digfort`: designate areas to dig from a csv file.
- `drain-aquifer`: remove aquifers from the map.
- `growcrops`: cheat to make farm crops instantly grow.
- magmasource: continuously spawn magma from any map tile.
- removebadthoughts: delete all negative thoughts from your dwarves.
- slayrace: instakill all units of a given race, optionally with magma.
- `superdwarf`: per-creature `fastdwarf`.
- `gui/mechanisms`: browse mechanism links of the current building.
- `gui/room-list`: browse other rooms owned by the unit when assigning one.
- `gui/liquids`: a GUI front-end for the liquids plugin.
- `gui/rename`: renaming stockpiles, workshops and units via an in-game dialog.
- `gui/power-meter`: front-end for the Power Meter plugin.
- `gui/siege-engine`: front-end for the Siege Engine plugin.
- `gui/choose-weapons`: auto-choose matching weapons in the military equip screen.

New Plugins
-----------
- `manipulator`: a Dwarf Therapist like UI in the game (:kbd:`u`:kbd:`l`)
- `steam-engine`: an alternative to Water Reactors which make more sense.
  See ``hack/raw/*_steam_engine.txt`` for the necessary raw definitions.
- `power-meter`: a pressure plate modification to detect powered gear
  boxes on adjacent tiles. `gui/power-meter` implements
  the build configuration UI.
- `siege-engine`:  massive overhaul for siege engines, configured via `gui/siege-engine`
- `add-spatter`: allows poison coatings via raw reactions, among other things.
