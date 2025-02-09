spectate
========

.. dfhack-tool::
    :summary: Automated spectator mode.
    :tags: fort inspection interface

This tool is for those who like to watch their dwarves go about their business.

When enabled, `spectate` will lock the camera to following the dwarves
scurrying around your fort. Every once in a while, it will automatically switch
to following a different dwarf. It can also switch to following animals,
hostiles, or visiting units. You can switch to the next target (or a previous
target) immediately with the left/right arrow keys.

By default, `spectate` will disengage and turn itself off when you move the
map, just like the vanilla follow mechanic. It will also disengage immediately
if you open the squads menu for military action.

It can also annotate your dwarves on the map with their name, job, and other
information, either as floating tooltips or in a panel that comes up when you
hover the mouse over a target.

Run `gui/spectate` to configure the plugin's settings.

Settings are saved globally, so your preferences for `spectate` and its
overlays will apply to all forts, not just the currently loaded one. Follow
mode is automatically disabled when you load a fort so you can get your
bearings before re-enabling.

Usage
-----

::

    enable spectate
    spectate [status]
    spectate toggle
    spectate set <setting> <value>
    spectate overlay <name> enable|disable

Examples
--------

``enable spectate``
    Start following dwarves and observing life in your fort.

``spectate toggle``
    Toggle the plugin on or off. Intended for use with a keybinding. The
    default is Ctrl-Shift-S.

``spectate``
    The plugin reports its configured status.

``spectate set auto-unpause true``
    Configure `spectate` to automatically dismiss popups and pause events, like
    siege announcements.

``spectate set follow-seconds 30``
    Configure `spectate` to switch targets every 30 seconds when in follow mode.

``spectate overlay follow enable``
    Show informative tooltips that follow each unit on the map.

Settings
--------

``auto-disengage`` (default: enabled)
    Toggle automatically disabling the plugin when the player moves the map or
    opens the squad panel. If this is disabled, you will need to manually
    disable the plugin to turn off follow mode.

``auto-unpause`` (default: disabled)
    Toggle auto-dismissal of announcements that pause the game, like sieges,
    forgotten beasts, etc.

``cinematic-action`` (default: enabled)
    Toggle whether to switch targets more rapidly when there is conflict.

``follow-seconds`` (default: 10)
    Set the time interval for changing the followed unit. The interval does not
    include time that the game is paused.

``include-animals`` (default: disabled)
    Toggle whether to sometimes follow fort animals.

``include-hostiles`` (default: disabled)
    Toggle whether to sometimes follow hostiles (eg. undead, titans, invaders,
    etc.)

``include-visitors`` (default: disabled)
    Toggle whether to sometimes follow visiting units, like diplomats.

``include-wildlife`` (default: disabled)
    Toggle whether to sometimes follow wildlife.

``prefer-conflict`` (default: enabled)
    Toggle whether to prefer following units in active conflict.

``prefer-new-arrivals`` (default: enabled)
    Toggle whether to prefer following (non-siege) units that have newly
    arrived on the map.

``tooltip-follow-job`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    job of the dwarf in the tooltip.

``tooltip-follow-name`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    name of the dwarf in the tooltip.

``tooltip-follow-stress`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    happiness level (stress) of the dwarf in the tooltip.

``tooltip-hover-job`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    job of the dwarf in the hover panel.

``tooltip-hover-name`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    name of the dwarf in the hover panel.

``tooltip-hover-stress`` (default: enabled)
    If the ``spectate.follow`` overlay is enabled, toggle whether to show the
    happiness level (stress) of the dwarf in the hover panel.

Overlays
--------

``spectate`` provides two overlays via the `overlay` framework to add
information and functionality to the main map. These overlays can be controlled
via the ``spectate overlay`` command or the ``Overlays`` tab in
`gui/control-panel`.

The information displayed by these overlays can be configured via the
``spectate set`` command or the `gui/spectate` interface.

``spectate.follow``
    Show informative tooltips that follow each unit on the map. You can enable
    this overlay by running ``spectate overlay follow enable`` or,
    equivalently, ``overlay enable spectate.follow``.

``spectate.hover``
    Show a popup panel with selected information when your mouse cursor hovers
    over a unit. You can enable this overlay by running
    ``spectate overlay hover enable`` or, equivalently,
    ``overlay enable spectate.hover``.
