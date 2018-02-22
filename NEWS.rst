.. comment
    This is the changelog file for DFHack.  If you add or change anything, note
    it here under the heading "DFHack Future", in the appropriate section.
    Items within each section are listed in alphabetical order to minimise merge
    conflicts.  Try to match the style and level of detail of the other entries.

    This file should not contain details specific to prereleases, but it should
    contain changes from previous stable releases. For example, if a bug was
    introduced in one alpha version and fixed in another, do not include it
    here.

    Sections for each release are added as required, and consist solely of the
    following in order as subheadings::

        New [Internal Commands | Plugins | Scripts | Tweaks | Features]
        Fixes
        Misc Improvements
        Removed
        Internals
        Lua
        Ruby

    When referring to a script, plugin, or command, use backticks (```) to
    create a link to the relevant documentation - and check that the docs are
    still up to date!

    When adding a new release, change "DFHack future" to the appropriate title
    before releasing, and then add a new "DFHack future" section after releasing.

.. _changelog:

#########
Changelog
#########

.. contents::
   :depth: 2

DFHack 0.44.05-r2
=================

New Plugins
-----------
- `embark-assistant`: adds more information and features to embark screen

New Scripts
-----------
- `adv-fix-sleepers`: fixes units in adventure mode who refuse to wake up (:bug:`6798`)
- `hermit`: blocks caravans, migrants, diplomats (for hermit challenge)

New Features
------------
- With ``PRINT_MODE:TEXT``, setting the ``DFHACK_HEADLESS`` environment variable
  will hide DF's display and allow the console to be used normally. (Note that
  this is intended for testing and is not very useful for actual gameplay.)

Fixes
-----
- `devel/export-dt-ini`: fix language_name offsets for DT 39.2+
- `devel/inject-raws`: fixed gloves and shoes (old typo causing errors)
- `remotefortressreader`: fixed an issue with not all engravings being included
- `view-item-info`: fixed an error with some shields

Misc Improvements
-----------------
- `adv-rumors`: added more keywords, including names
- `autochop`: can now exclude trees that produce fruit, food, or cookable items
- `remotefortressreader`: added plant type support

DFHack 0.44.05-r1
=================

New Scripts
-----------
- `break-dance`: Breaks up a stuck dance activity
- `cannibalism`: Allows consumption of sapient corpses
- `devel/check-other-ids`: Checks the validity of "other" vectors in the
  ``world`` global
- `fillneeds`: Use with a unit selected to make them focused and unstressed
- `firestarter`: Lights things on fire: items, locations, entire inventories even!
- `flashstep`: Teleports adventurer to cursor
- `ghostly`: Turns an adventurer into a ghost or back
- `gui/cp437-table`: An in-game CP437 table
- `questport`: Sends your adventurer to the location of your quest log cursor
- `view-unit-reports`: opens the reports screen with combat reports for the selected unit

Fixes
-----
- Fixed issues with the console output color affecting the prompt on Windows
- `createitem`: stopped items from teleporting away in some forts
- `devel/inject-raws`: now recognizes spaces in reaction names
- `dig`: added support for designation priorities - fixes issues with
  designations from ``digv`` and related commands having extremely high priority
- `dwarfmonitor`:

    - fixed display of creatures and poetic/music/dance forms on ``prefs`` screen
    - added "view unit" option
    - now exposes the selected unit to other tools

- `gui/gm-unit`: can now edit mining skill
- `gui/quickcmd`: stopped error from adding too many commands
- `names`: fixed many errors
- `quicksave`: fixed an issue where the "Saving..." indicator often wouldn't appear

Misc Improvements
-----------------
- The console now provides suggestions for built-in commands
- `binpatch`: now reports errors for empty patch files
- `devel/export-dt-ini`: avoid hardcoding flags
- `exportlegends`:

    - reordered some tags to match DF's order
    - added progress indicators for exporting long lists

- `force`: now provides useful help
- `full-heal`:

    - can now select corpses to resurrect
    - now resets body part temperatures upon resurrection to prevent creatures
      from freezing/melting again
    - now resets units' vanish countdown to reverse effects of `exterminate`

- `gui/gm-editor`: added enum names to enum edit dialogs
- `gui/gm-unit`:

    - made skill search case-insensitive
    - added a profession editor
    - misc. layout improvements

- `gui/liquids`: added more keybindings: 0-7 to change liquid level, P/B to cycle backwards
- `gui/pathable`: added tile types to sidebar
- `gui/rename`: added "clear" and "special characters" options
- `launch`: can now ride creatures
- `modtools/skill-change`:

    - now updates skill levels appropriately
    - only prints output if ``-loud`` is passed

- `names`: can now edit names of units
- `remotefortressreader`:

    - support for moving adventurers
    - suport for item stack sizes, vehicles, gem shapes, item volume, art images, item improvements
    - some performance improvements

Removed
-------
- `warn-stuck-trees`: :bug:`9252` fixed in DF 0.44.01
- `tweak`: ``kitchen-keys``: :bug:`614` fixed in DF 0.44.04

Internals
---------
- ``Gui::getAnyUnit()`` supports many more screens/menus
- New globals available:

    - ``version``
    - ``min_load_version``
    - ``movie_version``
    - ``basic_seed``
    - ``title``
    - ``title_spaced``
    - ``ui_building_resize_radius``
    - ``soul_next_id``

Lua
---
- Added a new ``dfhack.console`` API
- Exposed ``get_vector()`` (from C++) for all types that support ``find()``,
  e.g. ``df.unit.get_vector() == df.global.world.units.all``
- Improved ``json`` I/O error messages
- Stopped a crash when trying to create instances of classes whose vtable
  addresses are not available


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
- Lua has been updated to 5.3 - see http://www.lua.org/manual/5.3/readme.html for details

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
Added support for searching more lists


Older Changelogs
================
Are kept in a seperate file:  `HISTORY`

.. that's ``docs/history.rst``, if you're reading the raw text.
