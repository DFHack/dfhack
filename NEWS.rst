.. comment
    This is the changelog file for DFHack.  If you add or change anything, note
    it here under the heading "DFHack Future", in the appropriate section.
    Items within each section are listed in alphabetical order to minimise merge
    conflicts.  Try to match the style and level of detail of the other entries.

    Sections for each release are added as required, and consist solely of the
    following in order as subheadings::

        Internals
        Lua
        New [Internal Commands | Plugins | Scripts | Features]
        Fixes
        Misc Improvements
        Removed

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

DFHack future
=============

Internals
---------
- Persistent data may now stored in a json format instead of using fake historical figures.
  This is more efficient for plugins and scripts that use a large amount of persistent data.

- EventManager: there is now a ``PRESAVE`` event, triggered while the game writes data to files. All save data should still be loaded in memory.
  This gives plugins and scripts a last chance to ensure persistent data is preserved, but modifying other data structures may corrupt the save.

Lua
---
- Label widgets can now easily register handlers for mouse clicks

New Features
------------
- `gui/gm-editor` it's now possible to insert default types to containers. For primitive types leave the type entry empty, for references use ``*``.

Fixes
-----
- `createitem`: Now moves multiple created items to cursor correctly
- `exportlegends`: Improved handling of unknown enum items (fixes many errors)
- `gui/create-item`: Fixed quality when creating multiple items
- `gui/mod-manager`: Fixed error when mods folder doesn't exist
- `modtools/item-trigger`: Fixed handling of items with subtypes

Misc Improvements
-----------------
- `autogems`: Converted persistent data to json format
- `catsplosion`: now a lua script instead of a plugin
- `fix/diplomats`: replaces ``fixdiplomats``
- `fix/merchants`: replaces ``fixmerchants``
- `stockflow`: Converted persistent data to json format
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
