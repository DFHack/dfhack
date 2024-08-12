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

0.47.05-r8
==========

New Plugins
-----------
- `overlay`: plugin is transformed from a single line of text that runs `gui/launcher` on click to a fully-featured overlay injection framework. It now houses a popup menu for keybindings relevant to the current DF screen, all the widgets previously provided by `dwarfmonitor` (e.g. the current date and number of happy/unhappy dwarves), the overlay that highlights suspended buildings when you pause, and others. See `overlay-dev-guide` for details.
- `channel-safely`: auto-manage channel designations to keep dwarves safe

Fixes
-----
- `automaterial`: fix the cursor jumping up a z level when clicking quickly after box select
- `buildingplan`: fix crash when canceling out of placement mode for a building with planning mode enabled and subsequently attempting to place a building that does not have planning mode enabled and that has no pertinent materials available
- `gui/create-item`: prevent materials list filter from intercepting sublist hotkeys
- `mousequery`: fix the cursor jumping up z levels sometimes when using TWBT
- `tiletypes`: no longer resets dig priority to the default when updating other properties of a tile
- `automaterial`: fix rendering errors with box boundary markers
- `autolabor`, `autohauler`: properly handle jobs 241, 242, and 243
- `autofarm`: flush output so status text is visible immediately after running the command
- `hotkeys`: correctly detect hotkeys bound to number keys, F11, and F12
- `labormanager`: associate quern construction with the correct labor
- ``EventManager``: fix a segmentation fault with the ``REPORT`` event
- ``EventManager``: fix the ``JOB_STARTED`` event only sending events to the first handler listed instead of all registered handlers
- Core: ensure ``foo.init`` always runs before ``foo.*.init`` (e.g. ``dfhack.init`` should always run before ``dfhack.something.init``)

Misc Improvements
-----------------
- `autolabor`, `autohauler`: refactored to use DFHack's messaging system for info/debug/trace messages
- `blueprint`: new ``--smooth`` option for recording all smoothed floors and walls instead of just the ones that require smoothing for later carving
- `blueprint`: record built constructions in blueprints
- `blueprint`: record stockpile/building/zone names in blueprints
- `blueprint`: record room sizes in blueprints
- `blueprint`: generate meta blueprints to reduce the number of blueprints you have to apply
- `blueprint`: support splitting the output file into phases grouped by when they can be applied
- `blueprint`: when splitting output files, number them so they sort into the order you should apply them in
- `dwarfmonitor`: widgets have been ported to the overlay framework and can be enabled and configured via the `gui/overlay` UI
- `ls`: indent tag listings and wrap them in the rightmost column for better readability
- `ls`: new ``--exclude`` option for hiding matched scripts from the output. this can be especially useful for modders who don't want their mod scripts to be included in ``ls`` output.
- `hotkeys`: hotkey screen has been transformed into an interactive `overlay` widget that you can bring up by moving the mouse cursor over the hotspot (in the upper left corner of the screen by default). Enable/disable/reposition the hotspot in the `gui/overlay` UI. Even if the hotspot is disabled, the menu can be brought up at any time with the Ctrl-Shift-C hotkey.
- `hotkeys`: now supports printing active hotkeys to the console with ``hotkeys list``
- `digtype`: new ``-z`` option for digtype to restrict designations to the current z-level and down
- UX: List widgets now have mouse-interactive scrollbars
- UX: You can now hold down the mouse button on a scrollbar to make it scroll multiple times.
- UX: You can now drag the scrollbar up and down to scroll to a specific spot
- `orders`: replace shell craft orders in the standard orders list you get with ``orders import library/basic``  with orders for shell leggings. They have a slightly higher trade price. Also, "shleggings" is just hilarious.
- `spectate`: new ``auto-unpause`` option for auto-dismissal of announcement pause events (e.g. sieges).
- `spectate`: new ``auto-disengage`` option for auto-disengagement of plugin through player interaction whilst unpaused.
- `spectate`: new ``tick-threshold`` option for specifying the maximum time to follow the same dwarf
- `spectate`: new ``animals`` option for sometimes following animals
- `spectate`: new ``hostiles`` option for sometimes following hostiles
- `spectate`: new ``visiting`` option for sometimes following visiting merchants, diplomats or plain visitors
- `spectate`: added persistent configuration of the plugin settings
- `gui/cp437-table`: new global keybinding for the clickable on-screen keyboard for players with keyboard layouts that prevent them from using certain keys: Ctrl-Shift-K
- `quickfort-library-guide`: improved layout of marksdwarf barracks in the example Dreamfort blueprints

Documentation
-------------
- `overlay-dev-guide`: documentation and guide for injecting functionality into DF viewscreens from Lua scripts and creating interactive overlay widgets
- ``dfhack.gui.revealInDwarfmodeMap``: document ``center`` bool for Lua API

API
---
- ``Gui::anywhere_hotkey``: for plugin commands bound to keybindings that can be invoked on any screen
- ``Gui::autoDFAnnouncement``, ``Gui::pauseRecenter``: added functionality reverse-engineered from announcement code
- ``Gui::revealInDwarfmodeMap``: Now enforce valid view bounds when pos invalid, add variant accepting x, y, z
- ``Lua::PushInterfaceKeys()``: transforms viewscreen ``feed()`` keys into something that can be interpreted by lua-based widgets
- ``Lua::Push()``: now handles maps with otherwise supported keys and values
- Units module: added new predicates for:
  - ``isUnitInBox()``
  - ``isAnimal()``
  - ``isVisiting()`` any visiting unit (diplomat, merchant, visitor)
  - ``isVisitor()`` ie. not merchants or diplomats
  - ``isInvader()``
  - ``isDemon()`` returns true for unique/regular demons
  - ``isTitan()``
  - ``isMegabeast()``
  - ``isGreatDanger()`` returns true if unit is a demon, titan, or megabeast
  - ``isSemiMegabeast()``
  - ``isNightCreature()``
  - ``isDanger()`` returns true if is a 'GreatDanger', semi-megabeast, night creature, undead, or invader
- Units module: modified predicates:
  - ``isUndead()`` now optionally ignores vampires instead of always ignoring vampires
  - ``isCitizen()`` now optionally ignores insane citizens instead of always ignoring insane citizens
- Units module: new action timer API for speeding up of slowing down units
- Constructions module: added ``insert()`` to insert constructions into the game's sorted list.
- MiscUtils: added the following string transformation functions (refactored from ``uicommon.h``): ``int_to_string``, ``ltrim``, ``rtrim``, and ``trim``; added ``string_to_int``

Lua
---
- ``widgets.Scrollbar``: new scrollbar widget that can be paired with an associated scrollable widget. Integrated with ``widgets.Label`` and ``widgets.List``.
- ``widgets.List``: new ``getIdxUnderMouse()`` function for detecting the list index under the active mouse cursor. this allows for "selection follows mouse" behavior
- ``widgets.List``: shift-clicking now triggers the ``submit2`` attribute function if it is defined
- ``dfhack.constructions.findAtTile()``: exposed preexisting function to Lua.
- ``dfhack.constructions.insert()``: exposed new function to Lua.
- ``widgets.Panel``: new ``frame_style`` and ``frame_title`` attributes for drawing frames around groups of widgets
- ``widgets.EditField``: now allows other widgets to process characters that the ``on_char`` callback rejects.
- ``widgets.FilteredList``: now provides a useful default search key for list items made up of text tokens instead of plain text
- ``widgets.ResizingPanel``: now accounts for frame inset when calculating frame size
- ``widgets.HotkeyLabel``: now ignores mouse clicks when ``on_activate`` is not defined
- ``gui.Screen.show()``: now returns ``self`` as a convenience
- ``gui.View.getMousePos()`` now takes an optional ``ViewRect`` parameter in case the caller wants to get the mouse pos relative to a rect that is not the frame_body (such as the frame_rect that includes the frame itself)
- Lua mouse events now conform to documented behavior in `lua-api` -- ``_MOUSE_L_DOWN`` will be sent exactly once per mouse click and ``_MOUSE_L`` will be sent repeatedly as long as the button is held down. Similarly for right mouse button events.

Internals
---------
- Constructions module: ``findAtTile`` now uses a binary search intead of a linear search
- MSVC warning level upped to /W3, and /WX added to make warnings cause compilations to fail.

Removed
-------
- `resume`: functionality (including suspended building overlay) has moved to `unsuspend`

0.47.05-r7
==========

New Plugins
-----------
- `autonestbox`: split off from `zone` into its own plugin. Note that to enable, the command has changed from ``autonestbox start`` to ``enable autonestbox``.
- `autobutcher`: split off from `zone` into its own plugin. Note that to enable, the command has changed from ``autobutcher start`` to ``enable autobutcher``.
- `overlay`: display a "DFHack" button in the lower left corner that you can click to start the new GUI command launcher. The `dwarfmonitor` weather display had to be moved to make room for the button. If you are seeing the weather indicator rendered over the overlay button, please remove the ``dfhack-config/dwarfmonitor.json`` file to fix the weather indicator display offset.

New Internal Commands
---------------------
- `tags`: new built-in command to list the tool category tags and their definitions. tags associated with each tool are visible in the tool help and in the output of `ls`.

Fixes
-----
- `autochop`: designate largest trees for chopping first, instead of the smallest
- ``dfhack.run_script``: ensure the arguments passed to scripts are always strings. This allows other scripts to call ``run_script`` with numeric args and it won't break parameter parsing.
- `dig-now`: Fix direction of smoothed walls when adjacent to a door or floodgate
- ``job.removeJob()``: ensure jobs are removed from the world list when they are canceled
- `quickfort`: `Dreamfort <quickfort-blueprint-guide>` blueprint set: declare the hospital zone before building the coffer; otherwise DF fails to stock the hospital with materials
- ``dfhack.buildings.findCivzonesAt``: no longer return duplicate civzones after loading a save with existing civzones

