burrows
=======

.. dfhack-tool::
    :summary: Auto-expand burrows as you dig.
    :tags: unavailable
    :no-command:

.. dfhack-command:: burrow
    :summary: Quickly add units/tiles to burrows.

When a wall inside a burrow with a name ending in ``+`` is dug out, the burrow
will be extended to newly-revealed adjacent walls.

Usage
-----

``burrow enable auto-grow``
    When a wall inside a burrow with a name ending in '+' is dug out, the burrow
    will be extended to newly-revealed adjacent walls. This final '+' may be
    omitted in burrow name args of other ``burrow`` commands. Note that digging
    1-wide corridors with the miner inside the burrow is SLOW.
``burrow disable auto-grow``
    Disables auto-grow processing.
``burrow clear-unit <burrow> [<burrow> ...]``
    Remove all units from the named burrows.
``burrow clear-tiles <burrow> [<burrow> ...]``
    Remove all tiles from the named burrows.
``burrow set-units target-burrow <burrow> [<burrow> ...]``
    Clear all units from the target burrow, then add units from the named source
    burrows.
``burrow add-units target-burrow <burrow> [<burrow> ...]``
    Add units from the source burrows to the target.
``burrow remove-units target-burrow <burrow> [<burrow> ...]``
    Remove units in source burrows from the target.
``burrow set-tiles target-burrow <burrow> [<burrow> ...]``
    Clear target burrow tiles and add tiles from the names source burrows.
``burrow add-tiles target-burrow <burrow> [<burrow> ...]``
    Add tiles from the source burrows to the target.
``burrow remove-tiles target-burrow <burrow> [<burrow> ...]``
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
