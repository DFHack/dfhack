:orphan:

.. _History:

#####################
Historical changelogs
#####################

This file is where old changelogs live, so the `current changelog <changelog>`
doesn't get too long. Some of these changelogs are also formatted differently
from current changelogs and would be difficult for the current `changelog
generation system <build-changelog>` to handle.

.. contents:: Contents
  :local:
  :depth: 1

DFHack 0.43.05-r3
=================

Internals
---------
- Fixed an uncommon crash that could occur when printing text to the console
- Added lots of previously-missing DF classes
- More names for fields: https://github.com/DFHack/df-structures/compare/0.43.05-r2...0.43.05

Fixes
-----
- Linux: fixed argument to ``setarch`` in the ``dfhack`` launcher script
- Ruby: fixed an error that occurred when the DF path contained an apostrophe
- `diggingInvaders` now compiles again and is included
- `labormanager`:

    - stopped waiting for on-duty military dwarves with minor injuries to obtain care
    - stopped waiting for meetings when participant(s) are dead
    - fixed a crash for dwarves with no cultural identity

- `luasocket`: fixed ``receive()`` with a byte count
- `orders`: fixed an error when importing orders with material categories
- `siren`: fixed an error
- `stockpiles`: fixed serialization of barrel and bin counts
- `view-item-info`: fixed a ``CHEESE_MAT``-related error

Misc Improvements
-----------------
- `devel/export-dt-ini`: added more offsets for new DT versions
- `digfort`: added support for changing z-levels
- `exportlegends`: suppressed ABSTRACT_BUILDING warning
- `gui/dfstatus`: excluded logs in constructions
- `labormanager`:

    - stopped assigning woodcutting jobs to elves
    - "recover wounded" jobs now weighted based on altruism

- `remotefortressreader`: added support for buildings, grass, riders, and
  hair/beard styles


DFHack 0.43.05-r2
=================

Internals
---------
- Rebuilding DFHack can be faster if nothing Git-related has changed
- Plugins can now hook Screen::readTile()
- Improved Lua compatibility with plugins that hook into GUI functions (like TWBT)
- Expanded focus strings for jobmanagement and workquota_condition viewscreens
- ``Gui::getAnyUnit()``: added support for viewscreen_unitst,
  viewscreen_textviewerst, viewscreen_layer_unit_relationshipst
- Fixed (limited) keybinding support in PRINT_MODE:TEXT on macOS
- Added a new standardized ``Gui::refreshSidebar()`` function to fix behavior of
  some plugins on the lowest z-level
- New ``Buildings`` module functions: ``markedForRemoval()``, ``getCageOccupants()``
- Limited recursive command invocations to 20 to prevent crashes
- Added an ``onLoad.init-example`` file

Lua
---
- Improved C++ exception handling for some native functions that aren't direct
  wrappers around C++ functions (in this case, error messages could be nil and
  cause the Lua interpreter to quit)
- Added support for a ``key_pen`` option in Label widgets
- Fixed ``to_first`` argument to ``dfhack.screen.dismiss()``
- Added optional ``map`` parameters to some screen functions
- Exposed some more functions to Lua:

    - ``dfhack.gui.refreshSidebar()``
    - ``dfhack.gui.getAnyUnit()``
    - ``dfhack.gui.getAnyBuilding()``
    - ``dfhack.gui.getAnyItem()``
    - ``dfhack.gui.getAnyPlant()``
    - ``dfhack.gui.getDepthAt()``
    - ``dfhack.units.getUnitsInBox()``
    - ``dfhack.units.isVisible()``
    - ``dfhack.maps.isTileVisible()``
    - ``dfhack.buildings.markedForRemoval()``
    - ``dfhack.buildings.getCageOccupants()``
    - ``dfhack.internal.md5()``
    - ``dfhack.internal.md5File()``
    - ``dfhack.internal.threadid()``

- New function: ``widgets.Pages:getSelectedPage()``
- Added a ``key`` option to EditField and FilteredList widgets
- Fixed an issue preventing ``repeatUtil.cancel()`` from working when called
  from the callback

Ruby
----
- Fixed a crash when creating new instances of DF virtual classes (e.g. fixes a
  `lever` crash)
- Ruby scripts can now be loaded from any script paths specified (from script-
  paths.txt or registered through the Lua API)
- ``unit_find()`` now uses ``Gui::getSelectedUnit()`` and works in more places
  (e.g. `exterminate` now works from more screens, like `command-prompt`)

New Internal Commands
---------------------
- `alias`: allows configuring aliases for other commands

New Plugins
-----------
- `orders`: Manipulate manager orders
- `pathable`: Back-end for `gui/pathable`

New Scripts
-----------
- `clear-smoke`: Removes all smoke from the map
- `empty-bin`: Empty a bin onto the floor
- `fix/retrieve-units`: Spawns stuck invaders/guests
- `fix/stuck-merchants`: Dismisses stuck merchants that haven't entered the map yet
- `gui/pathable`: View whether tiles on the map can be pathed to
- `gui/teleport`: A front-end for the `teleport` script
- `warn-stuck-trees`: Detects citizens stuck in trees

