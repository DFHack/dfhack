probe
=====

.. dfhack-tool::
    :summary: Display low-level properties of the selected tile.
    :tags: adventure fort inspection map

.. dfhack-command:: bprobe
    :summary: Display low-level properties of the selected building.

.. dfhack-command:: cprobe
    :summary: Display low-level properties of the selected unit.

Usage:

``probe``
    Displays properties of the tile selected with :kbd:`k`. Some of these
    properties can be passed into `tiletypes`.
``bprobe``
    Displays properties of the building selected with :kbd:`q` or :kbd:`t`.
    For deeper inspection of the building, see `gui/gm-editor`.
``cprobe``
    Displays properties of the unit selected with :kbd:`v`. It also displays the
    IDs of any worn items. For deeper inspection of the unit and inventory items,
    see `gui/gm-unit` and `gui/gm-editor`.
