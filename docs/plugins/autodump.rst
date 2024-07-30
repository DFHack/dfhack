autodump
========

.. dfhack-tool::
    :summary: Instantly gather or destroy items marked for dumping.
    :tags: fort armok fps items

.. dfhack-command:: autodump-destroy-here
    :summary: Destroy items marked for dumping under the keyboard cursor.

This tool can instantly move all unforbidden items marked for dumping to the
tile under the keyboard cursor. After moving the items, the dump flag is unset
and the forbid flag is set, just as if it had been dumped normally.

The keyboard cursor can be placed on a floor tile or in the air. If in air
the items will be converted into projectiles and fall. Items cannot be dumped
inside of walls or fortifications.

Usage
-----

::

    autodump [<options>]
    autodump-destroy-here

``autodump-destroy-here`` is an alias for ``autodump destroy-here`` and is
intended for use as a keybinding.

Options
-------

``destroy``
    Destroy instead of dumping. Doesn't require a cursor. If ``autodump`` is
    called again with this option before the game is resumed, it cancels
    pending destroy actions.
``destroy-here``
    Destroy items marked for dumping under the cursor.
``visible``
    Only process items that are not hidden.
``hidden``
    Only process hidden items.
``forbidden``
    Only process forbidden items (default: only unforbidden).

Examples
--------

``autodump``
    Teleports items marked for dumping to the cursor position.
``autodump destroy``
    Destroys all unforbidden items marked for dumping
``autodump-destroy-here``
    Destroys items on the selected tile that are marked for dumping.
