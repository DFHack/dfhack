zone
====

.. dfhack-tool::
    :summary: Manage activity zones, cages, and the animals therein.
    :tags: unavailable

Usage
-----

``enable zone``
   Add helpful filters to the pen/pasture sidebar menu (e.g. show only caged
   grazers).
``zone set``
   Set zone or cage under cursor as default for future ``assign`` or ``tocages``
   commands.
``zone assign [<zone id>] [<filter>]``
   Assign unit(s) to the zone with the given ID, or to the most recent pen or
   pit marked with the ``set`` command. If no filters are set, then a unit must
   be selected in the in-game ui.
``zone unassign [<filter>]``
    Unassign selected creature from its zone.
``zone nick <nickname> [<filter>]``
    Assign the given nickname to the selected animal or the animals matched by
    the given filter.
``zone remnick [<filter>]``
    Remove nicknames from the selected animal or the animals matched by the
    given filter.
``zone enumnick <nickname prefix> [<filter>]``
    Assign enumerated nicknames (e.g. "Hen 1", "Hen 2"...).
``zone tocages [<filter>]``
    Assign unit(s) to cages that have been built inside the pasture selected
    with the ``set`` command.
``zone uinfo [<filter>]``
    Print info about unit(s). If no filters are set, then a unit must be
    selected in the in-game ui.
``zone zinfo``
    Print info about the zone(s) and any buildings under the cursor.

Examples
--------

Before any ``assign`` or ``tocages`` examples can be used, you must first move
the cursor over a pen/pasture or pit zone and run ``zone set`` to select the
zone.

``zone assign all own ALPACA minage 3 maxage 10``
    Assign all of your alpacas who are between 3 and 10 years old to the
    selected pasture.
``zone assign all own caged grazer nick ineedgrass``
    Assign all of your grazers who are sitting in cages on stockpiles (e.g.
    after buying them from merchants) to the selected pasture and give them the
    nickname 'ineedgrass'.
``zone assign all own not grazer not race CAT``
    Assign all of your animals who are not grazers (excluding cats) to the
    selected pasture.
    "  zone assign all own milkable not grazer\n"
``zone assign all own female milkable not grazer``
    Assign all of your non-grazing milkable creatures to the selected pasture or
    cage.
``zone assign all own race DWARF maxage 2``
    Throw all useless kids into a pit :) They'll be fine I'm sure.
``zone nick donttouchme``
    Nicknames all units in the current default zone or cage to 'donttouchme'.
    This is especially useful for protecting a group of animals assigned to a
    pasture or cage from being "processed" by `autobutcher`.
``zone tocages count 50 own tame male not grazer``
    Stuff up to 50 of your tame male animals who are not grazers into cages
    built on the current default zone.

Filters
-------

:all:            Process all units.
:count <n>:      Process only up to n units.
:unassigned:     Not assigned to zone, chain or built cage.
:minage <years>: Minimum age. Must be followed by a number.
:maxage <years>: Maximum age. Must be followed by a number.
:not:            Negates the next filter keyword. All of the keywords documented
                 below are negatable.
:race:           Must be followed by a race RAW ID (e.g. BIRD_TURKEY, ALPACA,
                 etc).
:caged:          In a built cage.
:own:            From own civilization. You'll usually want to include this
                 filter.
:war:            Trained war creature.
:hunting:        Trained hunting creature.
:tamed:          Creature is tame.
:trained:        Creature is trained. Finds war/hunting creatures as well as
                 creatures who have a training level greater than 'domesticated'.
                 If you want to specifically search for war/hunting creatures
                 use ``war`` or ``hunting``.
:trainablewar:   Creature can be trained for war (and is not already trained for
                 war/hunt).
:trainablehunt:  Creature can be trained for hunting (and is not already trained
                 for war/hunt).
:male:           Creature is male.
:female:         Creature is female.
:egglayer:       Race lays eggs. If you want units who actually lay eggs, also
                 specify ``female``.
