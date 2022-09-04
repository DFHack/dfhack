spectate
========

.. dfhack-tool::
    :summary: Automatically follow productive dwarves.
    :tags: fort interface

Usage
-----

::

    enable spectate
    disable spectate
    spectate <option> <value>


When enabled, the plugin will automatically switch which dwarf is being
followed periodically, preferring dwarves on z-levels with the highest
job activity.

To set features that toggle between two states, use {0,1} to specify
which state the feature should be in. Anything else will take any positive
value.

Examples
--------

``spectate``
    The plugin reports its feature status.


``spectate auto-unpause 1``
    Enable the spectate plugin to automatically dismiss pause events caused
    by the game. Siege events are one example of such a game event.

``spectate tick-interval 50``
    Set the tick interval the followed dwarf can be changed at back to its
    default value.

Options
-------

:no option:      Show plugin status.
:tick-threshold: Set the plugin's tick interval for changing the followed dwarf.
                 Acts as a maximum wait time when used with focus-jobs.
:focus-jobs:     Toggle whether the plugin should always be following a job.
:auto-unpause:   Toggle auto-dismissal of game pause events.
:auto-disengage: Toggle auto-disengagement of plugin through player intervention.
