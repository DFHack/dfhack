fix-unit-occupancy
==================

.. dfhack-tool::
    :summary: Fix phantom unit occupancy issues.
    :tags: unavailable

If you see "unit blocking tile" messages that you can't account for
(:bug:`3499`), this tool can help.

Usage
-----

::

    enable fix-unit-occupancy
    fix-unit-occupancy [here] [-n]
    fix-unit-occupancy interval <num_ticks>

When run without arguments (or with just the ``here`` or ``-n`` parameters),
the fix just runs once. You can also have it run periodically by enabling the
plugin.

Examples
--------

``fix-unit-occupancy``
    Run once and fix all occupancy issues on the map.
``fix-unit-occupancy -n``
    Report on, but do not fix, all occupancy issues on the map.

Options
-------

``here``
    Only operate on the tile at the cursor.
``-n``
    Report issues, but do not write any changes to the map.
``interval <num_ticks>``
    Set how often the plugin will check for and fix issues when it is enabled.
    The default is 1200 ticks, or 1 game day.
