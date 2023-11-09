logistics
=========

.. dfhack-tool::
    :summary: Automatically mark and route items in monitored stockpiles.
    :tags: fort auto animals items stockpiles

Commands act upon the stockpile selected in the UI unless another stockpile
identifier is specified on the commandline.

When the plugin is enabled, it checks stockpiles marked with automelt,
autotrade, autodump, and/or autotrain features twice every in-game day, and
will mark valid items/animals in those stockpiles for melting, trading,
dumping, and/or training, respectively.

For autotrade, items will be marked for trading only when a caravan is
approaching or is already at the trade depot. Items (or bins that contain
items) of which a noble has forbidden export will not be marked for trade.

Stockpiles can be registered for ``logistics`` features by toggling the options
in the `stockpiles` overlay that comes up when you select a stockpile in the UI.

Usage
-----

::

    enable logistics
    logistics [status]
    logistics now
    logistics add [melt] [trade] [dump] [train] [<options>]
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
``-m``, ``--melt-masterworks``
    If specified with a ``logistics add melt`` command, will configure the
    stockpile to allow melting of masterworks. By default, masterworks are not
    marked for melting, even if they are in an automelt stockpile.
