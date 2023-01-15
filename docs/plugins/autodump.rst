autodump
========

.. dfhack-tool::
    :summary: Automatically set items in a stockpile to be dumped.
    :tags: untested fort armok fps productivity items stockpiles
    :no-command:

.. dfhack-command:: autodump
    :summary: Teleports items marked for dumping to the cursor position.

.. dfhack-command:: autodump-destroy-here
    :summary: Destroy items marked for dumping under the cursor.

.. dfhack-command:: autodump-destroy-item
    :summary: Destroys the selected item.

When `enabled <enable>`, this plugin adds an option to the :kbd:`q` menu for
stockpiles. When the ``autodump`` option is selected for the stockpile, any
items placed in the stockpile will automatically be designated to be dumped.

When invoked as a command, it can instantly move all unforbidden items marked
for dumping to the tile under the cursor. After moving the items, the dump flag
is unset and the forbid flag is set, just as if it had been dumped normally. Be
aware that dwarves that are en route to pick up the item for dumping may still
come and move the item to your dump zone.

The cursor must be placed on a floor tile so the items can be dumped there.

Usage
-----

::

    enable autodump
    autodump [<options>]
    autodump-destroy-here
    autodump-destroy-item

``autodump-destroy-here`` is an alias for ``autodump destroy-here`` and is
intended for use as a keybinding.

``autodump-destroy-item`` destroys only the selected item. The item may be
selected in the :kbd:`k` list or in the container item list. If called again
before the game is resumed, cancels destruction of the item.

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
``autodump-destroy-item``
    Destroys the selected item.
