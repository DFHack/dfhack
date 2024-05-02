workNow
=======

.. dfhack-tool::
    :summary: Reduce the time that dwarves idle after completing a job.
    :tags: untested fort auto labors

After finishing a job, dwarves will wander away for a while before picking up a
new job. This plugin will automatically poke the game to assign dwarves to new
tasks.

Usage
-----

``workNow``
    Print current plugin status.
``workNow 0``
    Stop monitoring and poking.
``workNow 1``
    Poke the game to assign dwarves to tasks whenever the game is paused.
``workNow 2``
    Poke the game to assign dwarves to tasks whenever a dwarf finishes a job.
