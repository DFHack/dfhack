autodump
========
This plugin adds an option to the :kbd:`q` menu for stckpiles when `enabled <enable>`.
When autodump is enabled for a stockpile, any items placed in the stockpile will
automatically be designated to be dumped.

Alternatively, you can use it to quickly move all items designated to be dumped.
Items are instantly moved to the cursor position, the dump flag is unset,
and the forbid flag is set, as if it had been dumped normally.
Be aware that any active dump item tasks still point at the item.

Cursor must be placed on a floor tile so the items can be dumped there.

Options:

:destroy:            Destroy instead of dumping. Doesn't require a cursor.
                     If called again before the game is resumed, cancels destroy.
:destroy-here:       As ``destroy``, but only the selected item in the :kbd:`k` list,
                     or inside a container.
                     Alias ``autodump-destroy-here``, for keybindings.
                     :dfhack-keybind:`autodump-destroy-here`
:visible:            Only process items that are not hidden.
:hidden:             Only process hidden items.
:forbidden:          Only process forbidden items (default: only unforbidden).

``autodump-destroy-item`` destroys the selected item, which may be selected
in the :kbd:`k` list, or inside a container. If called again before the game
is resumed, cancels destruction of the item.
:dfhack-keybind:`autodump-destroy-item`
