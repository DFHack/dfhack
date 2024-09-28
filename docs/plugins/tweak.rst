tweak
=====

.. dfhack-tool::
    :summary: A collection of tweaks and bugfixes.
    :tags: fort bugfix fps gameplay interface

Usage
-----

::

    tweak [list]
    tweak <command> [disable] [quiet]

Run the ``tweak`` command with the tweak name to enable its effects. Add a
``disable`` keyword to disable them, and a ``quiet`` keyword to prevent
printing of non-error output.

Examples
--------

``tweak eggs-fertile``
    Enable the ``eggs-fertile`` tweak.

``tweak fast-heat quiet``
    Enable the ``fast-heat`` tweak, but don't print anything to the console to
    say that it has been enabled.

``tweak fast-heat disable quiet``
    Disable the ``fast-heat`` tweak, and be quiet about it.

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
    Displays an indicator on fertile eggs.
``fast-heat``
    Improves temperature update performance by ensuring that 1 degree of item
    temperature is crossed in no more than 100 ticks when updating from the
    environment temperature. This reduces the time it takes for temperature to
    reach equilibrium and improves FPS when there are many items.
``flask-contents``
    Names filled waterskins, flasks, and vials according to their contents,
    the same way other containers such as barrels, bins, and cages are named.
    (:bug:`4914`)
``realistic-melting``
    Makes amortized metal bar returns for melting uniform across all item types.
    Affects weapons, shields, armor parts, tools, and trap components. The target
    amount of metal produced by melting is 95% of the metal used for production
    of the item. Each level of wear decreases melt return by a further 10%. The game
    has a fixed granularity of 0.3 for metal bar returns, so individual items will
    randomly return an amount that may be above or below the target. For example
    a metal cap with item size 1 will produce 0.9 of a bar with a 16.6% chance of
    producing an additional 0.3 of a bar. Over time, the average return for melting
    these types of caps will be ~0.95 of a bar. Calculations for melting return are
    done for items with base game production cost. Melting return might not be
    calculated correctly for modded items or created in custom reactions not
    respecting vanilla production costs. (:bug:`6027`)
``named-codices``
    Displays titles for books instead of the default material description.
``partial-items``
    Displays percentages on partially-consumed items such as hospital cloth.
``reaction-gloves``
    Fixes reactions to produce gloves in sets with correct handedness
    (:bug:`6273`).
