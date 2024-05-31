.. _fix/occupancy:

fix-occupancy
=============

.. dfhack-tool::
    :summary: Provides the backend logic for fix/occupancy.
    :tags: fort bugfix map
    :no-command:

.. dfhack-command:: fix/occupancy
    :summary: Fix phantom occupancy issues.


If you see "unit blocking tile", "building present", or "items occupying site"
messages that you can't account for (:bug:`3499`), this tool can help.

Usage
-----

::

    fix/occupancy [<pos>] [<options>]

The optional position can be map coordinates in the form ``0,0,0``, without
spaces, or the string ``here`` to use the position of the keyboard cursor, if
active.

Examples
--------

``fix/occupancy``
    Fix all occupancy issues on the map.
``fix/occupancy here``
    Fix all occupancy issues on the tile selected by the keyboard cursor.
``fix-unit-occupancy -n``
    Report on, but do not fix, all occupancy issues on the map.

Options
-------

``-n``, ``--dry-run``
    Report issues, but do not write any changes to the map.
