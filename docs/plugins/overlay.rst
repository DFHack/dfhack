overlay
=======

.. dfhack-tool::
    :summary: Manage on-screen overlay widgets.
    :tags: dfhack interface

The overlay framework manages the on-screen widgets that other tools (including
3rd party plugins and scripts) can register for display. For a graphical
configuration interface, please see `gui/control-panel`. If you are a developer
who wants to write an overlay widget, please see the `overlay-dev-guide`.

Usage
-----

``enable overlay``
    Display enabled widgets.
``overlay enable|disable all|<name or list number> [<name or list number> ...]``
    Enable/disable all or specified widgets. Widgets can be specified by either
    their name or their number, as returned by ``overlay list``.
``overlay list [<filter>]``
    Show a list of all the widgets that are registered with the overlay
    framework, optionally filtered by the given filter string.
``overlay position <name or list number> [default|<x> <y>]``
    Display configuration information for the given widget or change the
    position where it is rendered. See the `Widget position`_ section below for
    details.
``overlay trigger <name or list number>``
    Intended to be used by keybindings for manually triggering a widget. For
    example, you could use an ``overlay trigger`` keybinding to show a menu that
    normally appears when you hover the mouse over a screen hotspot.

Examples
--------

``overlay enable all``
    Enable all widgets. Note that they will only be displayed on the screens
    that they are associated with. You can see which screens a widget will be
    displayed on, along with whether the widget is a hotspot, by calling
    ``overlay position``.
``overlay position hotkeys.menu``
    Show the current configuration of the `hotkeys` menu widget.
``overlay position dwarfmonitor.cursor -2 -3``
    Display the `dwarfmonitor` cursor position reporting widget in the lower
    right corner of the screen, 2 tiles from the left and 3 tiles from the
    bottom.
``overlay position dwarfmonitor.cursor default``
    Reset the `dwarfmonitor` cursor position to its default.
``overlay trigger hotkeys.menu``
    Trigger the `hotkeys` menu widget so that it shows its popup menu. This is
    what is run when you hit :kbd:`Ctrl`:kbd:`Shift`:kbd:`C`.
``overlay trigger notes.map_notes add Kitchen``
    Trigger the `notes` overlay widget so that it shows its new note popup
    with given title.

Widget position
---------------

Widgets can be positioned at any (``x``, ``y``) position on the screen, and can
be specified relative to any edge. Coordinates are 1-based, which means that
``1`` is the far left column (for ``x``) or the top row (for ``y``). Negative
numbers are measured from the right of the screen to the right edge of the
widget or from the bottom of the screen to the bottom of the widget,
respectively.

For easy reference, the corners can be found at the following coordinates:

:(1, 1): top left corner
:(-1, 1): top right corner
:(1, -1): lower left corner
:(-1, -1): lower right corner

Overlay
-------

The `overlay` plugin also provides a standard overlay itself:
``title_version``, which displays the DFHack version on the DF title screen,
along with quick links to `quickstart-guide` and `gui/control-panel`.