New Tweaks
----------
- `tweak` burrow-name-cancel: Implements the "back" option when renaming a
  burrow, which currently does nothing (:bug:`1518`)
- `tweak` cage-butcher: Adds an option to butcher units when viewing cages with "q"

Fixes
-----
- Enforced use of ``stdout.log`` and ``stderr.log`` (instead of their ``.txt``
  counterparts) on Windows
- Fixed ``getItemBaseValue()`` for cheese, sheets and instruments
- Fixed alignment in:

    - ``viewscreen_choose_start_sitest``
    - ``viewscreen_export_graphical_mapst``
    - ``viewscreen_setupadventurest``
    - ``viewscreen_setupdwarfgamest``

- `adv-max-skills`: fixed error due to viewscreen changes
- `autolabor`: fixed a crash when assigning haulers while traders are active
- `buildingplan`: fixed an issue that prevented certain numbers from being used
  in building names
- `confirm`:

    - dialogs are now closed permanently when disabled from the settings UI
    - fixed an issue that could have prevented closing dialogs opened by pressing "s"

- `embark-tools`: stopped the sand indicator from overlapping dialogs
- `exportlegends`: fixed some crashes and site map issues
- `devel/find-offsets`: fixed ``current_weather`` scan
- `gui/extended-status`: fixed an error when no beds are available
- `gui/family-affairs`: fixed issues with assigning lovers
- `gui/gm-editor`:

    - made keybinding display order consistent
    - stopped keys from performing actions in help screen

- `gui/manager-quantity`:

    - now allows orders with a limit of 0
    - fixed screen detection

- `gui/mechanisms`, `gui/room-list`: fixed an issue when recentering the map when exiting
- `lever`: prevented pulling non-lever buildings, which can cause crashes
- `markdown`: fixed file encoding
- `modtools/create-unit`:

    - fixed when popup announcements are present
    - added checks to ensure that the current game mode is restored

- `resume`: stopped drawing on the map border
- `show-unit-syndromes`: fixed an error when handling some syndromes
- `strangemood`: fixed some issues with material searches
- `view-item-info`: fixed a color-related error for some materials

Misc Improvements
-----------------
- Docs: prevented automatic hyphenation in some browsers, which was producing
  excessive hyphenation sometimes
- `command-prompt`: invoking ``command-prompt`` a second time now hides the prompt
- `gui/extended-status`: added an option to assign/replace the manager
- `gui/load-screen`:

    - adjusted dialog width for long folder names
    - added modification times and DF versions to dialog

- `gui/mechanisms`, `gui/room-list`, `gui/siege-engine`: add and list "exit to map" options
- `lever`: added support for pulling levers at high priority
- `markdown`: now recognizes ``-n`` in addition to ``/n``
- `remotefortressreader`: more data exported, used by Armok Vision v0.17.0
- `resume`, `siege-engine`: improved compatibility with GUI-hooking plugins (like TWBT)
- `sc-script`: improved help text
- `teleport`: can now be used as a module
- `tweak` embark-profile-name: now enabled in ``dfhack.init-example``
- `tweak` hotkey-clear: fixed display on larger screens


DFHack 0.43.05-r1
=================

Internals
---------
- 64-bit support on all platforms
- Several structure fixes to match 64-bit DF's memory layout
- Added ``DFHack::Job::removeJob()`` function
- New module: ``Designations`` - handles designation creation (currently for plants only)
- Added ``Gui::getSelectedPlant()``
- Added ``Units::getMainSocialActivity()``, ``Units::getMainSocialEvent()``
- Visual Studio 2015 now required to build on Windows instead of 2010
- GCC 4.8 or newer required to build on Linux and OS X (and now supported on OS X)
- Updated TinyXML from 2.5.3 to 2.6.2
- Added the ability to download files manually before building

Lua
---
- Lua has been updated to 5.3 - see https://www.lua.org/manual/5.3/readme.html for details

    - Floats are no longer implicitly converted to integers in DFHack API calls

- ``df.new()`` supports more types: ``char``, ``intptr_t``, ``uintptr_t``, ``long``, ``unsigned long``
- String representations of vectors and a few other containers now include their lengths
- Added a ``tile-material`` module
- Added a ``Painter:key_string()`` method
- Made ``dfhack.gui.revealInDwarfmodeMap()`` available

Ruby
----
- Added support for loading ruby 2.x libraries

New Plugins
-----------
- `dwarfvet` enables animal caretaking
- `generated-creature-renamer`: Renames generated creature IDs for use with graphics packs
- `labormanager` (formerly autolabor2): a more advanced alternative to `autolabor`
- `misery`: re-added and updated for the 0.4x series
- `title-folder`: shows DF folder name in window title bar when enabled

