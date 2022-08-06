.. _restrictice:
.. _restrictliquids:

filltraffic
===========
**Tags:** `tag/fort`, `tag/productivity`, `tag/design`, `tag/map`
:dfhack-keybind:`filltraffic`
:dfhack-keybind:`alltraffic`
:dfhack-keybind:`restrictice`
:dfhack-keybind:`restrictliquids`

Usage:

``filltraffic <designation> [<options>]``
    Set traffic designations using flood-fill starting at the cursor. Flood
    filling stops at walls and doors.
``alltraffic <designation>``
    Set traffic designations for every single tile of the map - useful for
    resetting traffic designations.
``restrictice``
    Restrict traffic on all tiles on top of visible ice.
``restrictliquids``
    Restrict traffic on all visible tiles with liquid.

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
