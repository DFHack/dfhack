spectate
========

.. dfhack-tool::
    :summary: Automatically follow productive dwarves.
    :tags: fort interface

Usage
-----

::

    enable spectate
    spectate
    spectate set <setting> <value>
    spectate enable|disable <feature>

When enabled, the plugin will lock the camera to following the dwarves
scurrying around your fort. Every once in a while, it will automatically switch
to following a different dwarf, preferring dwarves on z-levels with the highest
job activity.

If you have the ``auto-disengage`` feature disabled, you can switch to a new
dwarf immediately by hitting one of the map movement keys (``wasd`` by
default). To stop following dwarves, bring up `gui/launcher` and run
``disable spectate``.

Changes to settings will be saved with your fort, but if `spectate` is enabled
when you save the fort, it will disenable itself when you load so you can get
your bearings before re-enabling follow mode with ``enable spectate`` again.

Examples
--------

``enable spectate``
    Starting following dwarves and observing life in your fort.

``spectate``
    The plugin reports its configured status.

``spectate enable auto-unpause``
    Enable the spectate plugin to automatically dismiss pause events caused
    by the game. Siege events are one example of such a game event.

``spectate set tick-threshold 1000``
    Set the tick interval between camera changes back to its default value.

Features
--------
:auto-unpause:   Toggle auto-dismissal of game pause events. (default: disabled)
:auto-disengage: Toggle auto-disengagement of plugin through player
                 intervention while unpaused. (default: disabled)
:animals:        Toggle whether to sometimes follow animals. (default: disabled)
:hostiles:       Toggle whether to sometimes follow hostiles (eg. undead,
                 titans, invaders, etc.) (default: disabled)
:visiting:       Toggle whether to sometimes follow visiting units (eg.
                 diplomats)

Settings
--------
:tick-threshold: Set the plugin's tick interval for changing the followed
                 dwarf. (default: 1000)
