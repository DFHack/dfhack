fastdwarf
=========

.. dfhack-tool::
    :summary: Citizens walk fast and and finish jobs instantly.
    :tags: fort armok units

Usage
-----

::

    enable fastdwarf
    fastdwarf [status]
    fastdwarf <fast mode> [<tele mode>]

Examples
--------

``enable fastdwarf`` or ``fastdwarf 1``
    Make all your citizens move and work at maximum speed.
``fastdwarf``
    Print out current configuration.
``fastdwarf 1 1``
    In addition to working at maximum speed, dwarves also teleport to their
    destinations. It is possible that units end up in places they shouldn't be,
    and you may need to `teleport` stranded units back to safety.

Options
-------

Fast modes:

:0: Citizens move and work at normal rates.
:1: Citizens move and work at maximum speed.
:2: ALL units move (and work) at maximum speed, including creatures, visitors,
    long-term residents, and hostiles.

Tele modes:

:0: No teleportation.
:1: Citizens teleport to their job destinations.

Note that a dwarf will only teleport when:

- They are not pushing anything (like a wheelbarrow)
- They are not dragging anything/anyone or being dragged
- They are not following anyone or being followed
- They have a job with a valid destination

So you may still see dwarves walking normally or just standing still if they do
not have a job to teleport to. Since jobs get done so much faster, you will
likely see groups of dwarves just standing around, with individual dwarves
periodically disappaearing and reappearing moments later when they have a job
to do.