New Scripts
-----------
- `adv-rumors`: improves the "Bring up specific incident or rumor" menu in adventure mode
- `fix/tile-occupancy`: Clears bad occupancy flags on the selected tile.
- `install-info`: Logs basic troubleshooting information about the current DFHack installation
- `load-save`: loads a save non-interactively
- `modtools/change-build-menu`: Edit the build mode sidebar menus
- `modtools/if-entity`: Run a command if the current entity matches a given ID
- `season-palette`: Swap color palettes with the changes of the seasons
- `unforbid`: Unforbids all items

New Tweaks
----------
- `tweak condition-material <tweak>`: fixes a crash in the work order condition material list
- `tweak hotkey-clear <tweak>`: adds an option to clear bindings from DF hotkeys

Fixes
-----
- The DF path on OS X can now contain spaces and ``:`` characters
- Buildings::setOwner() changes now persist properly when saved
- ``ls`` now lists scripts in folders other than ``hack/scripts``, when applicable
- Fixed ``plug`` output alignment for plugins with long names
- `add-thought`: fixed support for emotion names
- `autochop`:

    - fixed several issues with job creation and removal
    - stopped designating the center tile (unreachable) for large trees
    - stopped options from moving when enabling and disabling burrows
    - fixed display of unnamed burrows

- `devel/find-offsets`: fixed a crash when vtables used by globals aren't available
- `getplants`:

    - fixed several issues with job creation and removal
    - stopped designating the center tile (unreachable) for large trees

- `gui/workflow`: added extra keybinding to work with `gui/extended-status`
- `manipulator`:

    - Fixed crash when selecting a profession from an empty list
    - Custom professions are now sorted alphabetically more reliably

- `modtools/create-item`:

    - made gloves usable by specifying handedness
    - now creates pairs of boots and gloves

- `modtools/create-unit`:

    - stopped permanently overwriting the creature creation menu in arena mode
    - now uses non-English names
    - added ``-setUnitToFort`` option to make a unit a civ/group member more easily
    - fixed some issues where units would appear in unrevealed areas of the map

- `modtools/item-trigger`: fixed errors with plant growths
- `remotefortressreader`: fixed a crash when serializing the local map
- `ruby`: fixed a crash when unloading the plugin on Windows
- `stonesense`: disabled overlay in STANDARD-based print modes to prevent crashes
- `title-version`: now hidden when loading an arena

Misc Improvements
-----------------
- Documented all default keybindings (from :file:`dfhack.init-example`) in the
  docs for the relevant commands; updates enforced by build system.
- `autounsuspend`: reduced update frequency to address potential performance issues
- `gui/extended-status`: added a feature to queue beds
- `lua` and `gui/gm-editor` now support the same aliases (``scr``, ``unit``, etc.)
- `manipulator`: added social activities to job column
- `remotefortressreader`: Added support for

    - world map snow coverage
    - spatters
    - wall info
    - site towers, world buildings
    - surface material
    - building items
    - DF version info

- `title-version`: Added a prerelease indicator
- `workflow`: Re-added ``Alt-W`` keybindings


DFHack 0.43.05-beta2
====================

Fixes
-----
- Fixed Buildings::updateBuildings(), along with building creation/deletion events
- Fixed ``plug`` output alignment for plugins with long names
- Fixed a crash that happened when a ``LUA_PATH`` environment variable was set
- `add-thought`: fixed number conversion
- `gui/workflow`: fixed range editing producing the wrong results for certain numbers
- `modtools/create-unit`: now uses non-English names
- `modtools/item-trigger`: fixed errors with plant growths
- `remotefortressreader`: fixed a crash when serializing the local map
- `stockflow`: fixed an issue with non-integer manager order limits
- `title-folder`: fixed compatibility issues with certain SDL libraries on macOS

Structures
----------
- Added some missing renderer VTable addresses on macOS
- ``entity.resources.organic``: identified ``parchment``
- ``entity_sell_category``: added ``Parchment`` and ``CupsMugsGoblets``
- ``ui_advmode_menu``: added ``Build``
- ``ui_unit_view_mode``: added ``PrefOccupation``
- ``unit_skill``: identified ``natural_skill_lvl`` (was ``unk_1c``)
- ``viewscreen_jobmanagementst``: identified ``max_workshops``
- ``viewscreen_overallstatusst``:  made ``visible_pages`` an enum
- ``viewscreen_pricest``: identified fields
- ``viewscreen_workquota_conditionst``: gave some fields ``unk`` names

API Changes
-----------
- Allowed the Lua API to accept integer-like floats and strings when expecting an integer
- Lua: New ``Painter:key_string()`` method
- Lua: Added ``dfhack.getArchitecture()`` and ``dfhack.getArchitectureName()``

Additions/Removals:
-------------------
- Added `adv-rumors` script: improves the "Bring up specific incident or rumor" menu in adventure mode
- Added `install-info` script for basic troubleshooting
- Added `tweak condition-material <tweak>`: fixes a crash in the work order condition material list
- Added `tweak hotkey-clear <tweak>`: adds an option to clear bindings from DF hotkeys
- `autofarm`: reverted local biome detection (from 0.43.05-alpha3)

