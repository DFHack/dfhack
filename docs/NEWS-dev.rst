.. comment
    This is the development changelog file for DFHack.  If you add or change
    anything, note it here under the heading "DFHack Future", in the appropriate
    section. Items within each section are listed in alphabetical order to
    minimise merge conflicts.  Try to match the style and level of detail of the
    other entries.

    This file contains changes that are relevant to users of prereleases. These
    changes should include changes from just the previous release, whether that
    release was stable or not. For instance, a feature added in 0.43.05-alpha1
    should go under "0.43.05-alpha1" here *and* "0.43.05-r1" (or "future") in
    NEWS.rst. A fix in one prerelease for an issue in the previous prerelease
    should just go here in the appropriate section, not in NEWS.rst.

    Sections for each release are added as required, and consist solely of the
    following in order as subheadings::

        Fixes
        Structures
        API Changes
        Additions/Removals
        Other Changes

    When referring to a script, plugin, or command, use backticks (```) to
    create a link to the relevant documentation - and check that the docs are
    still up to date!

    When adding a new release, change "DFHack future" to the appropriate title
    before releasing, and then add a new "DFHack future" section after releasing.

.. _dev-changelog:

#####################
Development Changelog
#####################

.. contents::
   :depth: 2

DFHack 0.44.05-alpha1
=====================

Structures
----------
- ``incident``: re-aligned again to match disassembly

Other Changes
-------------
- `gui/liquids`: added more keybindings: 0-7 to change liquid level, P/B to cycle backwards

DFHack 0.44.04-alpha1
=====================

Fixes
-----
- `devel/inject-raws`: now recognizes spaces in reaction names
- `exportlegends`: fixed an error that could occur when exporting empty lists

Structures
----------
- ``artifact_record``: fixed layout (changed in 0.44.04)
- ``incident``: fixed layout (changed in 0.44.01) - note that many fields have moved

DFHack 0.44.03-beta1
====================

Fixes
-----
- `autolabor`, `autohauler`, `labormanager`: added support for "put item on
  display" jobs and building/destroying display furniture
- `gui/gm-editor`: fixed an error when editing primitives in Lua tables

Structures
----------
- Added 7 new globals from DF: ``version``, ``min_load_version``,
  ``movie_version``, ``basic_seed``, ``title``, ``title_spaced``,
  ``ui_building_resize_radius``
- Added ``twbt_render_map`` code offset on x64
- Fixed an issue preventing ``enabler`` from being allocated by DFHack
- Added ``job_type.PutItemOnDisplay``
- Found ``renderer`` vtable on osx64
- ``adventure_movement_optionst``, ``adventure_movement_hold_tilest``,
  ``adventure_movement_climbst``: named coordinate fields
- ``mission``: added type
- ``unit``: added 3 new vmethods: ``getCreatureTile``, ``getCorpseTile``, ``getGlowTile``
- ``viewscreen_assign_display_itemst``: fixed layout on x64 and identified many fields
- ``viewscreen_reportlistst``: fixed layout, added ``mission_id`` vector
- ``world.status``: named ``missions`` vector

Other Changes
-------------
- `devel/dump-offsets`: now ignores ``index`` globals
- `gui/pathable`: added tile types to sidebar
- `modtools/skill-change`:

    - now updates skill levels appropriately
    - only prints output if ``-loud`` is passed

DFHack 0.44.03-alpha1
=====================

Other Changes
-------------
- Lua: Improved ``json`` I/O error messages
- Lua: Stopped a crash when trying to create instances of classes whose vtable
  addresses are not available

DFHack 0.44.02-beta1
====================

Fixes
-----
- Fixed issues with the console output color affecting the prompt on Windows
- `createitem`: stopped items from teleporting away in some forts
- `gui/gm-unit`: can now edit mining skill
- `gui/quickcmd`: stopped error from adding too many commands
- `modtools/create-unit`: fixed error when domesticating units

Structures
----------
- Located ``start_dwarf_count`` offset for all builds except 64-bit Linux;
  `startdwarf` should work now
- Added ``buildings_other_id.DISPLAY_CASE``
- Fixed ``viewscreen_titlest.start_savegames`` alignment
- Fixed ``unit`` alignment
- Identified ``historical_entity.unknown1b.deities`` (deity IDs)

API Changes
-----------
- Lua; Exposed ``get_vector()`` (from C++) for all types that support
  ``find()``, e.g. ``df.unit.get_vector() == df.global.world.units.all``

Additions/Removals
------------------
- Added `devel/check-other-ids`: Checks the validity of "other" vectors in the
  ``world`` global
- Added `gui/cp437-table`: An in-game CP437 table
- Removed `warn-stuck-trees`: the corresponding DF bug was fixed in 0.44.01

Other Changes
-------------
- The console now provides suggestions for built-in commands
- `devel/export-dt-ini`: avoid hardcoding flags
- `exportlegends`:

    - reordered some tags to match DF's order
    - added progress indicators for exporting long lists

- `gui/gm-editor`: added enum names to enum edit dialogs
- `gui/gm-unit`: made skill search case-insensitive
- `gui/rename`: added "clear" and "special characters" options
- `remotefortressreader`: includes item stack sizes and some performance improvements


DFHack 0.44.02-alpha1
=====================

Fixes
-----
- Fixed a crash that could occur if a symbol table in symbols.xml had no content
- The Lua API can now wrap functions with 12 or 13 parameters

Structures
----------
- The ``ui_menu_width`` global is now a 2-byte array; the second item is the
  former ``ui_area_map_width`` global, which is now removed
- The former ``announcements`` global is now a field in ``d_init``
- ``world`` fields formerly beginning with ``job_`` are now fields of
  ``world.jobs``, e.g. ``world.job_list`` is now ``world.jobs.list``

API Changes
-----------
- Lua: Added a new ``dfhack.console`` API

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
