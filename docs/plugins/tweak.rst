tweak
=====

.. dfhack-tool::
    :summary: A collection of tweaks and bugfixes.
    :tags: fort bugfix fps gameplay interface

Usage
-----

::

    tweak [list]
    tweak <command> [disable]

Run the ``tweak`` command with the tweak name to enable its effects. Add a
``disable`` keyword to disable them.

Commands
--------

.. comment: please keep these sorted alphabetically

``adamantine-cloth-wear``
    Prevents adamantine clothing from wearing out while being worn
    (:bug:`6481`).
``craft-age-wear``
    Fixes crafted items not wearing out over time (:bug:`6003`). With this
    tweak, items made from cloth and leather will gain a level of wear every 20
    in-game years.
``eggs-fertile``
    Displays an indicator on fetile eggs.
``fast-heat``
    Improves temperature update performance by ensuring that 1 degree of item
    temperature is crossed in no more than 100 ticks when updating from the
    environment temperature. This reduces the time it takes for temperature to
    reach equilibrium and improves FPS when there are many items.
``partial-items``
    Displays percentages on partially-consumed items such as hospital cloth.
``reaction-gloves``
    Fixes reactions to produce gloves in sets with correct handedness
    (:bug:`6273`).