Other Changes
-------------
- Added a DOWNLOAD_RUBY CMake option, to allow use of a system/external ruby library
- Added the ability to download files manually before building
- `gui/extended-status`: added a feature to queue beds
- `remotefortressreader`: added building items, DF version info
- `stonesense`: Added support for 64-bit macOS and Linux

DFHack 0.43.05-beta1
====================

Fixes
-----
- Fixed various crashes on 64-bit Windows related to DFHack screens, notably `manipulator`
- Fixed addresses of next_id globals on 64-bit Linux (fixes an `automaterial`/box-select crash)
- ``ls`` now lists scripts in folders other than ``hack/scripts``, when applicable
- `modtools/create-unit`: stopped permanently overwriting the creature creation
  menu in arena mode
- `season-palette`: fixed an issue where only part of the screen was redrawn
  after changing the color scheme
- `title-version`: now hidden when loading an arena

Structures
----------
- ``file_compressorst``: fixed field sizes on x64
- ``historical_entity``: fixed alignment on x64
- ``ui_sidebar_menus.command_line``: fixed field sizes on x64
- ``viewscreen_choose_start_sitest``: added 3 missing fields, renamed ``in_embark_only_warning``
- ``viewscreen_layer_arena_creaturest``: identified more fields
- ``world.math``: identified
- ``world.murky_pools``: identified

Additions/Removals
------------------
- `generated-creature-renamer`: Renames generated creature IDs for use with graphics packs

Other Changes
-------------
- `title-version`: Added a prerelease indicator

DFHack 0.43.05-alpha4
=====================

Fixes
-----
- Fixed an issue with uninitialized bitfields that was causing several issues
  (disappearing buildings in `buildingplan`'s planning mode, strange behavior in
  the extended `stocks` screen, and likely other problems). This issue was
  introduced in 0.43.05-alpha3.
- `stockflow`: Fixed an "integer expected" error

Structures
----------
- Located several globals on 64-bit Linux: flows, timed_events, ui_advmode,
  ui_building_assign_type, ui_building_assign_is_marked,
  ui_building_assign_units, ui_building_assign_items, and ui_look_list. This
  fixes `search-plugin`, `zone`, and `force`, among others.
- ``ui_sidebar_menus``: Fixed some x64 alignment issues

Additions/Removals
------------------
- Added `fix/tile-occupancy`: Clears bad occupancy flags on the selected tile.
  Useful for fixing blocked tiles introduced by the above buildingplan issue.
- Added a Lua ``tile-material`` module

Other Changes
-------------
- `labormanager`: Add support for shell crafts
- `manipulator`: Custom professions are now sorted alphabetically more reliably

DFHack 0.43.05-alpha3
=====================

Fixes
-----
- `add-thought`: fixed support for emotion names
- `autofarm`: Made surface farms detect local biome
- `devel/export-dt-ini`: fixed squad_schedule_entry size
- `labormanager`:

    - Now accounts for unit attributes
    - Made instrument-building jobs work (constructed instruments)
    - Fixed deconstructing constructed instruments
    - Fixed jobs in bowyer's shops
    - Fixed trap component jobs
    - Fixed multi-material construction jobs
    - Fixed deconstruction of buildings containing items
    - Fixed interference caused by "store item in vehicle" jobs

- `manipulator`: Fixed crash when selecting a profession from an empty list
- `ruby`:

    - Fixed crash on Win64 due to truncated global addresses
    - Fixed compilation on Win64
    - Use correct raw string length with encodings

Structures
----------
- Changed many ``comment`` XML attributes with version numbers to use new
  ``since`` attribute instead
- ``activity_event_conflictst.sides``: named many fields
- ``building_def.build_key``: fixed size on 64-bit Linux and OS X
- ``historical_kills``:

    - ``unk_30`` -> ``killed_underground_region``
    - ``unk_40`` -> ``killed_region``

- ``historical_kills.killed_undead``: removed ``skeletal`` flag
- ``ui_advmode``: aligned enough so that it doesn't crash (64-bit OS X/Linux)
- ``ui_advmode.show_menu``: changed from bool to enum
- ``unit_personality.emotions.flags``: now a bitfield

API Changes
-----------
- Added ``DFHack::Job::removeJob()`` function
- C++: Removed bitfield constructors that take an initial value. These kept
  bitfields from being used in unions. Set ``bitfield.whole`` directly instead.
- Lua: ``bitfield.whole`` now returns an integer, not a decimal

