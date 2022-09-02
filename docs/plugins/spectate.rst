spectate
========

.. dfhack-tool::
    :summary: Automatically follow exciting dwarves.
    :tags: fort interface
    :no-command:

Usage
-----

::

    spectate
    spectate <option>


The plugin will automatically switch which dwarf is being followed periodically,
preferring dwarves on z-levels with the highest job activity.

Examples
--------

``spectate``
    See the status of spectate, what is enabled or disabled.

``spectate enable``
    Enable spectate plugin to pseudo-randomly follow dwarves around.

``spectate auto-unpause``
    Enable the spectate plugin to automatically dismiss pause events caused
    by the game. Siege events are one example of such a game event.

Options
-------

:no option:     Show plugin status.
:enable:        Enable plugin.
:disable:       Disable plugin.
:auto-unpause:  Toggle auto-dismissal of game pause events.
