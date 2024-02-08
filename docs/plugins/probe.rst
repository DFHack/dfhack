probe
=====

.. dfhack-tool::
    :summary: Display low-level properties of the selected tile.
    :tags: adventure fort inspection buildings map units

.. dfhack-command:: bprobe
    :summary: Display low-level properties of the selected building.

.. dfhack-command:: cprobe
    :summary: Display low-level properties of the selected unit.

Usage
-----

``probe [<options>]``
    Displays properties of the tile highlighted with the keyboard cursor or at
    the specified coordinates. If you do not have a keyboard cursor visible,
    enter a mode that shows the keyboard cursor (like mining mode). If the
    keyboard cursor is still not visible, hit Alt-k to invoke
    `toggle-kbd-cursor`.
``bprobe``
    Displays properties of the selected building. For deeper inspection of the
    building, see `gui/gm-editor`.
``cprobe``
    Displays properties of the selected unit. It also displays the IDs of any
    worn items. For deeper inspection of the unit and inventory items, see
    `gui/gm-unit` and `gui/gm-editor`.

Options
-------

``-c``, ``--cursor <x>,<y>,<z>``
    Use the specified map coordinates instead of the current keyboard cursor.
    If this option is specified, then an active keyboard cursor is not
    necessary.