:grazer:         Race is a grazer.
:milkable:       Race is milkable. If you want units who actually can be milked,
                 also specify ``female``.
:merchant:       Is a merchant / belongs to a merchant. Should only be used for
                 pitting or slaughtering, not for stealing animals.

Usage with single units
-----------------------
One convenient way to use the zone tool is to bind the commands ``zone assign``
and ``zone set`` to hotkeys. Place the in-game cursor over a pen/pasture or pit
and use the ``zone set`` hotkey to mark it. Then you can select units on the map
(in 'v' or 'k' mode), in the unit list or from inside cages and use the
``zone assign`` hotkey to assign them to their new home. Allows pitting your own
dwarves, by the way.

Matching with filters
---------------------
All filters can be used together with the ``assign`` and ``tocages`` commands.

Note that it's not possible to reassign units who are inside built cages or
chained, though this likely won't matter because if you have gone to the trouble
of creating a zoo or chaining a creature, you probably wouldn't want them
reassigned anyways. Also, ``zone`` will avoid caging owned pets because the owner
uncages them after a while which results in infinite hauling back and forth.

Most filters should include an ``own`` element (which implies ``tame``) unless
you want to use ``zone assign`` for pitting hostiles. The ``own`` filter ignores
dwarves unless you explicitly specify ``race DWARF`` (so it's safe to use
``assign all own`` to one big pasture if you want to have all your animals in
the same place).

The ``egglayer`` and ``milkable`` filters should be used together with
``female`` unless you want the males of the race included. Merchants and their
animals are ignored unless you specify ``merchant`` (pitting them should be no
problem, but stealing and pasturing their animals is not a good idea since
currently they are not properly added to your own stocks; slaughtering them
should work).

Most filters can be negated (e.g. ``not grazer`` -> race is not a grazer).

Mass-renaming
-------------

Using the ``nick`` command, you can set the same nickname for multiple units.
If used without ``assign``, ``all``, or ``count``, it will rename all units in
the current default target zone. Combined with ``assign``, ``all``, or ``count``
(and likely further optional filters) it will rename units matching the filter
conditions.

Cage zones
----------

The ``tocages`` command assigns units to a set of cages, for example a room next
to your butcher shop(s). Units will be spread evenly among available cages to
optimize hauling to and butchering from them. For this to work you need to build
cages and then place one pen/pasture activity zone above them, covering all
cages you want to use. Then use ``zone set`` (like with ``assign``) and run
``zone tocages <filter>``. ``tocages`` can be used together with ``nick`` or
``remnick`` to adjust nicknames while assigning to cages.

Overlays
--------

Animal Assignment

Advanced unit selection is available via an `overlay` widget that appears when
you select a cage, restraint, pasture zone, or pit/pond zone.

In the window that pops up when you click the hotkey hint or hit the hotkey on your keyboard, you can:

- search for units by name
- sort or filter by status (Assigned here, Pastured elsewhere, On restraint, On
    display in cage, In movable cage, or Roaming)
- sort or filter by disposition (Pet, Domesticated, Partially trained, Wild
    (trainable), Wild (untrainable), or Hostile)
- sort by gender
- sort by name
- filter by whether the unit lays eggs
- filter by whether the unit needs a grazing area

The window is fully navigatable via keyboard or mouse. Hit Enter or click on a
unit to assign/unassign it to the currently selected zone or building. Shift
click to assign/unassign a range of units.

You can also keep the window open and click around on different cages,
restraints, pastures, or pit/ponds, so you can manage multiple buildings/zones
without having to close and reopen the window.

Just like all other overlays, you can disable this one in `gui/control-panel` on
the Overlays tab if you don't want the option of using it.

Location retirement

When viewing location (temple, guildhall, tavern, etc.) details, there is now a
"Retire location" button that you can use to remove the location from the list
of available locations for your fort.

Before you can retire the location, you have to remove all units assigned
occupations for that location and you have to detach the location from any
zones. Once you confirm retirement, the location will be removed from the list.