Additions/Removals
------------------
- Removed source for treefarm plugin (wasn't built)
- Added `modtools/change-build-menu`: Edit the build mode sidebar menus
- Added `modtools/if-entity`: Run a command if the current entity matches a
  given ID
- Added `season-palette`: Swap color palettes with the changes of the seasons

Other changes
-------------
- Changed minimum GCC version to 4.8 on OS X and Linux (earlier versions
  wouldn't have worked on Linux anyway)
- Updated TinyXML from 2.5.3 to 2.6.2

DFHack 0.43.03-r1
=================

Lua
---
- Label widgets can now easily register handlers for mouse clicks

New Features
------------
- `add-thought`: allow syndrome name as ``-thought`` argument
- `gui/gm-editor`

    - Added ability to insert default types into containers. For primitive types leave the type entry empty, and for references use ``*``.
    - Added ``shift-esc`` binding to fully exit from editor
    - Added ``gui/gm-editor toggle`` command to toggle editor visibility (saving position)

- `modtools/create-unit`:

    - Added an option to attach units to an existing wild animal population
    - Added an option to attach units to a map feature

Fixes
-----
- `autofarm`: Can now handle crops that grow for more than a season
- `combine-plants`: Fixed recursion into sub-containers
- `createitem`: Now moves multiple created items to cursor correctly
- `exportlegends`: Improved handling of unknown enum items (fixes many errors)
- `gui/create-item`: Fixed quality when creating multiple items
- `gui/mod-manager`: Fixed error when mods folder doesn't exist
- `modtools/item-trigger`: Fixed handling of items with subtypes
- `reveal`: ``revflood`` now handles constructed stairs with floors in generated fortresses
- `stockflow`:

    - Can order metal mechanisms
    - Fixed material category of thread-spinning jobs

Misc Improvements
-----------------
- The built-in ``ls`` command now wraps the descriptions of commands
- `catsplosion`: now a lua script instead of a plugin
- `fix/diplomats`: replaces ``fixdiplomats``
- `fix/merchants`: replaces ``fixmerchants``
- `prefchange`: added a ``help`` option
- `probe`: now displays raw tiletype names
- Unified script documentation and in-terminal help options

Removed
-------
- `tweak` manager-quantity: no longer needed

DFHack 0.42.06-r1
=================

Internals
---------
- Commands to run on startup can be specified on the command line with ``+``

    Example::

        ./dfhack +devel/print-args example
        "Dwarf Fortress.exe" +devel/print-args example

- Prevented plugins with active viewscreens from being unloaded and causing a crash
- Additional script search paths can be specified in dfhack-config/script-paths.txt

Lua
---
- `building-hacks` now supports ``auto_gears`` flags. It automatically finds and animates gears in building definition
- Changed how `eventful` triggers reaction complete. Now it has ``onReactionComplete`` and ``onReactionCompleting``. Second one can be canceled

New Plugins
-----------
- `autogems`: Creates a new Workshop Order setting, automatically cutting rough gems

New Scripts
-----------
- `devel/save-version`: Displays DF version information about the current save
- `modtools/extra-gamelog`: replaces ``log-region``, ``soundsense-season``, and ``soundsense``

New Features
------------
- `buildingplan`: Support for floodgates, grates, and bars
- `colonies`: new ``place`` subcommand and supports any vermin (default honey bees)
- `confirm`: Added a confirmation for retiring locations
- `exportlegends`: Exports more information (poetic/musical/dance forms, written/artifact content, landmasses, extra histfig information, and more)
- `search-plugin`: Support for new screens:

    - location occupation assignment
    - civilization animal training knowledge
    - animal trainer assignment

- `tweak`:

    - ``tweak block-labors``: Prevents labors that can't be used from being toggled
    - ``tweak hide-priority``: Adds an option to hide designation priority indicators
    - ``tweak title-start-rename``: Adds a safe rename option to the title screen "Start Playing" menu

- `zone`:

    - Added ``unassign`` subcommand
    - Added ``only`` option to ``assign`` subcommand

Fixes
-----
- Fixed a crash bug caused by the historical figures DFHack uses to store persistent data.
- More plugins should recognize non-dwarf citizens
- Fixed a possible crash from cloning jobs
- moveToBuilding() now sets flags for items that aren't a structural part of the building properly
- `autotrade`, `stocks`: Made trading work when multiple caravans are present but only some can trade
- `confirm` note-delete: No longer interferes with name entry
- `exportlegends`: Handles entities without specific races, and a few other fixes for things new to v0.42
- `fastdwarf`: Fixed a bug involving teleporting mothers but not the babies they're holding.
- `gaydar`: Fixed text display on OS X/Linux and failure with soul-less creatures
- `manipulator`:

    - allowed editing of non-dwarf citizens
    - stopped ghosts and visitors from being editable
    - fixed applying last custom profession

- `modtools/create-unit`: Stopped making units without civs historical figures
- `modtools/force`:

    - Removed siege option
    - Prevented a crash resulting from a bad civilization option

- `showmood`: Fixed name display on OS X/Linux
- `view-item-info`: Fixed density units

Misc Improvements
-----------------
- `autochop`: Can now edit log minimum/maximum directly and remove limit entirely
- `autolabor`, `autohauler`, `manipulator`: Added support for new jobs/labors/skills
- `colonies`: now implemented by a script
- `createitem`: Can now create items anywhere without specifying a unit, as long as a unit exists on the map
- `devel/export-dt-ini`: Updated for 0.42.06
- `devel/find-offsets`: Automated several more scans
- `gui/gm-editor`: Now supports finding some items with a numeric ID (with ``i``)
- `lua`: Now supports some built-in variables like `gui/gm-editor`, e.g. ``unit``, ``screen``
- `remotefortressreader`: Can now trigger keyboard events
- `stockflow`: Now offers better control over individual craft jobs
- `weather`: now implemented by a script
- `zone`: colored output

Removed
-------
- DFusion: legacy script system, obsolete or replaced by better alternatives


DFHack 0.40.24-r5
=================

New Features
------------
- `confirm`:

    - Added a ``uniform-delete`` option for military uniform deletion
    - Added a basic in-game configuration UI

Fixes
-----
- Fixed a rare crash that could result from running `keybinding` in onLoadWorld.init
- Script help that doesn't start with a space is now recognized correctly
- `confirm`: Fixed issues with haul-delete, route-delete, and squad-disband confirmations intercepting keys too aggressively
- `emigration` should work now
- `fix-unit-occupancy`: Significantly optimized - up to 2,000 times faster in large fortresses
- `gui/create-item`: Allow exiting quantity prompt
- `gui/family-affairs`: Fixed an issue where lack of relationships wasn't recognized and other issues
- `modtools/create-unit`: Fixed a possible issue in reclaim fortress mode
- `search-plugin`: Fixed a crash on the military screen
- `tweak` max-wheelbarrow: Fixed a minor display issue with large numbers
- `workflow`: Fixed a crash related to job postings (and added a fix for existing, broken jobs)

Misc Improvements
-----------------
- Unrecognized command feedback now includes more information about plugins
- `fix/dry-buckets`: replaces the ``drybuckets`` plugin
- `feature`: now implemented by a script

DFHack 0.40.24-r4
=================

Internals
---------
- A method for caching screen output is now available to Lua (and C++)
- Developer plugins can be ignored on startup by setting the ``DFHACK_NO_DEV_PLUGINS`` environment variable
- The console on Linux and OS X now recognizes keyboard input between prompts
- JSON libraries available (C++ and Lua)
- More DFHack build information used in plugin version checks and available to plugins and lua scripts
- Fixed a rare overflow issue that could cause crashes on Linux and OS X
- Stopped DF window from receiving input when unfocused on OS X
- Fixed issues with keybindings involving :kbd:`Ctrl`:kbd:`A` and :kbd:`Ctrl`:kbd:`Z`,
  as well as :kbd:`Alt`:kbd:`E`/:kbd:`U`/:kbd:`N` on OS X
- Multiple contexts can now be specified when adding keybindings
- Keybindings can now use :kbd:`F10`-:kbd:`F12` and :kbd:`0`-:kbd:`9`
- Plugin system is no longer restricted to plugins that exist on startup
- :file:`dfhack.init` file locations significantly generalized

Lua
---
- Scripts can be enabled with the built-in `enable`/`disable <disable>` commands
- A new function, ``reqscript()``, is available as a safer alternative to ``script_environment()``
- Lua viewscreens can choose not to intercept the OPTIONS keybinding

New internal commands
---------------------
- `kill-lua`: Interrupt running Lua scripts
- `type`: Show where a command is implemented

New plugins
-----------
- `confirm`: Adds confirmation dialogs for several potentially dangerous actions
- `fix-unit-occupancy`: Fixes issues with unit occupancy, such as faulty "unit blocking tile" messages (:bug:`3499`)
- `title-version` (formerly ``vshook``): Display DFHack version on title screen

New scripts
-----------
- `armoks-blessing`: Adjust all attributes, personality, age and skills of all dwarves in play
- `brainwash`: brainwash a dwarf (modifying their personality)
- `burial`:  sets all unowned coffins to allow burial ("-pets" to allow pets too)
- `deteriorateclothes`: make worn clothes on the ground wear far faster to boost FPS
- `deterioratecorpses`: make body parts wear away far faster to boost FPS
- `deterioratefood`: make food vanish after a few months if not used
- `elevate-mental`: elevate all the mental attributes of a unit
- `elevate-physical`: elevate all the physical attributes of a unit
- `emigration`: stressed dwarves may leave your fortress if they see a chance
- `fix-ster`:  changes fertility/sterility of animals or dwarves
- `gui/family-affairs`: investigate and alter romantic relationships
- `make-legendary`: modify skill(s) of a single unit
- `modtools/create-unit`: create new units from nothing
- `modtools/equip-item`: a script to equip items on units
- `points`:  set number of points available at embark screen
- `pref-adjust`: Adjust all preferences of all dwarves in play
- `rejuvenate`: make any "old" dwarf 20 years old
- `starvingdead`: make undead weaken after one month on the map, and crumble after six
- `view-item-info`:  adds information and customisable descriptions to item viewscreens
- `warn-starving`:  check for starving, thirsty, or very drowsy units and pause with warning if any are found

New tweaks
----------
- embark-profile-name: Allows the use of lowercase letters when saving embark profiles
- kitchen-keys: Fixes DF kitchen meal keybindings
- kitchen-prefs-color: Changes color of enabled items to green in kitchen preferences
- kitchen-prefs-empty: Fixes a layout issue with empty kitchen tabs

Fixes
-----
- Plugins with vmethod hooks can now be reloaded on OS X
- Lua's ``os.system()`` now works on OS X
- Fixed default arguments in Lua gametype detection functions
- Circular lua dependencies (reqscript/script_environment) fixed
- Prevented crash in ``Items::createItem()``
- `buildingplan`: Now supports hatch covers
- `gui/create-item`: fixed assigning quality to items, made :kbd:`Esc` work properly
- `gui/gm-editor`: handles lua tables properly
- `help`: now recognizes built-in commands, like ``help``
- `manipulator`: fixed crash when selecting custom professions when none are found
- `remotefortressreader`: fixed crash when attempting to send map info when no map was loaded
- `search-plugin`: fixed crash in unit list after cancelling a job; fixed crash when disabling stockpile category after searching in a subcategory
- `stockpiles`: now checks/sanitizes filenames when saving
- `stocks`: fixed a crash when right-clicking
- `steam-engine`: fixed a crash on arena load; number keys (e.g. 2/8) take priority over cursor keys when applicable
- tweak fps-min fixed
- tweak farm-plot-select: Stopped controls from appearing when plots weren't fully built
- `workflow`: Fixed some issues with stuck jobs. Existing stuck jobs must be cancelled and re-added
- `zone`: Fixed a crash when using ``zone set`` (and a few other potential crashes)

Misc Improvements
-----------------
- DFHack documentation:

    - massively reorganised, into files of more readable size
    - added many missing entries
    - indexes, internal links, offline search all documents
    - includes documentation of linked projects (df-structures, third-party scripts)
    - better HTML generation with Sphinx
    - documentation for scripts now located in source files

- `autolabor`:

    - Stopped modification of labors that shouldn't be modified for brokers/diplomats
    - Prioritize skilled dwarves more efficiently
    - Prevent dwarves from running away with tools from previous jobs

- `automaterial`: Fixed several issues with constructions being allowed/disallowed incorrectly when using box-select
- `dwarfmonitor`:

    - widgets' positions, formats, etc. are now customizable
    - weather display now separated from the date display
    - New mouse cursor widget

- `gui/dfstatus`: Can enable/disable individual categories and customize metal bar list
- `full-heal`: ``-r`` option removes corpses
- `gui/gm-editor`

    - Pointers can now be displaced
    - Added some useful aliases: "item" for the selected item, "screen" for the current screen, etc.
    - Now avoids errors with unrecognized types

- `gui/hack-wish`: renamed to `gui/create-item`
- `keybinding list <keybinding>` accepts a context
- `lever`:

    - Lists lever names
    - ``lever pull`` can be used to pull the currently-selected lever

- ``memview``: Fixed display issue
- `modtools/create-item`: arguments are named more clearly, and you can specify the creator to be the unit with id ``df.global.unit_next_id-1`` (useful in conjunction with `modtools/create-unit`)
- ``nyan``: Can now be stopped with dfhack-run
- `plug`: lists all plugins; shows state and number of commands in plugins
- `prospect`: works from within command-prompt
- `quicksave`: Restricted to fortress mode
- `remotefortressreader`: Exposes more information
- `search-plugin`:

    - Supports noble suggestion screen (e.g. suggesting a baron)
    - Supports fortress mode loo[k] menu
    - Recognizes ? and ; keys

- `stocks`: can now match beginning and end of item names
- `teleport`: Fixed cursor recognition
- `tidlers`, `twaterlvl`: now implemented by scripts instead of a plugin
- `tweak`:

    - debug output now logged to stderr.log instead of console - makes DFHack start faster
    - farm-plot-select: Fixed issues with selecting undiscovered crops

- `workflow`: Improved handling of plant reactions

Removed
-------
- `embark-tools` nano: 1x1 embarks are now possible in vanilla 0.40.24

DFHack 0.40.24-r3
=================

Internals
---------
- Ruby library now included on OS X - Ruby scripts should work on OS X 10.10
- libstdc++ should work with older versions of OS X
- Added support for `onMapLoad.init / onMapUnload.init <other_init_files>` scripts
- game type detection functions are now available in the World module
- The ``DFHACK_LOG_MEM_RANGES`` environment variable can be used to log information to ``stderr.log`` on OS X
- Fixed adventure mode menu names
- Fixed command usage information for some commands

Lua
---
- Lua scripts will only be reloaded if necessary
- Added a ``df2console()`` wrapper, useful for printing DF (CP437-encoded) text to the console in a portable way
- Added a ``strerror()`` wrapper

New Internal Commands
---------------------
- `hide`, `show`:  hide and show the console on Windows
- `sc-script`:  Allows additional scripts to be run when certain events occur (similar to `onLoad.init` scripts)

New Plugins
-----------
- `autohauler`:  A hauling-only version of autolabor

New Scripts
-----------
- `modtools/reaction-product-trigger`:  triggers callbacks when products are produced (contrast with when reactions complete)

New Tweaks
----------
- `fps-min <tweak>`:  Fixes the in-game minimum FPS setting
- `shift-8-scroll <tweak>`:  Gives Shift+8 (or ``*``) priority when scrolling menus, instead of scrolling the map
- `tradereq-pet-gender <tweak>`:  Displays pet genders on the trade request screen

Fixes
-----
- Fixed game type detection in `3dveins`, `gui/create-item`, `reveal`, `seedwatch`
- ``PRELOAD_LIB``:  More extensible on Linux
- `add-spatter`, `eventful`:  Fixed crash on world load
- `add-thought`:  Now has a proper subthought arg.
- `building-hacks`:  Made buildings produce/consume correct amount of power
- `fix-armory`:  compiles and is available again (albeit with issues)
- `gui/gm-editor`:  Added search option (accessible with "s")
- `hack-wish <gui/create-item>`:  Made items stack properly.
- `modtools/skill-change`:  Made level granularity work properly.
- `show-unit-syndromes`:  should work
- `stockflow`:

  - Fixed error message in Arena mode
  - no longer checks the DF version
  - fixed ballistic arrow head orders
  - convinces the bookkeeper to update records more often

- `zone`:  Stopped crash when scrolling cage owner list

Misc Improvements
-----------------
- `autolabor`:  A negative pool size can be specified to use the most unskilled dwarves
- `building-hacks`:

  - Added a way to allow building to work even if it consumes more power than is available.
  - Added setPower/getPower functions.

- `catsplosion`:  Can now trigger pregnancies in (most) other creatures
- `exportlegends`:  ``info`` and ``all`` options export ``legends_plus.xml`` with more data for legends utilities
- `manipulator`:

  - Added ability to edit nicknames/profession names
  - added "Job" as a View Type, in addition to "Profession" and "Squad"
  - added custom profession templates with masking

- `remotefortressreader`:  Exposes more information


DFHack 0.40.24-r2
=================

Internals
---------
- Lua scripts can set environment variables of each other with ``dfhack.run_script_with_env``
- Lua scripts can now call each others internal nonlocal functions with ``dfhack.script_environment(scriptName).functionName(arg1,arg2)``
- `eventful`: Lua reactions no longer require LUA_HOOK as a prefix; you can register a callback for the completion of any reaction with a name
- Filesystem module now provides file access/modification times and can list directories (normally and recursively)
- Units Module: New functions::

    isWar
    isHunter
    isAvailableForAdoption
    isOwnCiv
    isOwnRace
    getRaceName
    getRaceNamePlural
    getRaceBabyName
    getRaceChildName
    isBaby
    isChild
    isAdult
    isEggLayer
    isGrazer
    isMilkable
    isTrainableWar
    isTrainableHunting
    isTamable
    isMale
    isFemale
    isMerchant
    isForest
    isMarkedForSlaughter

- Buildings Module: New Functions::

    isActivityZone
    isPenPasture
    isPitPond
    isActive
    findPenPitAt

Fixes
-----
- ``dfhack.run_script`` should correctly find save-specific scripts now.
- `add-thought`: updated to properly affect stress.
- `hfs-pit`: should work now
- `autobutcher`: takes gelding into account
- :file:`init.lua` existence checks should be more reliable (notably when using non-English locales)

Misc Improvements
-----------------
Multiline commands are now possible inside dfhack.init scripts. See :file:`dfhack.init-example` for example usage.


DFHack 0.40.24-r1
=================

Internals
---------
CMake shouldn't cache DFHACK_RELEASE anymore. People may need to manually update/delete their CMake cache files to get rid of it.


DFHack 0.40.24-r0
=================

Internals
---------
- `EventManager`: fixed crash error with EQUIPMENT_CHANGE event.
- key modifier state exposed to Lua (ie :kbd:`Ctrl`, :kbd:`Alt`, :kbd:`Shift`)

Fixes
-----
``dfhack.sh`` can now be run from other directories on OS X

New Plugins
-----------
- `blueprint`: export part of your fortress to quickfort .csv files

New Scripts
-----------
- `hotkey-notes`:  print key, name, and jump position of hotkeys

Removed
-------
- needs_porting/*

Misc Improvements
-----------------
- Added support for searching more lists

DFHack 0.40.23-r1
=================

Internals
---------
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
---------
- Plugins on OS X now use ``.plug.dylib`` as an extension instead of ``.plug.so``

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
