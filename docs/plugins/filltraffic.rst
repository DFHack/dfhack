.. _restrictice:
.. _restrictliquids:

filltraffic
===========

.. dfhack-tool::
    :summary: Set traffic designations using flood-fill starting at the cursor.
    :tags: fort design productivity map

.. dfhack-command:: alltraffic
    :summary: Set traffic designations for every single tile of the map.

.. dfhack-command:: restrictice
    :summary: Restrict traffic on all tiles on top of visible ice.

.. dfhack-command:: restrictliquids
    :summary: Restrict traffic on all visible tiles with liquid.

Usage
-----

::

    filltraffic <designation> [<options>]
    alltraffic <designation>
    restrictice
    restrictliquids

For ``filltraffic``, flood filling stops at walls and doors.

Examples
--------

``filltraffic H``
    When used in a room with doors, it will set traffic to HIGH in just that
    room.

Options
-------

Traffic designations:

:H:     High Traffic.
:N:     Normal Traffic.
:L:     Low Traffic.
:R:     Restricted Traffic.

Filltraffic extra options:

:X:     Fill across z-levels.
:B:     Include buildings and stockpiles.
:P:     Include empty space.
