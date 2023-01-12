tweak
=====

.. dfhack-tool::
    :summary: A collection of tweaks and bugfixes.
    :tags: untested adventure fort armok bugfix fps interface

Usage
-----

::

    tweak <command> [disable]

Run the ``tweak`` command to run the tweak or enable its effects. For tweaks
that have persistent effects, append the ``disable`` keyword to disable them.

One-shot commands:

``clear-missing``
    Remove the missing status from the selected unit. This allows engraving
    slabs for ghostly, but not yet found, creatures.
``clear-ghostly``
    Remove the ghostly status from the selected unit and mark it as dead. This
    allows getting rid of bugged ghosts which do not show up in the engraving
    slab menu at all, even after using ``clear-missing``. It works, but is
    potentially very dangerous - so use with care. Probably (almost certainly)
    it does not have the same effects like a proper burial. You've been warned.
``fixmigrant``
    Remove the resident/merchant flag from the selected unit. Intended to fix
    bugged migrants/traders who stay at the map edge and don't enter your fort.
    Only works for dwarves (or generally the player's race in modded games).
    Do NOT abuse this for 'real' caravan merchants (if you really want to kidnap
    them, use ``tweak makeown`` instead, otherwise they will have their clothes
    set to forbidden).
``makeown``
    Force selected unit to become a member of your fort. Can be abused to grab
    caravan merchants and escorts, even if they don't belong to the player's
    race. Foreign sentients (humans, elves) can be put to work, but you can't
    assign rooms to them and they don't show up in labor management programs
    (like `manipulator` or Dwarf Therapist) because the game treats them like
    pets. Grabbing draft animals from a caravan can result in weirdness
    (animals go insane or berserk and are not flagged as tame), but you are
    allowed to mark them for slaughter. Grabbing wagons results in some funny
    spam, then they are scuttled.

Commands that persist until disabled or DF quits:

.. comment: please sort these alphabetically

``adamantine-cloth-wear``
    Prevents adamantine clothing from wearing out while being worn
    (:bug:`6481`).
``advmode-contained``
    Fixes custom reactions with container inputs in advmode
    (:bug:`6202`) in advmode. The issue is that the screen tries to force you to
    select the contents separately from the container. This forcefully skips
    child reagents.
``block-labors``
    Prevents labors that can't be used from being toggled.
``burrow-name-cancel``
    Implements the "back" option when renaming a burrow, which currently does
    nothing in vanilla DF (:bug:`1518`).
``cage-butcher``
    Adds an option to butcher units when viewing cages with :kbd:`q`.
``civ-view-agreement``
    Fixes overlapping text on the "view agreement" screen.
``condition-material``
    Fixes a crash in the work order condition material list (:bug:`9905`).
``craft-age-wear``
    Fixes crafted items not wearing out over time (:bug:`6003`). With this
    tweak, items made from cloth and leather will gain a level of wear every 20
    years.
``do-job-now``
    Adds a job priority toggle to the jobs list.
``embark-profile-name``
    Allows the use of lowercase letters when saving embark profiles.
``eggs-fertile``
    Displays a fertility indicator on nestboxes.
``farm-plot-select``
    Adds "Select all" and "Deselect all" options to farm plot menus.
``fast-heat``
    Improves temperature update performance by ensuring that 1 degree of item
    temperature is crossed in no more than specified number of frames when
    updating from the environment temperature. This reduces the time it takes
    for ``tweak stable-temp`` to stop updates again when equilibrium is
    disturbed.
``fast-trade``
    Makes Shift-Down in the Move Goods to Depot and Trade screens toggle the
    current item (fully, in case of a stack), and scroll down one line. Shift-Up
    undoes the last Shift-Down by scrolling up one line and then toggle the item.
``fps-min``
    Fixes the in-game minimum FPS setting (:bug:`6277`).
``hide-priority``
    Adds an option to hide designation priority indicators.
``hotkey-clear``
    Adds an option to clear currently-bound hotkeys (in the :kbd:`H` menu).
``import-priority-category``
    When meeting with a liaison, makes Shift+Left/Right arrow adjust all items
    in category when discussing an import agreement with the liaison.
``kitchen-prefs-all``
    Adds an option to toggle cook/brew for all visible items in kitchen
    preferences.
``kitchen-prefs-color``
    Changes color of enabled items to green in kitchen preferences.
``kitchen-prefs-empty``
    Fixes a layout issue with empty kitchen tabs (:bug:`9000`).
``max-wheelbarrow``
    Allows assigning more than 3 wheelbarrows to a stockpile.
``military-color-assigned``
    Color squad candidates already assigned to other squads in yellow/green to
    make them stand out more in the list.

                        .. image:``../images/tweak-mil-color.png

``military-stable-assign``
    Preserve list order and cursor position when assigning to squad, i.e. stop
    the rightmost list of the Positions page of the military screen from
    constantly resetting to the top.
``nestbox-color``
    Makes built nestboxes use the color of their material.
``partial-items``
    Displays percentages on partially-consumed items such as hospital cloth.
``pausing-fps-counter``
    Replace fortress mode FPS counter with one that stops counting when paused.
``reaction-gloves``
    Fixes reactions to produce gloves in sets with correct handedness
    (:bug:`6273`).
``shift-8-scroll``
    Gives Shift-8 (or :kbd:`*`) priority when scrolling menus, instead of
    scrolling the map.
``stable-cursor``
    Saves the exact cursor position between t/q/k/d/b/etc menus of fortress
    mode, if the map view is near enough to its previous position.
``stone-status-all``
    Adds an option to toggle the economic status of all stones.
``title-start-rename``
    Adds a safe rename option to the title screen "Start Playing" menu.
``tradereq-pet-gender``
    Displays pet genders on the trade request screen.
