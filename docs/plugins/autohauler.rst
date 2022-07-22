autohauler
==========

Tags:
:dfhack-keybind:`autohauler`

:index:`Automatically manage hauling labors.
<autohauler; Automatically manage hauling labors.>` Similar to `autolabor`, but
instead of managing all labors, ``autohauler`` only addresses hauling labors,
leaving the assignment of skilled labors entirely up to you. You can use the
in-game `manipulator` UI or an external tool like Dwarf Therapist to do so.

Idle dwarves who are not on active military duty will be assigned the hauling
labors; everyone else (including those currently hauling) will have the hauling
labors removed. This is to encourage every dwarf to do their assigned skilled
labors whenever possible, but resort to hauling when those jobs are not
available. This also implies that the user will have a very tight skill
assignment, with most skilled labors only being assigned to just a few dwarves
and almost every non-military dwarf having at least one skilled labor assigned.

Autohauler allows a skill to be used as a flag to exempt a dwarf from
``autohauler``'s effects. By default, this is the unused ALCHEMIST labor, but it
can be changed by the user.

Usage:

- ``enable autohauler``
    Start managing hauling labors. This is normally all you need to do.
    Autohauler works well on default settings.
- ``autohauler status``
    Show autohauler status and status of fort dwarves.
- ``autohauler <labor> haulers``
    Set whether a particular labor should be assigned to haulers.
- ``autohauler <labor> allow|forbid``
    Set whether a particular labor should mark a dwarf as exempt from hauling.
    By default, only the ``ALCHEMIST`` labor is set to ``forbid``.
- ``autohauler reset-all|<labor> reset``
    Reset a particular labor (or all labors) to their default
    haulers/allow/forbid state.
- ``autohauler list``
    Show the active configuration for all labors.
- ``autohauler frameskip <number>``
    Set the number of frames between runs of autohauler.
- ``autohauler debug``
    In the next cycle, output the state of every dwarf.

Examples
--------

- ``autohauler HAUL_STONE haulers``
    Set stone hauling as a hauling labor (this is already the default).
- ``autohauler RECOVER_WOUNDED allow``
    Allow the "Recover wounded" labor to be manually assigned by the player. By
    default it is automatically given to haulers.
- ``autohauler MINE forbid``
    Don't assign hauling labors to dwarves with the Mining labor enabled.
