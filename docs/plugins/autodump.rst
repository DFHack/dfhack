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


                     Alias ``autodump-destroy-here``, for keybindings.
                     :dfhack-keybind:`autodump-destroy-here`

``autodump-destroy-item`` destroys the selected item, which may be selected
in the :kbd:`k` list, or inside a container. If called again before the game
is resumed, cancels destruction of the item.
:dfhack-keybind:`autodump-destroy-item`



 commands.push_back(PluginCommand(
        "autodump", "Teleport items marked for dumping to the cursor.",
        df_autodump, false,
        "  This utility lets you quickly move all items designated to be dumped.\n"
        "  Items are instantly moved to the cursor position, the dump flag is unset,\n"
        "  and the forbid flag is set, as if it had been dumped normally.\n"
        "  Be aware that any active dump item tasks still point at the item.\n"
        "Options:\n"
        "  destroy       - instead of dumping, destroy the items instantly.\n"
        "  destroy-here  - only affect the tile under cursor.\n"
        "  visible       - only process items that are not hidden.\n"
        "  hidden        - only process hidden items.\n"
        "  forbidden     - only process forbidden items (default: only unforbidden).\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-here", "Destroy items marked for dumping under cursor.",
        df_autodump_destroy_here, Gui::cursor_hotkey,
        "  Identical to autodump destroy-here, but intended for use as keybinding.\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-item", "Destroy the selected item.",
        df_autodump_destroy_item, Gui::any_item_hotkey,
        "  Destroy the selected item. The item may be selected\n"
        "  in the 'k' list, or inside a container. If called\n"
        "  again before the game is resumed, cancels destroy.\n"
    ));
    return CR_OK;
