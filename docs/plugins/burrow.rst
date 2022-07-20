burrow
======

Tags:
:dfhack-keybind:`burrow`

Quick commands for burrow control. Allows manipulating burrows and automated
burrow expansion while digging.

Usage:

- ``burrows enable auto-grow``
    When a wall inside a burrow with a name ending in '+' is dug out, the burrow
    will be extended to newly-revealed adjacent walls. This final '+' may be
    omitted in burrow name args of other ``burrows`` commands. Note that digging
    1-wide corridors with the miner inside the burrow is SLOW. Be sure to also
    run ``enable burrow`` for this feature to work.
- ``burrows disable auto-grow``
    Disables auto-grow processing.
- ``burrows clear-unit <burrow> [<burrow> ...]``
    Remove all units from the named burrows.
- ``burrows clear-tiles <burrow> [<burrow> ...]``
    Remove all tiles from the named burrows.
- ``burrows set-units target-burrow <burrow> [<burrow> ...]``
    Clear all units from the target burrow, then add units from the named source
    burrows.
- ``burrows add-units target-burrow <burrow> [<burrow> ...]``
    Add units from the source burrows to the target.
- ``burrows remove-units target-burrow <burrow> [<burrow> ...]``
    Remove units in source burrows from the target.
- ``burrows set-tiles target-burrow <burrow> [<burrow> ...]``
    Clear target burrow tiles and adds tiles from the names source burrows.
- ``burrows add-tiles target-burrow <burrow> [<burrow> ...]``
    Add tiles from the source burrows to the target.
- ``burrows remove-tiles target-burrow <burrow> [<burrow> ...]``
    Remove tiles in source burrows from the target.

In place of a source burrow, you can use one of the following keywords:

- ``ABOVE_GROUND``
- ``SUBTERRANEAN``
- ``INSIDE``
- ``OUTSIDE``
- ``LIGHT``
- ``DARK``
- ``HIDDEN``
- ``REVEALED``

to add tiles with the given properties.
