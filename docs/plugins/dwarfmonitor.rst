dwarfmonitor
============

.. dfhack-tool::
    :summary: Report on dwarf preferences and efficiency.
    :tags: unavailable

It can also show heads-up display widgets with live fort statistics.

Usage
-----

``enable dwarfmonitor``
    Enable tracking of job efficiency for display on the ``dwarfmonitor stats``
    screen.
``dwarfmonitor stats``
    Show statistics and efficiency summary.
``dwarfmonitor prefs``
    Show a summary of preferences for dwarves in your fort.

Widget configuration
--------------------

The following widgets are registered for display on the main fortress mode
screen with the `overlay` framework:

``dwarfmonitor.cursor``
    Show the current keyboard and mouse cursor positions.
``dwarfmonitor.date``
    Show the in-game date.
``dwarfmonitor.misery``
    Show overall happiness levels of all dwarves.
``dwarfmonitor.weather``
    Show current weather (e.g. rain/snow).

They can be enabled or disable via the `overlay` command.

The :file:`dfhack-config/dwarfmonitor.json` file can be edited to change widget
configuration with any of the following fields:

* ``date_format`` (string): format for the ``dwarfmonitor.date`` widget:

    * ``Y`` or ``y``: The current year
    * ``M``: The current month, zero-padded if necessary
    * ``m``: The current month, *not* zero-padded
    * ``D``: The current day, zero-padded if necessary
    * ``d``: The current day, *not* zero-padded

    The default date format is ``Y-M-D``, per the ISO8601_ standard.

* ``coords_type`` (string): the coordinate type to show in the ``dwarfmonitor.cursor`` widget:

    * ``all`` (the default): show all of the coordinate types listed here
    * ``mouse_ui``: the X/Y UI coordinates of the tile the mouse is positioned over
    * ``mouse_map``: the X/Y/Z map coordinates of the tile the mouse is
      positioned over (only if over the map)
    * ``keyboard_map``: the X/Y/Z map coordinates of the tile selected by the
      keyboard-controlled ``X`` cursor in DF (if active)

* ``coords_short`` (boolean, default: ``false``): if ``true``, hides explanatory
  text from the ``dwarfmonitor.cursor`` widget, and only shows coordinates as
  ``(X,Y,Z)``

Example configuration file:

.. code-block:: json

    {
        "date_format": "m/d/y",
        "coords_type": "mouse_map",
        "coords_short": false
    }

.. _ISO8601: https://en.wikipedia.org/wiki/ISO_8601
