automelt
========

.. dfhack-tool::
    :summary: Quickly designate items to be melted.
    :tags: fort productivity items stockpiles
    :no-command:

Automelt checks monitor-enabled stockpiles once every in-game day, and will mark valid items for melting.

Please see `gui/automelt` for the interactive configuration dialog.

Usage
-----

::

    enable automelt
    automelt [status]
    automelt designate
    automelt (monitor|nomonitor) <stockpile>[,<stockpile>...]

Examples
--------

Automatically monitor stockpile ("melt"), marking new valid items for melting. This also immediately marks all present items for melting::

    enable automelt
    automelt monitor melt

Enable monitoring for ("Stockpile #52"), which has not been given a custom name::

    automelt monitor "Stockpile #52"

Enable monitoring for ("Stockpile #52"), which has not been given a custom name::

    automelt monitor 52

Enable monitoring for multiple stockpiles ("Stockpile #52", "Stockpile #35", and "melt")::

    automelt monitor 52,"Stockpile #35",melt

Commands
--------

``status``
    Shows current configuration and relevant statistics

``designate``
    Designates items in monitored stockpiles for melting right now. This works even if ``automelt`` is not currently enabled.

``(no)monitor <stockpile>``
    Enable/disable monitoring of a given stockpile. Works with either the stockpile's name (if set) or ID.
    If the stockpile has no custom name set, you may designate it by either the full name as reported by
    the status command, or by just the number.
