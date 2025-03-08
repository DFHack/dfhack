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

`spectate` will automatically disengage and turn itself off when you move the
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
    spectate toggle [<setting>]
    spectate set <setting> <value> [<subvalue>]
    spectate overlay enable|disable

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

``spectate overlay enable``
    Show informative tooltips that follow each unit on the map. Note that this
    can be enabled independently of `spectate` itself.

``spectate set tooltip-follow-job-shortenings "Store item in stockpile" "Store"``
    Abbreviate the names of "Store item in stockpile" jobs to just "Store" when the
    job is displayed in the `spectate` tooltips. See the
    ``tooltip-follow-job-shortenings`` setting below for details.

``spectate toggle tooltip-follow``
    Toggle the follow tooltips on or off.

Settings
--------

``auto-unpause`` (default: disabled)
    Toggle auto-dismissal of announcements that pause the game, like sieges,
    forgotten beasts, etc.

``cinematic-action`` (default: enabled)
    Toggle whether to switch targets more rapidly when there is conflict.

``follow-seconds`` (default: 10)
    Set the time interval for changing the followed unit. The interval does not
    include time that the game is paused.

``include-animals`` (default: disabled)
    Toggle whether to sometimes follow fort animals and wildlife.

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

``tooltip-follow`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    tooltips that follow onscreen dwarves around the map.

``tooltip-follow-blink-milliseconds`` (default: 3000)
    If the ``spectate.tooltip`` overlay is enabled, set the tooltip's blink
    duration in milliseconds. Set to 0 to always show.

``tooltip-follow-hold-to-show`` (default: none)
    If the ``spectate.tooltip`` overlay is enabled, specifies a modifier key
    (one of none, ctrl, alt, or shift) that has to be held to show the tooltips
    that follow onscreen dwarves around the map. This supersedes the
    ``tooltip-follow-blink-milliseconds`` option.

``tooltip-follow-job`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    job of the dwarf in the tooltip.

``tooltip-follow-job-shortenings`` (default: "Store item in stockpile" -> "Store item")
    If the ``spectate.tooltip`` overlay is enabled, this dictionary is used to
    shorten some job names, f.e. "Store item in stockpile" becomes "Store item".
    You can pass two parameters to ``spectate set tooltip-follow-job-shortenings`` to
    add or change elements in the dictionary. See the Examples section for an example.

``tooltip-follow-name`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    name of the dwarf in the tooltip.

``tooltip-follow-stress`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    happiness level (stress) of the dwarf in the tooltip.

``tooltip-follow-stress-levels`` (default: Displeased, Content, Pleased are disabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    specific happiness level (stress) of the dwarf in the tooltip. F.e.
    ``tooltip-follow-stress-levels 2 true`` would show the Displeased emoticon.
    See ``tooltip-stress-levels`` below for details.

``tooltip-hover`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    hover popup panel when your mouse cursor is over a unit.

``tooltip-hover-job`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    job of the dwarf in the hover panel.

``tooltip-hover-name`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    name of the dwarf in the hover panel.

``tooltip-hover-stress`` (default: enabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    happiness level (stress) of the dwarf in the hover panel.

``tooltip-hover-stress-levels`` (default: Displeased, Content, Pleased are disabled)
    If the ``spectate.tooltip`` overlay is enabled, toggle whether to show the
    specific happiness level (stress) of the dwarf in the hover panel. F.e.
    ``tooltip-hover-stress-levels 2 true`` would show the Displeased emoticon.
    See ``tooltip-stress-levels`` below for details.

``tooltip-stress-levels``
    Controls how happiness levels (stress) are displayed (emoticon and color).
    F.e. ``tooltip-stress-levels 6 text XD`` will change the emoticon for
    Ecstatic dwarves to ``XD``.
    Default values are:

.. list-table::
   :widths: 25 25 25 25
   :header-rows: 1

   * - Level
     - name
     - text
     - pen
   * - 0
     - Miserable
     - ``=C``
     - COLOR_RED
   * - 1
     - Unhappy
     - ``:C``
     - COLOR_LIGHTRED
   * - 2
     - Displeased
     - ``:(``
     - COLOR_YELLOW
   * - 3
     - Content
     - ``:]``
     - COLOR_GREY
   * - 4
     - Pleased
     - ``:)``
     - COLOR_GREEN
   * - 5
     - Happy
     - ``:D``
     - COLOR_LIGHTGREEN
   * - 6
     - Ecstatic
     - ``=D``
     - COLOR_LIGHTCYAN

Keep in mind that the text may look different when rendered in the game's font.

Overlays
--------

``spectate.tooltip``

``spectate`` can show informative tooltips that follow each unit on the map
and/or a popup panel with information when your mouse cursor hovers over a unit.

This overlay is managed via the `overlay` framework. It can be controlled via
the ``spectate overlay`` command or the ``Overlays`` tab in `gui/control-panel`.

``spectate.followpanel``

This overlay adds widgets to the vanilla follow panel -- the one that appears
in the lower left corner of the screen when you are following a unit. When you
are following a unit, regardless of whether the `spectate` plugin is enabled,
you can use the keyboard cursor left/right keys to switch which unit you are
following. There is also an indicator for whether spectate mode is enabled
(that is, whether the `spectate` plugin is enabled), and there is a button for
launching the `gui/spectate` configuration UI.
