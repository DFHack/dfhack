autodump
========

Quickly designate or teleport items to be dumped. When `enabled <enable>`, this
plugin adds an option to the :kbd:`q` menu for stockpiles. When the ``autodump``
option is selected for the stockpile, any items placed in the stockpile will
automatically be designated to be dumped.

When invoked as a command, it can instantly move all items designated to be
dumped to the tile under the cursor. After moving the items, the dump flag is
unset and the forbid flag is set, just as if it had been dumped normally. Be
aware that dwarves that are en route to pick up the item for dumping may still
come and move the item to your dump zone.

The cursor must be placed on a floor tile so the items can be dumped there.

Usage::

    enable autodump
    autodump [<options>]

Options:

- ``destroy``
    Destroy instead of dumping. Doesn't require a cursor. If ``autodump`` is
    called again with this option before the game is resumed, it cancels
    the destroy action.
- ``destroy-here``
    As ``destroy``, but only the selected item in the :kbd:`k` list, or inside a
    container.
- ``visible``
    Only process items that are not hidden.
- ``hidden``
    Only process hidden items.
- ``forbidden``
    Only process forbidden items (default: only unforbidden).

Examples:

- ``autodump``
    Teleports all unforbidden items marked for dumping to the cursor position.
- ``autodump destroy``
    Destroys all unforbidden items marked for dumping