Misc Improvements
-----------------
- Init scripts: ``dfhack.init`` and other init scripts have moved to ``dfhack-config/init/``. If you have customized your ``dfhack.init`` file and want to keep your changes, please move the part that you have customized to the new location at ``dfhack-config/init/dfhack.init``. If you do not have changes that you want to keep, do not copy anything, and the new defaults will be used automatically.
- History files: ``dfhack.history``, ``tiletypes.history``, ``lua.history``, and ``liquids.history`` have moved to the ``dfhack-config`` directory. If you'd like to keep the contents of your current history files, please move them to ``dfhack-config``.
- `do-job-now`: new global keybinding for boosting the priority of the jobs associated with the selected building/work order/unit/item etc.: Alt-N
- `gui/workorder-details`: new keybinding on the workorder details screen: ``D``
- `keybinding`: support backquote (\`) as a hotkey (and assign the hotkey to the new `gui/launcher` interface)
- `ls`: can now filter tools by substring or tag. note that dev scripts are hidden by default. pass the ``--dev`` option to show them.
- `manipulator`: add a library of useful default professions
- `manipulator`: move professions configuration from ``professions/`` to ``dfhack-config/professions/`` to keep it together with other dfhack configuration. If you have saved professions that you would like to keep, please manually move them to the new folder.
- ``materials.ItemTraitsDialog``: added a default ``on_select``-handler which toggles the traits.
- `orders`: added useful library of manager orders. see them with ``orders list`` and import them with, for example, ``orders import library/basic``
- `prospect`: add new ``--show`` option to give the player control over which report sections are shown. e.g. ``prospect all --show ores`` will just show information on ores.
- `seedwatch`: ``seedwatch all`` now adds all plants with seeds to the watchlist, not just the "basic" crops.
- UX: You can now move the cursor around in DFHack text fields in ``gui/`` scripts (e.g. `gui/blueprint`, `gui/quickfort`, or `gui/gm-editor`). You can move the cursor by clicking where you want it to go with the mouse or using the Left/Right arrow keys.  Ctrl+Left/Right will move one word at a time, and Alt+Left/Right will move to the beginning/end of the text.
- UX: You can now click on the hotkey hint text in many ``gui/`` script windows to activate the hotkey, like a button. Not all scripts have been updated to use the clickable widget yet, but you can try it in `gui/blueprint` or `gui/quickfort`.
- UX: Label widget scroll icons are replaced with scrollbars that represent the percentage of text on the screen and move with the position of the visible text, just like web browser scrollbars.
- `quickfort`: `Dreamfort <quickfort-blueprint-guide>` blueprint set improvements: set traffic designations to encourage dwarves to eat cooked food instead of raw ingredients

Documentation
-------------
- Added `modding-guide`
- Update all DFHack tool documentation (300+ pages) with standard syntax formatting, usage examples, and overall clarified text.
- Group DFHack tools by `tag <tools>` so similar tools are grouped and easy to find

API
---
- Removed "egg" ("eggy") hook support (Linux only). The only remaining method of hooking into DF is by interposing SDL calls, which has been the method used by all binary releases of DFHack.
- Removed ``Engravings`` module (C++-only). Access ``world.engravings`` directly instead.
- Removed ``Notes`` module (C++-only). Access ``ui.waypoints.points`` directly instead.
- Removed ``Windows`` module (C++-only) - unused.
- ``Constructions`` module (C++-only): removed ``t_construction``, ``isValid()``, ``getCount()``, ``getConstruction()``, and ``copyConstruction()``. Access ``world.constructions`` directly instead.
- ``Gui::getSelectedItem()``, ``Gui::getAnyItem()``: added support for the artifacts screen
- ``Units::teleport()``: now sets ``unit.idle_area`` to discourage units from walking back to their original location (or teleporting back, if using `fastdwarf`)

Lua
---
- History: added ``dfhack.getCommandHistory(history_id, history_filename)`` and ``dfhack.addCommandToHistory(history_id, history_filename, command)`` so gui scripts can access a commandline history without requiring a terminal.
- Added ``dfhack.screen.hideGuard()``: exposes the C++ ``Screen::Hide`` to Lua
- ``helpdb``: database and query interface for DFHack tool help text
- ``tile-material``: fix the order of declarations. The ``GetTileMat`` function now returns the material as intended (always returned nil before). Also changed the license info, with permission of the original author.
- ``utils.df_expr_to_ref()``: fixed some errors that could occur when navigating tables
- ``widgets.EditField``: new ``onsubmit2`` callback attribute is called when the user hits Shift-Enter.
- ``widgets.EditField``: new function: ``setCursor(position)`` sets the input cursor.
- ``widgets.EditField``: new attribute: ``ignore_keys`` lets you ignore specified characters if you want to use them as hotkeys
- ``widgets.FilteredList``: new attribute: ``edit_ignore_keys`` gets passed to the filter EditField as ``ignore_keys``
- ``widgets.Label``: ``scroll`` function now interprets the keywords ``+page``, ``-page``, ``+halfpage``, and ``-halfpage`` in addition to simple positive and negative numbers.
- ``widgets.HotkeyLabel``: clicking on the widget will now call ``on_activate()``.
- ``widgets.CycleHotkeyLabel``: clicking on the widget will now cycle the options and trigger ``on_change()``. This also applies to the ``ToggleHotkeyLabel`` subclass.

0.47.05-r6
==========

Fixes
-----
- `eventful`: fix ``eventful.registerReaction`` to correctly pass ``call_native`` argument thus allowing canceling vanilla item creation. Updated related documentation.
- `eventful`: renamed NEW_UNIT_ACTIVE event to UNIT_NEW_ACTIVE to match the ``EventManager`` event name
- `eventful`: fixed UNIT_NEW_ACTIVE event firing too often
- ``job.removeJob()``: fixes regression in DFHack 0.47.05-r5 where items/buildings associated with the job were not getting disassociated when the job is removed. Now `build-now` can build buildings and `gui/mass-remove` can cancel building deconstruction again
- ``widgets.CycleHotkeyLabel``: allow initial option values to be specified as an index instead of an option value

Misc Improvements
-----------------
- `confirm`: added a confirmation dialog for removing manager orders
- `confirm`: allow players to pause the confirmation dialog until they exit the current screen
- `dfhack-examples-guide`: refine food preparation orders so meal types are chosen intelligently according to the amount of meals that exist and the number of aviailable items to cook with
- `dfhack-examples-guide`: reduce required stock of dye for "Dye cloth" orders
- `dfhack-examples-guide`: fix material conditions for making jugs and pots
- `dfhack-examples-guide`: make wooden jugs by default to differentiate them from other stone tools. this allows players to more easily select jugs out with a properly-configured stockpile (i.e. the new ``woodentools`` alias)
- `quickfort-alias-guide`: new aliases: ``forbidsearch``, ``permitsearch``, and ``togglesearch`` use the `search-plugin` plugin to alter the settings for a filtered list of item types when configuring stockpiles
- `quickfort-alias-guide`: new aliases: ``stonetools`` and ``woodentools``.  the ``jugs`` alias is deprecated. please use ``stonetools`` instead, which is the same as the old ``jugs`` alias.
- `quickfort-alias-guide`: new aliases: ``usablehair``, ``permitusablehair``, and ``forbidusablehair`` alter settings for the types of hair/wool that can be made into cloth: sheep, llama, alpaca, and troll. The ``craftrefuse`` aliases have been altered to use this alias as well.
- `quickfort-alias-guide`: new aliases: ``forbidthread``, ``permitthread``, ``forbidadamantinethread``, ``permitadamantinethread``, ``forbidcloth``, ``permitcloth``, ``forbidadamantinecloth``, and ``permitadamantinecloth`` give you more control how adamantine-derived items are stored
- `quickfort`: `Dreamfort <quickfort-blueprint-guide>` blueprint set improvements: automatically create tavern, library, and temple locations (restricted to residents only by default), automatically associate the rented rooms with the tavern
- `quickfort`: `Dreamfort <quickfort-blueprint-guide>` blueprint set improvements: new design for the services level, including were-bitten hospital recovery rooms and an appropriately-themed interrogation room next to the jail! Also fits better in a 1x1 embark for minimalist players.

API
---
- ``word_wrap``: argument ``bool collapse_whitespace`` converted to enum ``word_wrap_whitespace_mode mode``, with valid modes ``WSMODE_KEEP_ALL``, ``WSMODE_COLLAPSE_ALL``, and ``WSMODE_TRIM_LEADING``.

Lua
---
- ``gui.View``: all ``View`` subclasses (including all ``Widgets``) can now acquire keyboard focus with the new ``View:setFocus()`` function. See docs for details.
- ``materials.ItemTraitsDialog``: new dialog to edit item traits (where "item" is part of a job or work order or similar). The list of traits is the same as in vanilla work order conditions "``t`` change traits".
- ``widgets.EditField``: the ``key_sep`` string is now configurable
- ``widgets.EditField``: can now display an optional string label in addition to the activation key
- ``widgets.EditField``: views that have an ``EditField`` subview no longer need to manually manage the ``EditField`` activation state and input routing.  This is now handled automatically by the new ``gui.View`` keyboard focus subsystem.
- ``widgets.HotkeyLabel``: the ``key_sep`` string is now configurable

0.47.05-r5
==========

New Plugins
-----------
- `spectate`: "spectator mode" -- automatically follows dwarves doing things in your fort

New Tweaks
----------
- `tweak`: ``partial-items`` displays percentage remaining for partially-consumed items such as hospital cloth

Fixes
-----
- `autofarm`: removed restriction on only planting "discovered" plants
- `cxxrandom`: fixed exception when calling ``bool_distribution``
- `luasocket`: return correct status code when closing socket connections so clients can know when to retry

Misc Improvements
-----------------
- `autochop`: only designate the amount of trees required to reach ``max_logs``
- `autochop`: preferably designate larger trees over smaller ones
- `blueprint`: ``track`` phase renamed to ``carve``
- `blueprint`: carved fortifications and (optionally) engravings are now captured in generated blueprints
- `cursecheck`: new option, ``--ids`` prints creature and race IDs of the cursed creature
- `debug`: DFHack log messages now have configurable headers (e.g. timestamp, origin plugin name, etc.) via the ``debugfilter`` command of the `debug` plugin
- `debug`: script execution log messages (e.g. "Loading script: dfhack_extras.init" can now be controlled with the ``debugfilter`` command. To hide the messages, add this line to your ``dfhack.init`` file: ``debugfilter set Warning core script``
- `dfhack-examples-guide`: add mugs to ``basic`` manager orders
- `dfhack-examples-guide`: ``onMapLoad_dreamfort.init`` remove "cheaty" commands and new tweaks that are now in the default ``dfhack.init-example`` file
- ``dfhack.init-example``: recently-added tweaks added to example ``dfhack.init`` file
- `dig-now`: handle fortification carving
- `EventManager`: add new event type ``JOB_STARTED``, triggered when a job first gains a worker
- `EventManager`: add new event type ``UNIT_NEW_ACTIVE``, triggered when a new unit appears on the active list
- `gui/create-item`: Added "(chain)" annotation text for armours with the [CHAIN_METAL_TEXT] flag set
- `manipulator`: tweak colors to make the cursor easier to locate
- `stocks`: allow search terms to match the full item label, even when the label is truncated for length
- `tweak`: ``stable-cursor`` now keeps the cursor stable even when the viewport moves a small amount

Documentation
-------------
- add more examples to the plugin example skeleton files so they are more informative for a newbie
- `confirm`: correct the command name in the plugin help text
- `cxxrandom`: added usage examples
- ``Lua API.rst``:  added ``isHidden(unit)``, ``isFortControlled(unit)``, ``getOuterContainerRef(unit)``, ``getOuterContainerRef(item)``
- `lua-string`: document DFHack string extensions (``startswith()``, ``endswith()``, ``split()``, ``trim()``, ``wrap()``, and ``escape_pattern()``)
- `quickfort-blueprint-guide`: added screenshots to the Dreamfort case study and overall clarified text
- `remote-client-libs`: add new Rust client library
- update download link and installation instructions for Visual C++ 2015 build tools on Windows
- update information regarding obtaining a compatible Windows build environment

API
---
- add functions reverse-engineered from ambushing unit code: ``Units::isHidden()``, ``Units::isFortControlled()``, ``Units::getOuterContainerRef()``, ``Items::getOuterContainerRef()``
- ``Job::removeJob()``: use the job cancel vmethod graciously provided by The Toady One in place of a synthetic method derived from reverse engineering

Lua
---
- `custom-raw-tokens`: library for accessing tokens added to raws by mods
- ``dfhack.units``: Lua wrappers for functions reverse-engineered from ambushing unit code: ``isHidden(unit)``, ``isFortControlled(unit)``, ``getOuterContainerRef(unit)``, ``getOuterContainerRef(item)``
- ``dialogs``: ``show*`` functions now return a reference to the created dialog
- ``dwarfmode.enterSidebarMode()``: passing ``df.ui_sidebar_mode.DesignateMine`` now always results in you entering ``DesignateMine`` mode and not ``DesignateChopTrees``, even when you looking at the surface (where the default designation mode is ``DesignateChopTrees``)
- ``dwarfmode.MenuOverlay``: if ``sidebar_mode`` attribute is set, automatically manage entering a specific sidebar mode on show and restoring the previous sidebar mode on dismiss
- ``dwarfmode.MenuOverlay``: new class function ``renderMapOverlay`` to assist with painting tiles over the visible map
- ``ensure_key``: new global function for retrieving or dynamically creating Lua table mappings
- ``safe_index``: now properly handles lua sparse tables that are indexed by numbers
- ``string``: new function ``escape_pattern()`` escapes regex special characters within a string
- ``widgets``: unset values in ``frame_inset`` table default to ``0``
- ``widgets``: ``FilteredList`` class now allows all punctuation to be typed into the filter and can match search keys that start with punctuation
- ``widgets``: minimum height of ``ListBox`` dialog is now calculated correctly when there are no items in the list (e.g. when a filter doesn't match anything)
- ``widgets``: if ``autoarrange_subviews`` is set, ``Panel``\s will now automatically lay out widgets vertically according to their current height.  This allows you to have widgets dynamically change height or become visible/hidden and you don't have to worry about recalculating frame layouts
- ``widgets``: new class ``ResizingPanel`` (subclass of ``Panel``) automatically recalculates its own frame height based on the size, position, and visibility of its subviews
- ``widgets``: new class ``HotkeyLabel`` (subclass of ``Label``) that displays and reacts to hotkeys
- ``widgets``: new class ``CycleHotkeyLabel`` (subclass of ``Label``) allows users to cycle through a list of options by pressing a hotkey
- ``widgets``: new class ``ToggleHotkeyLabel`` (subclass of ``CycleHotkeyLabel``) toggles between ``On`` and ``Off`` states
- ``widgets``:  new class ``WrappedLabel`` (subclass of ``Label``) provides autowrapping of text
- ``widgets``:  new class ``TooltipLabel`` (subclass of ``WrappedLabel``) provides tooltip-like behavior

0.47.05-r4
==========

Fixes
-----
- `blueprint`: fixed passing incorrect parameters to `gui/blueprint` when you run ``blueprint gui`` with optional params
- `blueprint`: key sequences for constructed walls and down stairs are now correct
- `tailor`: fixed some inconsistencies (and possible crashes) when parsing certain subcommands, e.g. ``tailor help``
- `tiletypes-here`, `tiletypes-here-point`: fix crash when running from an unsuspended core context

Misc Improvements
-----------------
- Core: DFHack now prints the name of the init script it is running to the console and stderr
- `automaterial`: ensure construction tiles are laid down in order when using `buildingplan` to plan the constructions
- `blueprint`: all blueprint phases are now written to a single file, using `quickfort` multi-blueprint file syntax. to get the old behavior of each phase in its own file, pass the ``--splitby=phase`` parameter to ``blueprint``
- `blueprint`: you can now specify the position where the cursor should be when the blueprint is played back with `quickfort` by passing the ``--playback-start`` parameter
- `blueprint`: generated blueprints now have labels so `quickfort` can address them by name
- `blueprint`: all building types are now supported
- `blueprint`: multi-type stockpiles are now supported
- `blueprint`: non-rectangular stockpiles and buildings are now supported
- `blueprint`: blueprints are no longer generated for phases that have nothing to do (unless those phases are explicitly enabled on the commandline or gui)
- `blueprint`: new "track" phase that discovers and records carved tracks
- `blueprint`: new "zone" phase that discovers and records activity zones, including custom configuration for ponds, gathering, and hospitals
- `dig-now`: no longer leaves behind a designated tile when a tile was designated beneath a tile designated for channeling
- `orders`: added ``list`` subcommand to show existing exported orders
- `quickfort`, `dfhack-examples-guide`: Dreamfort blueprint set improvements based on playtesting and feedback. includes updated profession definitions.
- `quickfort-library-guide`: added light aquifer tap and pump stack blueprints (with step-by-step usage guides) to the quickfort blueprint library
- `quickfort`: Dreamfort blueprint set improvements: added iron and flux stock level indicators on the industry level and a prisoner processing quantum stockpile in the surface barracks. also added help text for how to manage sieges and how to manage prisoners after a siege.

API
---
- ``Buildings::findCivzonesAt()``: lookups now complete in constant time instead of linearly scanning through all civzones in the game

Lua
---
- ``argparse.processArgsGetopt()``: you can now have long form parameters that are not an alias for a short form parameter. For example, you can now have a parameter like ``--longparam`` without needing to have an equivalent one-letter ``-l`` param.
- ``dwarfmode.enterSidebarMode()``: ``df.ui_sidebar_mode.DesignateMine`` is now a suported target sidebar mode

Removed
-------
- `fortplan`: please use `quickfort` instead

0.47.05-r3
==========

New Plugins
-----------
- `dig-now`: instantly completes dig designations (including smoothing and carving tracks)

Fixes
-----
- Core: ``alt`` keydown state is now cleared when DF loses and regains focus, ensuring the ``alt`` modifier state is not stuck on for systems that don't send standard keyup events in response to ``alt-tab`` window manager events
- Lua: ``memscan.field_offset()``: fixed an issue causing `devel/export-dt-ini` to crash sometimes, especially on Windows
- `autofarm`: autofarm will now count plant growths as well as plants toward its thresholds
- `autogems`: no longer assigns gem cutting jobs to workshops with gem cutting prohibited in the workshop profile

Misc Improvements
-----------------
- `buildingplan`: now displays which items are attached and which items are still missing for planned buildings
- `orders`: support importing and exporting reaction-specific item conditions, like "lye-containing" for soap production orders
- `orders`: new ``sort`` command. sorts orders according to their repeat frequency. this prevents daily orders from blocking other orders for simlar items from ever getting completed.
- `tiletypes-here`, `tiletypes-here-point`: add ``--cursor`` and ``--quiet`` options to support non-interactive use cases
- `quickfort`: Dreamfort blueprint set improvements: extensive revision based on playtesting and feedback. includes updated ``onMapLoad_dreamfort.init`` settings file, enhanced automation orders, and premade profession definitions.  see full changelog at https://github.com/DFHack/dfhack/pull/1921 and https://github.com/DFHack/dfhack/pull/1925
- `tailor`: allow user to specify which materials to be used, and in what order

API
---
- The ``Items`` module ``moveTo*`` and ``remove`` functions now handle projectiles

Lua
---
- new global function: ``safe_pairs(iterable[, iterator_fn])`` will iterate over the ``iterable`` (a table or iterable userdata)  with the ``iterator_fn`` (``pairs`` if not otherwise specified) if iteration is possible. If iteration is not possible or would throw an error, for example if ``nil`` is passed as the ``iterable``, the iteration is just silently skipped.

Documentation
-------------
- `quickfort-library-guide`: updated dreamfort documentation and added screenshots
- `dfhack-examples-guide`: documentation for all of `dreamfort`'s supporting files (useful for all forts, not just Dreamfort!)

0.47.05-r2
==========

Fixes
-----
- Fixed an issue where scrollable text in Lua-based screens could prevent other widgets from scrolling
- Fixed an issue preventing some external scripts from creating zones and other abstract buildings (see note about room definitions under "Internals")
- `buildingplan`: fixed an issue where planned constructions designated with DF's sizing keys (``umkh``) would sometimes be larger than requested
- `buildingplan`: fixed an issue preventing other plugins like `automaterial` from planning constructions if the "enable all" buildingplan setting was turned on
- `buildingplan`: made navigation keys work properly in the materials selection screen when alternate keybindings are used
- `command-prompt`: fixed issues where overlays created by running certain commands (e.g. `gui/liquids`, `gui/teleport`) would not update the parent screen correctly
- `dwarfvet`: fixed a crash that could occur with hospitals overlapping with other buildings in certain ways
- `orders`: fixed crash when importing orders with malformed IDs
- ``quickfortress.csv`` blueprint: fixed refuse stockpile config and prevented stockpiles from covering stairways
- `stonesense`: fixed a crash that could occur when ctrl+scrolling or closing the Stonesense window
- `embark-assistant`: fixed faulty early exit in first search attempt when searching for waterfalls

Misc Improvements
-----------------
- Added adjectives to item selection dialogs, used in tools like `gui/create-item` - this makes it possible to differentiate between different types of high/low boots, shields, etc. (some of which are procedurally generated)
- `blueprint`: made ``depth`` and ``name`` parameters optional. ``depth`` now defaults to ``1`` (current level only) and ``name`` defaults to "blueprint"
- `blueprint`: ``depth`` can now be negative, which will result in the blueprints being written from the highest z-level to the lowest. Before, blueprints were always written from the lowest z-level to the highest.
- `blueprint`: added the ``--cursor`` option to set the starting coordinate for the generated blueprints. A game cursor is no longer necessary if this option is used.
- `quickfort`: the Dreamfort blueprint set can now be comfortably built in a 1x1 embark
- `stonesense`: sped up startup time
- `tweak` hide-priority: changed so that priorities stay hidden (or visible) when exiting and re-entering the designations menu
- `embark-assistant`: slightly improved performance of surveying and improved code a little

Lua
---
- new string utility functions:
    - ``string:wrap(width)`` wraps a string at space-separated word boundaries
    - ``string:trim()`` removes whitespace characters from the beginning and end of the string
    - ``string:split(delimiter, plain)`` splits a string with the given delimiter and returns a table of substrings. if ``plain`` is specified and set to ``true``, ``delimiter`` is interpreted as a literal string instead of as a pattern (the default)
- new library: ``argparse`` is a collection of commandline argument processing functions
- ``gui.Painter``: fixed error when calling ``viewport()`` method
- ``gui.dwarfmode``: new function: ``enterSidebarMode(sidebar_mode, max_esc)`` which uses keypresses to get into the specified sidebar mode from whatever the current screen is
- `reveal`: now exposes ``unhideFlood(pos)`` functionality to Lua
- new utility function: ``utils.normalizePath()``: normalizes directory slashes across platoforms to ``/`` and coaleses adjacent directory separators
- ``argparse.processArgsGetopt()`` (previously ``utils.processArgsGetopt()``):
    - now returns negative numbers (e.g. ``-10``) in the list of positional parameters instead of treating it as an option string equivalent to ``-1 -0``
    - now properly handles ``--`` like GNU ``getopt`` as a marker to treat all further parameters as non-options
    - now detects when required arguments to long-form options are missing
- `xlsxreader`: added Lua class wrappers for the xlsxreader plugin API

API
---
- Added ``dfhack.units.teleport(unit, pos)``
- Added ``dfhack.maps.getPlantAtTile(x, y, z)`` and ``dfhack.maps.getPlantAtTile(pos)``, and updated ``dfhack.gui.getSelectedPlant()`` to use it

Documentation
-------------
- Added more client library implementations to the `remote interface docs <remote-client-libs>`

Internals
---------
- The DFHack test harness is now much easier to use for iterative development.  Configuration can now be specified on the commandline, there are more test filter options, and the test harness can now easily rerun tests that have been run before.
- The ``test/main`` command to invoke the test harness has been renamed to just ``test``
- Unit tests must now match any output expected to be printed via ``dfhack.printerr()``
- Unit tests now support fortress mode (allowing tests that require a fortress map to be loaded) - note that these tests are skipped by continuous integration for now, pending a suitable test fortress
- Unit tests can now use ``delay_until(predicate_fn, timeout_frames)`` to delay until a condition is met
- Room definitions and extents are now created for abstract buildings so callers don't have to initialize the room structure themselves

0.47.05-r1
==========

Fixes
-----
- `confirm`: stopped exposing alternate names when convicting units
- `prospector`: improved pre embark rough estimates, particularly for small clusters

Misc Improvements
-----------------
- `autohauler`: allowed the ``Alchemist`` labor to be enabled in `manipulator` and other labor screens so it can be used for its intended purpose of flagging that no hauling labors should be assigned to a dwarf. Before, the only way to set the flag was to use an external program like Dwarf Therapist.
- `embark-assistant`: slightly improved performance of surveying
- `quickfort`: Dreamfort blueprint set improvements: `significant <http://www.bay12forums.com/smf/index.php?topic=176889.msg8239017#msg8239017>`_ refinements across the entire blueprint set. Dreamfort is now much faster, much more efficient, and much easier to use. The `checklist <https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=1459509569>`__ now includes a mini-walkthrough for quick reference. The spreadsheet now also includes `embark profile suggestions <https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=149144025>`__
- `quickfort`: added aliases for configuring masterwork and artifact core quality for all stockpile categories that have them; made it possible to take from multiple stockpiles in the ``quantumstop`` alias

Documentation
-------------
- `fortplan`: added deprecation warnings - fortplan has been replaced by `quickfort`

0.47.05-beta1
=============

Fixes
-----
- `embark-assistant`: fixed bug in soil depth determination for ocean tiles
- `orders`: don't crash when importing orders with malformed JSON

0.47.04-r5
==========

Fixes
-----
- `embark-assistant`: fixed order of factors when calculating min temperature
- `embark-assistant`: improved performance of surveying
- `quickfort`: fixed eventual crashes when creating zones
- `quickfort`: fixed library aliases for tallow and iron, copper, and steel weapons
- `seedwatch`: fixed an issue where the plugin would disable itself on map load
- `search`: fixed crash when searching the ``k`` sidebar and navigating to another tile with certain keys, like ``<`` or ``>``
- `stockflow`: fixed ``j`` character being intercepted when naming stockpiles
- `stockpiles`: no longer outputs hotkey help text beneath `stockflow` hotkey help text

Misc Improvements
-----------------
- Lua label widgets (used in all standard message boxes) are now scrollable with Up/Down/PgUp/PgDn keys
- `autofarm`: now fallows farms if all plants have reached the desired count
- `buildingplan`: added ability to set global settings from the console, e.g.  ``buildingplan set boulders false``
- `buildingplan`: added "enable all" option for buildingplan (so you don't have to enable all building types individually). This setting is not persisted (just like quickfort_mode is not persisted), but it can be set from onMapLoad.init
- `buildingplan`: modified ``Planning Mode`` status in the UI to show whether the plugin is in quickfort mode, "enable all" mode, or whether just the building type is enabled.
- `quickfort`: Dreamfort blueprint set improvements: added a streamlined checklist for all required dreamfort commands and gave names to stockpiles, levers, bridges, and zones
- `quickfort`: added aliases for bronze weapons and armor
- `quickfort`: added alias for tradeable crafts

Lua
---
- ``dfhack.run_command()``: changed to interface directly with the console when possible, which allows interactive commands and commands that detect the console encoding to work properly
- ``processArgsGetopt()`` added to utils.lua, providing a callback interface for parameter parsing and getopt-like flexibility for parameter ordering and combination (see docs in ``library/lua/utils.lua`` and ``library/lua/3rdparty/alt_getopt.lua`` for details).

Documentation
-------------
- Added documentation for Lua's ``dfhack.run_command()`` and variants

0.47.04-r4
==========

Fixes
-----
- Fixed an issue on some Linux systems where DFHack installed through a package manager would attempt to write files to a non-writable folder (notably when running `exportlegends` or `gui/autogems`)
- `buildingplan`: fixed an issue preventing artifacts from being matched when the maximum item quality is set to ``artifacts``
- `buildingplan`: stopped erroneously matching items to buildings while the game is paused
- `buildingplan`: fixed a crash when pressing 0 while having a noble room selected
- `dwarfvet`: fixed a crash that could occur when discharging patients
- `dwarfmonitor`: fixed a crash when opening the ``prefs`` screen if units have vague preferences
- `embark-assistant`: fixed an issue causing incursion resource matching (e.g. sand/clay) to skip some tiles if those resources were provided only through incursions
- `embark-assistant`: corrected river size determination by performing it at the MLT level rather than the world tile level
- `search`: fixed an issue where search options might not display if screens were destroyed and recreated programmatically (e.g. with `quickfort`)
- `workflow`: fixed an error when creating constraints on "mill plants" jobs and some other plant-related jobs
- `zone`: fixed an issue causing the ``enumnick`` subcommand to run when attempting to run ``assign``, ``unassign``, or ``slaughter``

Misc Improvements
-----------------
- `buildingplan`: added support for all buildings, furniture, and constructions (except for instruments)
- `buildingplan`: added support for respecting building job_item filters when matching items, so you can set your own programmatic filters for buildings before submitting them to buildingplan
- `buildingplan`: changed default filter setting for max quality from ``artifact`` to ``masterwork``
- `buildingplan`: changed min quality adjustment hotkeys from 'qw' to 'QW' to avoid conflict with existing hotkeys for setting roller speed - also changed max quality adjustment hotkeys from 'QW' to 'AS' to make room for the min quality hotkey changes
- `buildingplan`: added a new global settings page accessible via the ``G`` hotkey when on any building build screen; ``Quickfort Mode`` toggle for legacy Python Quickfort has been moved to this page
- `buildingplan`: added new global settings for whether generic building materials should match blocks, boulders, logs, and/or bars - defaults are everything but bars
- `embark-assistant`: split the lair types displayed on the local map into mound, burrow, and lair
- `probe`: added more output for designations and tile occupancy
- `quickfort`: The Dreamfort sample blueprints now have complete walkthroughs for each fort level and importable orders that automate basic fort stock management
- `quickfort`: added more blueprints to the blueprints library: several bedroom layouts, the Saracen Crypts, and the complete fortress example from Python Quickfort: TheQuickFortress

Documentation
-------------
- `quickfort-alias-guide`: alias syntax and alias standard library documentation for `quickfort` blueprints
- `quickfort-library-guide`: overview of the quickfort blueprint library

API
---
- `buildingplan`: added Lua interface API
- ``dfhack.job.isSuitableMaterial()``: added an item type parameter so the ``non_economic`` flag can be properly handled (it was being matched for all item types instead of just boulders)
- ``Buildings::setSize()``: changed to reuse existing extents when possible

Lua
---
- ``utils.addressof()``: fixed for raw userdata

0.47.04-r3
==========

New Plugins
-----------
- `xlsxreader`: provides an API for Lua scripts to read Excel spreadsheets

Fixes
-----
- `buildingplan`: fixed handling of buildings that require buckets
- `getplants`: fixed a crash that could occur on some maps
- `search`: fixed an issue causing item counts on the trade screen to display inconsistently when searching
- `stockpiles`: fixed a crash when loading food stockpiles
- `stockpiles`: fixed an error when saving furniture stockpiles

Misc Improvements
-----------------
- `createitem`: added support for plant growths (fruit, berries, leaves, etc.)
- `createitem`: added an ``inspect`` subcommand to print the item and material tokens of existing items, which can be used to create additional matching items
- `embark-assistant`: added support for searching for taller waterfalls (up to 50 z-levels tall)
- `search`: added support for searching for names containing non-ASCII characters using their ASCII equivalents
- `stocks`: added support for searching for items containing non-ASCII characters using their ASCII equivalents
- `zone`: added an ``enumnick`` subcommand to assign enumerated nicknames (e.g "Hen 1", "Hen 2"...)
- `zone`: added slaughter indication to ``uinfo`` output

Documentation
-------------
- Fixed syntax highlighting of most code blocks to use the appropriate language (or no language) instead of Python

API
---
- Added ``DFHack::to_search_normalized()`` (Lua: ``dfhack.toSearchNormalized()``) to convert non-ASCII alphabetic characters to their ASCII equivalents

0.47.04-r2
==========

New Tweaks
----------
- `tweak` do-job-now: adds a job priority toggle to the jobs list
- `tweak` reaction-gloves: adds an option to make reactions produce gloves in sets with correct handedness

Fixes
-----
- Fixed a segfault when attempting to start a headless session with a graphical PRINT_MODE setting
- Fixed an issue with the macOS launcher failing to un-quarantine some files
- Linux: fixed ``dfhack.getDFPath()`` (Lua) and ``Process::getPath()`` (C++) to always return the DF root path, even if the working directory has changed
- `getplants`: fixed issues causing plants to be collected even if they have no growths (or unripe growths)
- `labormanager`: fixed handling of new jobs in 0.47
- `labormanager`: fixed an issue preventing custom furnaces from being built
- `embark-assistant`: fixed a couple of incursion handling bugs.
- Fixed ``Units::isEggLayer``, ``Units::isGrazer``, ``Units::isMilkable``, ``Units::isTrainableHunting``, ``Units::isTrainableWar``, and ``Units::isTamable`` ignoring the unit's caste
- `RemoteFortressReader`: fixed a couple crashes that could result from decoding invalid enum items (``site_realization_building_type`` and ``improvement_type``)
- `RemoteFortressReader`: fixed an issue that could cause block coordinates to be incorrect
- `rendermax`: fixed a hang that could occur when enabling some renderers, notably on Linux
- `stonesense`: fixed a crash when launching Stonesense
- `stonesense`: fixed some issues that could cause the splash screen to hang

Misc Improvements
-----------------
- Linux/macOS: Added console keybindings for deleting words (Alt+Backspace and Alt+d in most terminals)
- `blueprint`: now writes blueprints to the ``blueprints/`` subfolder instead of the df root folder
- `blueprint`: now automatically creates folder trees when organizing blueprints into subfolders (e.g. ``blueprint 30 30 1 rooms/dining dig`` will create the file ``blueprints/rooms/dining-dig.csv``); previously it would fail if the ``blueprints/rooms/`` directory didn't already exist
- `confirm`: added a confirmation dialog for convicting dwarves of crimes
- `manipulator`: added a new column option to display units' goals

API
---
- Added ``Filesystem::mkdir_recursive``
- Extended ``Filesystem::listdir_recursive`` to optionally make returned filenames relative to the start directory
- ``Units``: added goal-related functions: ``getGoalType()``, ``getGoalName()``, ``isGoalAchieved()``

Internals
---------
- Added support for splitting scripts into multiple files in the ``scripts/internal`` folder without polluting the output of `ls`

Lua
---
- Added a ``ref_target`` field to primitive field references, corresponding to the ``ref-target`` XML attribute
- Made ``dfhack.units.getRaceNameById()``, ``dfhack.units.getRaceBabyNameById()``, and ``dfhack.units.getRaceChildNameById()`` available to Lua

Ruby
----
- Updated ``item_find`` and ``building_find`` to use centralized logic that works on more screens

Documentation
-------------
- Expanded the installation guide
- Added some new dev-facing pages, including dedicated pages about the remote API, memory research, and documentation
- Made a couple theme adjustments

0.47.04-r1
==========

Fixes
-----
- Fixed translation of certain types of in-game names
- Fixed a crash in ``find()`` for some types when no world is loaded
- `autogems`: fixed an issue with binned gems being ignored in linked stockpiles
- `stocks`: fixed display of book titles
- `tweak` embark-profile-name: fixed handling of the native shift+space key

Misc Improvements
-----------------
- ``dfhack.init-example``: enabled `autodump`
- `getplants`: added switches for designations for farming seeds and for max number designated per plant
- `manipulator`: added intrigue to displayed skills
- `search`: added support for the fortress mode justice screen

API
---
- Added ``Items::getBookTitle`` to get titles of books. Catches titles buried in improvements, unlike getDescription.

Lua
---
- ``pairs()`` now returns available class methods for DF types

0.47.04-beta1
=============

Fixes
-----
- Fixed a crash when starting DFHack in headless mode with no terminal

Misc Improvements
-----------------
- Added "bit" suffix to downloads (e.g. 64-bit)
- Tests:
    - moved from DF folder to hack/scripts folder, and disabled installation by default
    - made test runner script more flexible
- `dfhack-run`: added color output support
- `embark-assistant`:
    - updated embark aquifer info to show all aquifer kinds present
    - added neighbor display, including kobolds (SKULKING) and necro tower count
    - updated aquifer search criteria to handle the new variation
    - added search criteria for embark initial tree cover
    - added search criteria for necro tower count, neighbor civ count, and specific neighbors. Should handle additional entities, but not tested

Internals
---------
- Improved support for tagged unions, allowing tools to access union fields more safely
- Added separate changelogs in the scripts and df-structures repos
- Moved ``reversing`` scripts to df_misc repo

0.47.03-beta1
=============

New Scripts
-----------
- `devel/sc`: checks size of structures
- `devel/visualize-structure`: displays the raw memory of a structure

Fixes
-----
- @ `adv-max-skills`: fixed for 0.47
- `deep-embark`:
    - prevented running in non-fortress modes
    - ensured that only the newest wagon is deconstructed
- `full-heal`:
    - fixed issues with removing corpses
    - fixed resurrection for non-historical figures
- @ `modtools/create-unit`: added handling for arena tame setting
- `teleport`: fixed setting new tile occupancy

Misc Improvements
-----------------
- `deep-embark`:
    - improved support for using directly from the DFHack console
    - added a ``-clear`` option to cancel
- `exportlegends`:
    - added identity information
    - added creature raw names and flags
- `gui/prerelease-warning`: updated links and information about nightly builds
- `modtools/syndrome-trigger`: enabled simultaneous use of ``-synclass`` and ``-syndrome``
- `repeat`: added ``-list`` option

Structures
----------
- Dropped support for 0.44.12-0.47.02
- ``abstract_building_type``: added types (and subclasses) new to 0.47
- ``agreement_details_type``: added enum
- ``agreement_details``: added struct type (and many associated data types)
- ``agreement_party``: added struct type
- ``announcement_type``: added types new to 0.47
- ``artifact_claim_type``: added enum
- ``artifact_claim``: added struct type
- ``breath_attack_type``: added ``SHARP_ROCK``
- ``building_offering_placest``: new class
- ``building_type``: added ``OfferingPlace``
- ``creature_interaction_effect``: added subclasses new to 0.47
- ``creature_raw_flags``: identified several more items
- ``creature_raw_flags``: renamed many items to match DF names
- ``caste_raw_flags``: renamed many items to match DF names
- ``d_init``: added settings new to 0.47
- ``entity_name_type``: added ``MERCHANT_COMPANY``, ``CRAFT_GUILD``
- ``entity_position_responsibility``: added values new to 0.47
- ``fortress_type``: added enum
- ``general_ref_type``: added ``UNIT_INTERROGATEE``
- ``ghost_type``: added ``None`` value
- ``goal_type``: added goals types new to 0.47
- ``histfig_site_link``: added subclasses new to 0.47
- ``history_event_collection``: added subtypes new to 0.47
- ``history_event_context``: added lots of new fields
- ``history_event_reason``: added captions for all items
- ``history_event_reason``: added items new to 0.47
- ``history_event_type``: added types for events new to 0.47, as well as corresponding ``history_event`` subclasses (too many to list here)
- ``honors_type``: added struct type
- ``interaction_effect``: added subtypes new to 0.47
- ``interaction_source_experimentst``: added class type
- ``interaction_source_usage_hint``: added values new to 0.47
- ``interface_key``: added items for keys new to 0.47
- ``job_skill``: added ``INTRIGUE``, ``RIDING``
- ``lair_type``: added enum
- ``monument_type``: added enum
- ``next_global_id``: added enum
- ``poetic_form_action``: added ``Beseech``
- ``setup_character_info``: expanded significantly in 0.47
- ``text_system``: added layout for struct
- ``tile_occupancy``: added ``varied_heavy_aquifer``
- ``tool_uses``: added items: ``PLACE_OFFERING``, ``DIVINATION``, ``GAMES_OF_CHANCE``
- ``viewscreen_counterintelligencest``: new class (only layout identified so far)

0.44.12-r3
==========

New Plugins
-----------
- `autoclothing`: automatically manage clothing work orders
- `autofarm`: replaces the previous Ruby script of the same name, with some fixes
- `map-render`: allows programmatically rendering sections of the map that are off-screen
- `tailor`: automatically manages keeping your dorfs clothed

New Scripts
-----------
- `assign-attributes`: changes the attributes of a unit
- `assign-beliefs`: changes the beliefs of a unit
- `assign-facets`: changes the facets (traits) of a unit
- `assign-goals`: changes the goals of a unit
- `assign-preferences`: changes the preferences of a unit
- `assign-profile`: sets a dwarf's characteristics according to a predefined profile
- `assign-skills`: changes the skills of a unit
- `combat-harden`: sets a unit's combat-hardened value to a given percent
- `deep-embark`: allows embarking underground
- `devel/find-twbt`: finds a TWBT-related offset needed by the new `map-render` plugin
- `dwarf-op`: optimizes dwarves for fort-mode work; makes managing labors easier
- `forget-dead-body`: removes emotions associated with seeing a dead body
- `gui/create-tree`: creates a tree at the selected tile
- `linger`: takes over your killer in adventure mode
- `modtools/create-tree`: creates a tree
- `modtools/pref-edit`: add, remove, or edit the preferences of a unit
- `modtools/set-belief`: changes the beliefs (values) of units
- `modtools/set-need`: sets and edits unit needs
- `modtools/set-personality`: changes the personality of units
- `modtools/spawn-liquid`: spawns water or lava at the specified coordinates
- `set-orientation`: edits a unit's orientation
- `unretire-anyone`: turns any historical figure into a playable adventurer

Fixes
-----
- Fixed a crash in the macOS/Linux console when the prompt was wider than the screen width
- Fixed some cases where Lua filtered lists would not properly intercept keys, potentially triggering other actions on the same screen
- Fixed inconsistent results from ``Units::isGay`` for asexual units
- `autofarm`:
    - fixed biome detection to properly determine crop assignments on surface farms
    - reimplemented as a C++ plugin to make proper biome detection possible
- `bodyswap`: fixed companion list not being updated often enough
- `cxxrandom`: removed some extraneous debug information
- `digfort`: now accounts for z-level changes when calculating maximum y dimension
- `embark-assistant`:
    - fixed bug causing crash on worlds without generated metals (as well as pruning vectors as originally intended).
    - fixed bug causing mineral matching to fail to cut off at the magma sea, reporting presence of things that aren't (like DF does currently).
    - fixed bug causing half of the river tiles not to be recognized.
    - added logic to detect some river tiles DF doesn't generate data for (but are definitely present).
- `eventful`: fixed invalid building ID in some building events
- `exportlegends`: now escapes special characters in names properly
- `getplants`: fixed designation of plants out of season (note that picked plants are still designated incorrectly)
- `gui/autogems`: fixed error when no world is loaded
- `gui/companion-order`:
    - fixed error when resetting group leaders
    - ``leave`` now properly removes companion links
- `gui/create-item`: fixed module support - can now be used from other scripts
- `gui/stamper`:
    - stopped "invert" from resetting the designation type
    - switched to using DF's designation keybindings instead of custom bindings
    - fixed some typos and text overlapping
- `modtools/create-unit`:
    - fixed an error associating historical entities with units
    - stopped recalculating health to avoid newly-created citizens triggering a "recover wounded" job
    - fixed units created in arena mode having blank names
    - fixed units created in arena mode having the wrong race and/or interaction effects applied after creating units manually in-game
    - stopped units from spawning with extra items or skills previously selected in the arena
    - stopped setting some unneeded flags that could result in glowing creature tiles
    - set units created in adventure mode to have no family, instead of being related to the first creature in the world
- `modtools/reaction-product-trigger`:
    - fixed an error dealing with reactions in adventure mode
    - blocked ``\\BUILDING_ID`` for adventure mode reactions
    - fixed ``-clear`` to work without passing other unneeded arguments
- `modtools/reaction-trigger`:
    - fixed a bug when determining whether a command was run
    - fixed handling of ``-resetPolicy``
- `mousequery`: fixed calculation of map dimensions, which was sometimes preventing scrolling the map with the mouse when TWBT was enabled
- `RemoteFortressReader`:
    - fixed a crash when a unit's path has a length of 0
- `stonesense`:
    - fixed crash due to wagons and other soul-less creatures
- `tame`: now sets the civ ID of tamed animals (fixes compatibility with `autobutcher`)
- `title-folder`: silenced error when ``PRINT_MODE`` is set to ``TEXT``

Misc Improvements
-----------------
- Added a note to `dfhack-run` when called with no arguments (which is usually unintentional)
- On macOS, the launcher now attempts to un-quarantine the rest of DFHack
- `bodyswap`: added arena mode support
- `createitem`: added a list of valid castes to the "invalid caste" error message, for convenience
- `combine-drinks`: added more default output, similar to `combine-plants`
- `devel/export-dt-ini`: added more size information needed by newer Dwarf Therapist versions
- `dwarfmonitor`: enabled widgets to access other scripts and plugins by switching to the core Lua context
- `embark-assistant`:
    - added an in-game option to activate on the embark screen
    - changed waterfall detection to look for level drop rather than just presence
    - changed matching to take incursions, i.e. parts of other biomes, into consideration when evaluating tiles. This allows for e.g. finding multiple biomes on single tile embarks.
    - changed overlay display to show when incursion surveying is incomplete
    - changed overlay display to show evil weather
    - added optional parameter "fileresult" for crude external harness automated match support
    - improved focus movement logic to go to only required world tiles, increasing speed of subsequent searches considerably
- `exportlegends`: added rivers to custom XML export
- `exterminate`: added support for a special ``enemy`` caste
- `gui/gm-unit`: added support for editing:
    - added attribute editor
    - added orientation editor
    - added editor for bodies and body parts
    - added color editor
    - added belief editor
    - added personality editor
- `modtools/create-item`:
    - documented already-existing ``-quality`` option
- `modtools/create-unit`:
    - added the ability to specify ``\\LOCAL`` for the fort group entity
    - now enables the default labours for adult units with CAN_LEARN.
    - now sets historical figure orientation.
    - improved speed of creating multiple units at once
    - made the script usable as a module (from other scripts)
- `modtools/reaction-trigger`:
    - added ``-ignoreWorker``: ignores the worker when selecting the targets
    - changed the default behavior to skip inactive/dead units; added ``-dontSkipInactive`` to include creatures that are inactive
    - added ``-range``: controls how far elligible targets can be from the workshop
    - syndromes now are applied before commands are run, not after
    - if both a command and a syndrome are given, the command only runs if the syndrome could be applied
- `mousequery`: made it more clear when features are enabled
- `RemoteFortressReader`:
    - added a basic framework for controlling and reading the menus in DF (currently only supports the building menu)
    - added support for reading item raws
    - added a check for whether or not the game is currently saving or loading, for utilities to check if it's safe to read from DF
    - added unit facing direction estimate and position within tiles
    - added unit age
    - added unit wounds
    - added tree information
    - added check for units' current jobs when calculating the direction they are facing

API
---
- Added ``Maps::GetBiomeType`` and ``Maps::GetBiomeTypeByRef`` to infer biome types properly
- Added ``Units::getPhysicalDescription`` (note that this depends on the ``unit_get_physical_description`` offset, which is not yet available for all DF builds)
- Added new ``plugin_load_data`` and ``plugin_save_data`` events for plugins to load/save persistent data

Internals
---------
- Added new Persistence module
- Persistent data is now stored in JSON files instead of historical figures - existing data will be migrated when saving
- Cut down on internal DFHack dependencies to improve build times
- Improved concurrency in event and server handlers
- `stonesense`: fixed some OpenGL build issues on Linux

Lua
---
- Exposed ``gui.dwarfmode.get_movement_delta`` and ``gui.dwarfmode.get_hotkey_target``
- ``dfhack.run_command`` now returns the command's return code

Ruby
----
- Made ``unit_ishostile`` consistently return a boolean

Structures
----------
- Added ``unit_get_physical_description`` function offset on some platforms
- Added/identified types:
    - ``assume_identity_mode``
    - ``musical_form_purpose``
    - ``musical_form_style``
    - ``musical_form_pitch_style``
    - ``musical_form_feature``
    - ``musical_form_vocals``
    - ``musical_form_melodies``
    - ``musical_form_interval``
    - ``unit_emotion_memory``
- ``twbt_render_map``: added for 64-bit 0.44.12 (for `map-render`)
- ``personality_facet_type``, ``value_type``: added ``NONE`` values
- ``need_type``: fixed ``PrayOrMeditate`` typo

0.44.12-r2
==========

New Plugins
-----------
- `debug`: manages runtime debug print category filtering
- `nestboxes`: automatically scan for and forbid fertile eggs incubating in a nestbox

New Scripts
-----------
- `devel/query`: searches for field names in DF objects
- `extinguish`: puts out fires
- `tame`: sets tamed/trained status of animals

Fixes
-----
- `building-hacks`: fixed error when dealing with custom animation tables
- `devel/test-perlin`: fixed Lua error (``math.pow()``)
- `embark-assistant`: fixed crash when entering finder with a 16x16 embark selected, and added 16 to dimension choices
- `embark-skills`: fixed missing ``skill_points_remaining`` field
- `full-heal`:
    - stopped wagon resurrection
    - fixed a minor issue with post-resurrection hostility
- `gui/companion-order`:
    - fixed issues with printing coordinates
    - fixed issues with move command
    - fixed cheat commands (and removed "Power up", which was broken)
- `gui/gm-editor`: fixed reinterpret cast (``r``)
- `gui/pathable`: fixed error when sidebar is hidden with ``Tab``
- `labormanager`:
    - stopped assigning labors to ineligible dwarves, pets, etc.
    - stopped assigning invalid labors
    - added support for crafting jobs that use pearl
    - fixed issues causing cleaning jobs to not be assigned
    - added support for disabling management of specific labors
- `prospector`: (also affected `embark-tools`) - fixed a crash when prospecting an unusable site (ocean, mountains, etc.) with a large default embark size in d_init.txt (e.g. 16x16)
- `siege-engine`: fixed a few Lua errors (``math.pow()``, ``unit.relationship_ids``)
- `tweak`: fixed ``hotkey-clear``

Misc Improvements
-----------------
- `armoks-blessing`: improved documentation to list all available arguments
- `devel/export-dt-ini`:
    - added viewscreen offsets for DT 40.1.2
    - added item base flags offset
    - added needs offsets
- `embark-assistant`:
    - added match indicator display on the right ("World") map
    - changed 'c'ancel to abort find if it's under way and clear results if not, allowing use of partial surveys.
    - added Coal as a search criterion, as well as a coal indication as current embark selection info.
- `full-heal`:
    - added ``-all``, ``-all_civ`` and ``-all_citizens`` arguments
    - added module support
    - now removes historical figure death dates and ghost data
- `growcrops`: added ``all`` argument to grow all crops
- `gui/load-screen`: improved documentation
- `labormanager`: now takes nature value into account when assigning jobs
- `open-legends`: added warning about risk of save corruption and improved related documentation
- `points`: added support when in ``viewscreen_setupdwarfgamest`` and improved error messages
- `siren`: removed break handling (relevant ``misc_trait_type`` was no longer used - see "Structures" section)

Internals
---------
- Linux/macOS: changed recommended build backend from Make to Ninja (Make builds will be significantly slower now)
- Added a usable unit test framework for basic tests, and a few basic tests
- Core: various thread safety and memory management improvements
- Fixed CMake build dependencies for generated header files
- Fixed custom ``CMAKE_CXX_FLAGS`` not being passed to plugins
- Changed ``plugins/CMakeLists.custom.txt`` to be ignored by git and created (if needed) at build time instead
- Added ``CMakeSettings.json`` with intellisense support

Lua
---
- ``utils``: new ``OrderedTable`` class

Structures
----------
- Win32: added missing vtables for ``viewscreen_storesst`` and ``squad_order_rescue_hfst``
- ``activity_event_performancest``: renamed poem as written_content_id
- ``dance_form``: named musical_form_id and musical_written_content_id
- ``incident_sub6_performance.participants``: named performance_event and role_index
- ``incident_sub6_performance``: made performance_event an enum
- ``incident_sub6_performance``: named poetic_form_id, musical_form_id, and dance_form_id
- ``musical_form_instruments``: named minimum_required and maximum_permitted
- ``musical_form``: named voices field
- ``poetic_form``: identified many fields and related enum/bitfield types
- ``setup_character_info``: identified ``skill_points_remaining`` (for `embark-skills`)
- ``unit_thought_type``: added new expulsion thoughts from 0.44.12
- ``viewscreen_layer_militaryst``: identified ``equip.assigned.assigned_items``
- ``world_data``: added ``mountain_peak_flags`` type, including ``is_volcano``
- ``written_content``: named poetic_form
- ``unit_action.attack``: identified ``attack_skill``
- ``unit_action.attack``: added ``lightly_tap`` and ``spar_report`` flags
- ``misc_trait_type``: removed ``LikesOutdoors``, ``Hardened``, ``TimeSinceBreak``, ``OnBreak`` (all unused by DF)
- ``unit_personality``: identified ``stress_drain``, ``stress_boost``, ``likes_outdoors``, ``combat_hardened``
- ``plant_tree_tile``: gave connection bits more meaningful names (e.g. ``connection_east`` instead of ``thick_branches_1``)
- ``plant_tree_info``: identified ``extent_east``, etc.
- ``ui``: fixed alignment of ``main`` and ``squads`` (fixes `tweak` hotkey-clear and DF-AI)
- ``ui.main``: identified ``fortress_site``
- ``ui.squads``: identified ``kill_rect_targets_scroll``
- ``world_site``: identified names and/or types of some fields
- ``world_history``: identified names and/or types of some fields
- ``viewscreen_setupadventurest``: identified some nemesis and personality fields, and ``page.ChooseHistfig``
- ``unit_storage_status``: newly identified type, stores noble holdings information (used in ``viewscreen_layer_noblelistst``)
- ``viewscreen_layer_noblelistst``: identified ``storage_status`` (see ``unit_storage_status`` type)
- ``viewscreen_layer_arena_creaturest``: identified item- and name-related fields
- ``viewscreen_new_regionst``: identified ``rejection_msg``, ``raw_folder``, ``load_world_params``
- ``viewscreen_new_regionst``: changed many ``int8_t`` fields to ``bool``
- ``unit_flags3``: identified ``marked_for_gelding``
- ``body_part_status``: identified ``gelded``

API
---
- New debug features related to `debug` plugin:
  - Classes (C++ only): ``Signal<Signature, type_tag>``, ``DebugCategory``, ``DebugManager``
  - Macros: ``TRACE``, ``DEBUG``, ``INFO``, ``WARN``, ``ERR``, ``DBG_DECLARE``, ``DBG_EXTERN``

0.44.12-r1
==========

Fixes
-----
- Console: fixed crash when entering long commands on Linux/macOS
- Removed jsoncpp's ``include`` and ``lib`` folders from DFHack builds/packages
- Fixed special characters in `command-prompt` and other non-console in-game outputs on Linux/macOS (in tools using ``df2console``)
- `die`: fixed Windows crash in exit handling
- `dwarfmonitor`, `manipulator`: fixed stress cutoffs
- `modtools/force`: fixed a bug where the help text would always be displayed and nothing useful would happen
- `ruby`: fixed calling conventions for vmethods that return strings (currently ``enabler.GetKeyDisplay()``)
- `startdwarf`: fixed on 64-bit Linux

Misc Improvements
-----------------
- Reduced time for designation jobs from tools like `digv` to be assigned workers
- `embark-assistant`:
    - Switched to standard scrolling keys, improved spacing slightly
    - Introduced scrolling of Finder search criteria, removing requirement for 46 lines to work properly (Help/Info still formatted for 46 lines).
    - Added Freezing search criterion, allowing searches for NA/Frozen/At_Least_Partial/Partial/At_Most_Partial/Never Freezing embarks.
- `rejuvenate`:
    - Added ``-all`` argument to apply to all citizens
    - Added ``-force`` to include units under 20 years old
    - Clarified documentation

API
---
- Added to ``Units`` module:
    - ``getStressCategory(unit)``
    - ``getStressCategoryRaw(level)``
    - ``stress_cutoffs`` (Lua: ``getStressCutoffs()``)

Internals
---------
- Changed default build architecture to 64-bit
- Added documentation for all RPC functions and a build-time check
- Added support for build IDs to development builds
- Use ``dlsym(3)`` to find vtables from libgraphics.so

Structures
----------
- Added ``start_dwarf_count`` on 64-bit Linux again and fixed scanning script
- ``army_controller``: added new vector from 0.44.11
- ``viewscreen_civlistst``: split ``unk_20`` into 3 pointers
- ``belief_system``: new type, few fields identified
- ``mental_picture``: new type, some fields identified
- ``mission``: new type (used in ``viewscreen_civlistst``)
- ``mission_report``:
    - new type (renamed, was ``mission`` before)
    - identified some fields
- ``spoils_report``: new type, most fields identified
- ``viewscreen_civlistst``:
    - identified new pages
    - identified new messenger-related fields
- ``viewscreen_image_creatorst``:
    - fixed layout
    - identified many fields
- ``viewscreen_reportlistst``:
    - added new mission and spoils report-related fields (fixed layout)
- ``world``:
    - ``belief_systems``: identified
- ``world.languages``: identified (minimal information; whole languages stored elsewhere)
- ``world.status``:
    - ``mission_reports``: renamed, was ``missions``
    - ``spoils_reports``: identified
- ``world.unk_131ec0``, ``world.unk_131ef0``: researched layout
- ``world.worldgen_status``: identified many fields

0.44.12-alpha1
==============

Fixes
-----
- macOS: fixed ``renderer`` vtable address on x64 (fixes `rendermax`)
- `stonesense`: fixed ``PLANT:DESERT_LIME:LEAF`` typo

API
---
- Added C++-style linked list interface for DF linked lists

Structures
----------
- Dropped 0.44.11 support
- ``ui.squads``: Added fields new in 0.44.12

0.44.11-beta2.1
===============

Internals
---------
- `stonesense`: fixed build

0.44.11-beta2
=============

Fixes
-----
- Windows: Fixed console failing to initialize
- `command-prompt`: added support for commands that require a specific screen to be visible, e.g. `spotclean`
- `gui/workflow`: fixed advanced constraint menu for crafts

API
---
- Added ``Screen::Hide`` to temporarily hide screens, like `command-prompt`

0.44.11-beta1
=============

Fixes
-----
- Fixed displayed names (from ``Units::getVisibleName``) for units with identities
- Fixed potential memory leak in ``Screen::show()``
- `fix/dead-units`: fixed script trying to use missing isDiplomat function

Misc Improvements
-----------------
- Console:
    - added support for multibyte characters on Linux/macOS
    - made the console exit properly when an interactive command is active (`liquids`, `mode`, `tiletypes`)
- Made the ``DFHACK_PORT`` environment variable take priority over ``remote-server.json``
- Linux: added automatic support for GCC sanitizers in ``dfhack`` script
- `digfort`: added better map bounds checking
- `dfhack-run`: added support for port specified in ``remote-server.json``, to match DFHack's behavior
- `remove-stress`:
    - added support for ``-all`` as an alternative to the existing ``all`` argument for consistency
    - sped up significantly
    - improved output/error messages
    - now removes tantrums, depression, and obliviousness
- `ruby`: sped up handling of onupdate events

API
---
- New functions:
    - ``Units::isDiplomat(unit)``
- Exposed ``Screen::zoom()`` to C++ (was Lua-only)

Lua
---
- Added ``printall_recurse`` to print tables and DF references recursively. It can be also used with ``^`` from the `lua` interpreter.
- ``gui.widgets``: ``List:setChoices`` clones ``choices`` for internal table changes

Internals
---------
- jsoncpp: updated to version 1.8.4 and switched to using a git submodule

Structures
----------
- ``history_event_entity_expels_hfst``: added (new in 0.44.11)
- ``history_event_site_surrenderedst``: added (new in 0.44.11)
- ``history_event_type``: added ``SITE_SURRENDERED``, ``ENTITY_EXPELS_HF`` (new in 0.44.11)
- ``syndrome``: identified a few fields
- ``viewscreen_civlistst``: fixed layout and identified many fields

0.44.11-alpha1
==============

Structures
----------
- Dropped 0.44.10 support
- Added support for automatically sizing arrays indexed with an enum
- Removed stale generated CSV files and DT layouts from pre-0.43.05
- ``announcement_type``: new in 0.44.11: ``NEW_HOLDING``, ``NEW_MARKET_LINK``
- ``breath_attack_type``: added ``OTHER``
- ``historical_figure_info.relationships.list``: added ``unk_3a``-``unk_3c`` fields at end
- ``interface_key``: added bindings new in 0.44.11
- ``occupation_type``: new in 0.44.11: ``MESSENGER``
- ``profession``: new in 0.44.11: ``MESSENGER``
- ``ui_sidebar_menus``:
    - ``unit.in_squad``: renamed to ``unit.squad_list_opened``, fixed location
    - ``unit``: added ``expel_error`` and other unknown fields new in 0.44.11
    - ``hospital``: added, new in 0.44.11
    - ``num_speech_tokens``, ``unk_17d8``: moved out of ``command_line`` to fix layout on x64
- ``viewscreen_civlistst``: added a few new fields (incomplete)
- ``viewscreen_locationsst``: identified ``edit_input``

0.44.10-r2
==========

New Plugins
-----------
- `cxxrandom`: exposes some features of the C++11 random number library to Lua

New Scripts
-----------
- `gui/stamper`: allows manipulation of designations by transforms such as translations, reflections, rotations, and inversion
- `add-recipe`: adds unknown crafting recipes to the player's civ

Fixes
-----
- Fixed many tools incorrectly using the ``dead`` unit flag (they should generally check ``flags2.killed`` instead)
- Fixed many tools passing incorrect arguments to printf-style functions, including a few possible crashes (`changelayer`, `follow`, `forceequip`, `generated-creature-renamer`)
- Fixed ``-g`` flag (GDB) in Linux ``dfhack`` script (particularly on x64)
- Fixed several bugs in Lua scripts found by static analysis (df-luacheck)
- `autochop`, `autodump`, `autogems`, `automelt`, `autotrade`, `buildingplan`, `dwarfmonitor`, `fix-unit-occupancy`, `fortplan`, `stockflow`: fix issues with periodic tasks not working for some time after save/load cycles
- `autogems`, `fix-unit-occupancy`: stopped running when a fort isn't loaded (e.g. while embarking)
- `buildingplan`, `fortplan`: stopped running before a world has fully loaded
- `autogems`:
    - stop running repeatedly when paused
    - fixed crash when furnaces are linked to same stockpiles as jeweler's workshops
- `ban-cooking`: fixed errors introduced by kitchen structure changes in 0.44.10-r1
- `remove-stress`: fixed an error when running on soul-less units (e.g. with ``-all``)
- `revflood`: stopped revealing tiles adjacent to tiles above open space inappropriately
- `dig`: fixed "Inappropriate dig square" announcements if digging job has been posted
- `stockpiles`: ``loadstock`` now sets usable and unusable weapon and armor settings
- `stocks`: stopped listing carried items under stockpiles where they were picked up from
- `deramp`: fixed deramp to find designations that already have jobs posted
- `fixnaked`: fixed errors due to emotion changes in 0.44
- `autounsuspend`: now skips planned buildings

Misc Improvements
-----------------
- Added script name to messages produced by ``qerror()`` in Lua scripts
- Fixed an issue in around 30 scripts that could prevent edits to the files (adding valid arguments) from taking effect
- Linux: Added several new options to ``dfhack`` script: ``--remotegdb``, ``--gdbserver``, ``--strace``
- `bodyswap`: improved error handling
- `buildingplan`: added max quality setting
- `caravan`: documented (new in 0.44.10-alpha1)
- `deathcause`: added "slaughtered" to descriptions
- `fix/retrieve-units`: now re-adds units to active list to counteract `fix/dead-units`
- ``item-descriptions``: fixed several grammatical errors
- `modtools/create-unit`:
    - added quantity argument
    - now selects a caste at random if none is specified
- `mousequery`:
    - migrated several features from TWBT's fork
    - added ability to drag with left/right buttons
    - added depth display for TWBT (when multilevel is enabled)
    - made shift+click jump to lower levels visible with TWBT
- `title-version`: added version to options screen too
- `embark-assistant`:
    - changed region interaction matching to search for evil rain, syndrome rain, and reanimation rather than interaction presence (misleadingly called evil weather), reanimation, and thralling
    - gave syndrome rain and reanimation wider ranges of criterion values
- `fix/dead-units`: added a delay of around 1 month before removing units

API
---
- New functions (also exposed to Lua):
    - ``Units::isKilled()``
    - ``Units::isActive()``
    - ``Units::isGhost()``
- Removed Vermin module (unused and obsolete)

Lua
---
- Added ``profiler`` module to measure lua performance
- Enabled shift+cursor movement in WorkshopOverlay-derived screens

Structures
----------
- ``unit_flags1``: renamed ``dead`` to ``inactive`` to better reflect its use
- ``item_body_component``: fixed location of ``corpse_flags``
- ``job_type``: added ``is_designation`` attribute
- ``unit_thought_type``: added ``SawDeadBody`` (new in 0.44.10)
- ``unit_personality``: fixed location of ``current_focus`` and ``undistracted_focus``
- ``incident_sub6_performance``: identified some fields
- ``job_handler``: fixed static array layout

Internals
---------
- Added fallback for YouCompleteMe database lookup failures (e.g. for newly-created files)
- jsoncpp: fixed constructor with ``long`` on Linux
- Improved efficiency and error handling in ``stl_vsprintf`` and related functions
- Added build option to generate symbols for large generated files containing df-structures metadata

0.44.10-r1
==========

New Scripts
-----------
- `bodyswap`: shifts player control over to another unit in adventure mode

New Tweaks
----------
- `tweak` stone-status-all: adds an option to toggle the economic status of all stones
- `tweak` kitchen-prefs-all: adds an option to toggle cook/brew for all visible items in kitchen preferences

Fixes
-----
- Lua: registered ``dfhack.constructions.designateRemove()`` correctly
- `prospector`: fixed crash due to invalid vein materials
- `tweak` max-wheelbarrow: fixed conflict with building renaming
- `view-item-info`: stopped appending extra newlines permanently to descriptions

Misc Improvements
-----------------
- Added logo to documentation
- Documented several missing ``dfhack.gui`` Lua functions
- `adv-rumors`: bound to Ctrl-A
- `command-prompt`: added support for ``Gui::getSelectedPlant()``
- `gui/advfort`: bound to Ctrl-T
- `gui/room-list`: added support for ``Gui::getSelectedBuilding()``
- `gui/unit-info-viewer`: bound to Alt-I
- `modtools/create-unit`: made functions available to other scripts
- `search`:
    - added support for stone restrictions screen (under ``z``: Status)
    - added support for kitchen preferences (also under ``z``)

Internals
---------
- Fixed compiler warnings on all supported build configurations
- Windows build scripts now work with non-C system drives

API
---
- New functions (all available to Lua as well):
    - ``Buildings::getRoomDescription()``
    - ``Items::checkMandates()``
    - ``Items::canTrade()``
    - ``Items::canTradeWithContents()``
    - ``Items::isRouteVehicle()``
    - ``Items::isSquadEquipment()``
    - ``Kitchen::addExclusion()``
    - ``Kitchen::findExclusion()``
    - ``Kitchen::removeExclusion()``
- syndrome-util: added ``eraseSyndromeData()``

Structures
----------
- ``dfhack_room_quality_level``: new enum
- ``glowing_barrier``: identified ``triggered``, added comments
- ``item_flags2``: renamed ``has_written_content`` to ``unk_book``
- ``kitchen_exc_type``: new enum (for ``ui.kitchen``)
- ``mandate.mode``: now an enum
- ``unit_personality.emotions.flags.memory``: identified
- ``viewscreen_kitchenprefst.forbidden``, ``possible``: now a bitfield, ``kitchen_pref_flag``
- ``world_data.feature_map``: added extensive documentation (in XML)

0.44.10-beta1
=============

New Scripts
-----------
- `devel/find-primitive`: finds a primitive variable in memory

Fixes
-----
- Units::getAnyUnit(): fixed a couple problematic conditions and potential segfaults if global addresses are missing
- `stockpiles`: stopped sidebar option from overlapping with `autodump`
- `autodump`, `automelt`, `autotrade`, `stocks`, `stockpiles`: fixed conflict with building renaming
- `tweak` block-labors: fixed two causes of crashes related in the v-p-l menu
- `full-heal`:
    - units no longer have a tendency to melt after being healed
    - healed units are no longer treated as patients by hospital staff
    - healed units no longer attempt to clean themselves unsuccessfully
    - wounded fliers now regain the ability to fly upon being healing
    - now heals suffocation, numbness, infection, spilled guts and gelding
- `modtools/create-unit`:
    - creatures of the appropriate age are now spawned as babies or children where applicable
    - fix: civ_id is now properly assigned to historical_figure, resolving several hostility issues (spawned pets are no longer attacked by fortress military!)
    - fix: unnamed creatures are no longer spawned with a string of numbers as a first name
- `exterminate`: fixed documentation of ``this`` option

Misc Improvements
-----------------
- `blueprint`: added a basic Lua API
- `devel/export-dt-ini`: added tool offsets for DT 40
- `devel/save-version`: added current DF version to output
- `install-info`: added information on tweaks

Internals
---------
- Added ``Gui::inRenameBuilding()``
- Added function names to DFHack's NullPointer and InvalidArgument exceptions
- Linux: required plugins to have symbols resolved at link time, for consistency with other platforms

0.44.10-alpha1
==============

New Scripts
-----------
- `caravan`: adjusts properties of caravans
- `gui/autogems`: a configuration UI for the `autogems` plugin

Fixes
-----
- Fixed uninitialized pointer being returned from ``Gui::getAnyUnit()`` in rare cases
- `autohauler`, `autolabor`, `labormanager`: fixed fencepost error and potential crash
- `dwarfvet`: fixed infinite loop if an animal is not accepted at a hospital
- `liquids`: fixed "range" command to default to 1 for dimensions consistently
- `search`: fixed 4/6 keys in unit screen search
- `view-item-info`: fixed an error with some armor

Misc Improvements
-----------------
- `autogems`: can now blacklist arbitrary gem types (see `gui/autogems`)
- `exterminate`: added more words for current unit, removed warning
- `fpause`: now pauses worldgen as well

Internals
---------
- Added some build scripts for Sublime Text
- Changed submodule URLs to relative URLs so that they can be cloned consistently over different protocols (e.g. SSH)

0.44.09-r1
==========

Internals
---------
- OS X: Can now build with GCC 7 (or older)

Fixes
-----
- `modtools/item-trigger`: fixed token format in help text

Misc Improvements
-----------------
- Reorganized changelogs and improved changelog editing process
- `modtools/item-trigger`: added support for multiple type/material/contaminant conditions

Structures
----------
- ``renderer``: fixed vtable addresses on 64-bit OS X
- ``building_type``: added human-readable ``name`` attribute
- ``furnace_type``: added human-readable ``name`` attribute
- ``workshop_type``: added human-readable ``name`` attribute
- ``army``: added vector new in 0.44.07
- ``site_reputation_report``: named ``reports`` vector

0.44.09-alpha1
==============

Fixes
-----
- `digtype`: stopped designating non-vein tiles (open space, trees, etc.)
- `labormanager`: fixed crash due to dig jobs targeting some unrevealed map blocks

0.44.08-alpha1
==============

Fixes
-----
- `fix/dead-units`: fixed a bug that could remove some arriving (not dead) units

0.44.07-beta1
=============

Structures
----------
- Added symbols for Toady's `0.44.07 Linux test build <http://www.bay12forums.com/smf/index.php?topic=169839.msg7720111#msg7720111>`_ to fix :bug:`10615`
- ``world_site``: fixed alignment

Misc Improvements
-----------------
- `modtools/item-trigger`: added the ability to specify inventory mode(s) to trigger on

0.44.07-alpha1
==============

Fixes
-----
- Support for building on Ubuntu 18.04
- Fixed some CMake warnings (CMP0022)
- `embark-assistant`: fixed detection of reanimating biomes

Misc Improvements
-----------------
- `embark-assistant`:

    - Added search for adamantine
    - Now supports saving/loading profiles

- `fillneeds`: added ``-all`` option to apply to all units
- `remotefortressreader`: added flows, instruments, tool names, campfires, ocean waves, spiderwebs

Structures
----------
- Several new names in instrument raw structures
- ``identity``: identified ``profession``, ``civ``
- ``manager_order_template``: fixed last field type
- ``viewscreen_createquotast``: fixed layout
- ``world.language``: moved ``colors``, ``shapes``, ``patterns`` to ``world.descriptors``
- ``world.reactions``, ``world.reaction_categories``:\ moved to new compound, ``world.reactions``. Requires renaming:

    - ``world.reactions`` to ``world.reactions.reactions``
    - ``world.reaction_categories`` to ``world.reactions.reaction_categories``

0.44.05-r2
==========

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

New Plugins
-----------
- `embark-assistant`: adds more information and features to embark screen

New Scripts
-----------
- `adv-fix-sleepers`: fixes units in adventure mode who refuse to wake up (:bug:`6798`)
- `hermit`: blocks caravans, migrants, diplomats (for hermit challenge)

New Features
------------
- With ``PRINT_MODE:TEXT``, setting the ``DFHACK_HEADLESS`` environment variable will hide DF's display and allow the console to be used normally. (Note that this is intended for testing and is not very useful for actual gameplay.)

0.44.05-r1
==========

New Scripts
-----------
- `break-dance`: Breaks up a stuck dance activity
- `fillneeds`: Use with a unit selected to make them focused and unstressed
- `firestarter`: Lights things on fire: items, locations, entire inventories even!
- `flashstep`: Teleports adventurer to cursor
- `ghostly`: Turns an adventurer into a ghost or back
- `questport`: Sends your adventurer to the location of your quest log cursor
- `view-unit-reports`: opens the reports screen with combat reports for the selected unit

Fixes
-----
- `devel/inject-raws`: now recognizes spaces in reaction names
- `dig`: added support for designation priorities - fixes issues with designations from ``digv`` and related commands having extremely high priority
- `dwarfmonitor`:
    - fixed display of creatures and poetic/music/dance forms on ``prefs`` screen
    - added "view unit" option
    - now exposes the selected unit to other tools

- `names`: fixed many errors
- `quicksave`: fixed an issue where the "Saving..." indicator often wouldn't appear

Misc Improvements
-----------------
- `gui/gm-unit`:
    - added a profession editor
    - misc. layout improvements
- `remotefortressreader`:
    - support for moving adventurers
    - support for vehicles, gem shapes, item volume, art images, item improvements
- `binpatch`: now reports errors for empty patch files
- `force`: now provides useful help
- `full-heal`:
    - can now select corpses to resurrect
    - now resets body part temperatures upon resurrection to prevent creatures from freezing/melting again
    - now resets units' vanish countdown to reverse effects of `exterminate`
- `launch`: can now ride creatures
- `names`: can now edit names of units

Removed
-------
- `tweak`: ``kitchen-keys``: :bug:`614` fixed in DF 0.44.04

Internals
---------
- ``Gui::getAnyUnit()`` supports many more screens/menus

Structures
----------
- New globals: ``soul_next_id``

0.44.05-alpha1
==============

Misc Improvements
-----------------
- `gui/liquids`: added more keybindings: 0-7 to change liquid level, P/B to cycle backwards

Structures
----------
- ``incident``: re-aligned again to match disassembly

0.44.04-alpha1
==============

Fixes
-----
- `devel/inject-raws`: now recognizes spaces in reaction names
- `exportlegends`: fixed an error that could occur when exporting empty lists


Structures
----------
- ``artifact_record``: fixed layout (changed in 0.44.04)
- ``incident``: fixed layout (changed in 0.44.01) - note that many fields have moved

0.44.03-beta1
=============

Fixes
-----
- `autolabor`, `autohauler`, `labormanager`: added support for "put item on display" jobs and building/destroying display furniture
- `gui/gm-editor`: fixed an error when editing primitives in Lua tables

Misc Improvements
-----------------
- @ `devel/dump-offsets`: now ignores ``index`` globals
- `gui/pathable`: added tile types to sidebar
- `modtools/skill-change`:
    - now updates skill levels appropriately
    - only prints output if ``-loud`` is passed

Structures
----------
- New globals:
    - ``version``
    - ``min_load_version``
    - ``movie_version``
    - ``basic_seed``
    - ``title``
    - ``title_spaced``
    - ``ui_building_resize_radius``
- Added ``twbt_render_map`` code offset on x64
- Fixed an issue preventing ``enabler`` from being allocated by DFHack
- Added ``job_type.PutItemOnDisplay``
- Found ``renderer`` vtable on osx64
- ``adventure_movement_optionst``, ``adventure_movement_hold_tilest``, ``adventure_movement_climbst``: named coordinate fields
- ``mission``: added type
- ``unit``: added 3 new vmethods: ``getCreatureTile``, ``getCorpseTile``, ``getGlowTile``
- ``viewscreen_assign_display_itemst``: fixed layout on x64 and identified many fields
- ``viewscreen_reportlistst``: fixed layout, added ``mission_id`` vector
- ``world.status``: named ``missions`` vector

0.44.03-alpha1
==============

Lua
---
- Improved ``json`` I/O error messages
- Stopped a crash when trying to create instances of classes whose vtable addresses are not available

0.44.02-beta1
=============

New Scripts
-----------
- `devel/check-other-ids`: Checks the validity of "other" vectors in the ``world`` global
- `gui/cp437-table`: An in-game CP437 table

Fixes
-----
- Fixed issues with the console output color affecting the prompt on Windows
- `createitem`: stopped items from teleporting away in some forts
- `gui/gm-unit`: can now edit mining skill
- `gui/quickcmd`: stopped error from adding too many commands
- `modtools/create-unit`: fixed error when domesticating units

Misc Improvements
-----------------
- The console now provides suggestions for built-in commands
- `devel/export-dt-ini`: avoid hardcoding flags
- `exportlegends`:
    - reordered some tags to match DF's order
    - added progress indicators for exporting long lists
- `gui/gm-editor`: added enum names to enum edit dialogs
- `gui/gm-unit`: made skill search case-insensitive
- `gui/rename`: added "clear" and "special characters" options
- `remotefortressreader`:
    - includes item stack sizes
    - some performance improvements

Removed
-------
- `warn-stuck-trees`: :bug:`9252` fixed in DF 0.44.01

Lua
---
- Exposed ``get_vector()`` (from C++) for all types that support ``find()``, e.g. ``df.unit.get_vector() == df.global.world.units.all``

Structures
----------
- Located ``start_dwarf_count`` offset for all builds except 64-bit Linux; `startdwarf` should work now
- Added ``buildings_other_id.DISPLAY_CASE``
- Fixed ``viewscreen_titlest.start_savegames`` alignment
- Fixed ``unit`` alignment
- Identified ``historical_entity.unknown1b.deities`` (deity IDs)

0.44.02-alpha1
==============

New Scripts
-----------
- `devel/dump-offsets`: prints an XML version of the global table included in in DF

Fixes
-----
- Fixed a crash that could occur if a symbol table in symbols.xml had no content

Lua
---
- Added a new ``dfhack.console`` API
- API can now wrap functions with 12 or 13 parameters

Structures
----------
- The ``ui_menu_width`` global is now a 2-byte array; the second item is the former ``ui_area_map_width`` global, which is now removed
- The former ``announcements`` global is now a field in ``d_init``
- ``world`` fields formerly beginning with ``job_`` are now fields of ``world.jobs``, e.g. ``world.job_list`` is now ``world.jobs.list``

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
- `search-plugin`: workshop profile search added.
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
- `search-plugin`: Adds an incremental search function to the Stocks, Trading, Stockpile and Unit List screens.
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
