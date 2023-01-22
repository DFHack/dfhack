automelt
========

.. dfhack-tool::
    :summary: Quickly designate items to be melted.
    :tags: untested fort productivity items stockpiles
    :no-command:

Automelt checks monitor-enabled stockpiles once every in-game day, and will mark valid items for melting.

Please see `gui/automelt` for the interactive configuration dialog.

Usage
-----

::

    enable automelt
    automelt [status]
    automelt (designate)
    automelt (monitor|nomonitor) <stockpile>

Examples
--------

Automatically designate all meltable items in the stockpile ("melt") for melting. ::

    enable automelt
    automelt monitor melt

Enable monitoring for the stockpile ("melt"), and mmediately designate all meltable items in monitored stockpiles for melting. ::

    automelt monitor melt
    automelt designate

Commands
--------

``status``
    Shows current configuration and relevant statistics

``designate``
    Designates items in monitored stockpiles for melting right now. This works even if ``automelt`` is not currently enabled.

``(no)monitor <stockpile>``
    Enable/disable monitoring of a given stockpile. Requires the stockpile to have a name set.
