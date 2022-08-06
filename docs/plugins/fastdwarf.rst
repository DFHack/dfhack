fastdwarf
=========
**Tags:** `tag/fort`, `tag/armok`, `tag/units`
:dfhack-keybind:`fastdwarf`

Dwarves teleport and/or finish jobs instantly.

Usage::

    enable fastdwarf
    fastdwarf <speed mode> [<tele mode>]

Examples
--------

``fastdwarf 1``
    Make all your dwarves move and work at maximum speed.
``fastdwarf 1 1``
    In addition to working at maximum speed, dwarves also teleport to their
    destinations.

Options
-------

Speed modes:

:0: Dwarves move and work at normal rates.
:1: Dwarves move and work at maximum speed.
:2: ALL units move (and work) at maximum speed, including creatures and
    hostiles.

Tele modes:

:0: No teleportation.
:1: Dwarves teleport to their destinations.
