spectate
========

.. dfhack-tool::
    :summary: Automatically follow productive dwarves.
    :tags: untested fort interface

Usage
-----

::

    enable spectate
    spectate set <setting> <value>
    spectate enable|disable <feature>


When enabled, the plugin will automatically switch which dwarf is being
followed periodically, preferring dwarves on z-levels with the highest
job activity.

Changes to plugin settings will be saved per world. Whether the plugin itself
is enabled or not is not saved.

Examples
--------

``spectate``
    The plugin reports its configured status.

``spectate enable auto-unpause``
    Enable the spectate plugin to automatically dismiss pause events caused
    by the game. Siege events are one example of such a game event.

``spectate set tick-threshold 50``
    Set the tick interval the followed dwarf can be changed at back to its
    default value.

Features
--------
:auto-unpause:   Toggle auto-dismissal of game pause events. (default: disabled)
:auto-disengage: Toggle auto-disengagement of plugin through player intervention while unpaused. (default: disabled)
:animals:        Toggle whether to sometimes follow animals. (default: disabled)
:hostiles:       Toggle whether to sometimes follow hostiles (eg. undead, titan, invader, etc.) (default: disabled)
:visiting:       Toggle whether to sometimes follow visiting units (eg. diplomat)

Settings
--------
:tick-threshold: Set the plugin's tick interval for changing the followed dwarf. (default: 1000)
