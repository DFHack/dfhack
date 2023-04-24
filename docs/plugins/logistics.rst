logistics
=========

.. dfhack-tool::
    :summary: Automatically mark and route items in monitored stockpiles.
    :tags: fort productivity items stockpiles

Commands act upon the stockpile selected in the UI unless another stockpile
identifier is specified on the commandline.

When the plugin is enabled, it checks stockpiles marked with automelt,
autotrade, and/or autodump features twice every in-game day, and will mark valid
items in those stockpiles for melting, trading, and/or dumping, respectively.
Note that items will only be marked for trading if a caravan is approaching or
is already at the trade depot.

Please see `gui/logistics` for the interactive status and configuration dialog.

Usage
-----

::

    enable logistics
    logistics [status]
    logistics now
    logistics add [melt] [trade] [dump] [<options>]
    logistics clear [all] [<options>]

Examples
--------

``logistics``
    Print a summary of all your stockpiles, their ``logistics`` configuration,
    and the number of items that are designated (or can be designated) by each
    of the ``logistics`` processors.

``logistics now``
    Designate items in monitored stockpiles according to the current
    configuration. This works regardless of whether ``logistics`` is currently
    enabled.

``logistics add melt``
    Register the currently selected stockpile for automelting. Meltable items
    that are brought to this stockpile will be designated for melting.

``logistics add melt trade -s goblinite``
    Register the stockpile(s) named "goblinite" for automatic melting and
    automatic trading. Items will be marked for melting, but any items still in
    the stockpile when a caravan shows up will be brought to the trade depot
    for trading.

``logistics clear``
    Unregisters the currently selected stockpile from any monitoring. Any
    currently designated items will remain designated.

``logistics clear -s 12,15,goblinite``
    Unregisters the stockpiles with stockpile numbers 12 and 15, along with any
    stockpiles named "goblinite", from any monitoring.

``logistics clear all``
    Unregister all stockpiles from any monitoring.

Options
-------

``-s``, ``--stockpile <name or number>[,<name or number>...]``
    Causes the command to act upon stockpiles with the given names or numbers
    instead of the stockpile that is currently selected in the UI. Note that
    the numbers are the stockpile numbers, not the building ids.
