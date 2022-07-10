dwarfmonitor
============
Records dwarf activity to measure fort efficiency.

Options:

:enable <mode>:     Start monitoring ``mode``. ``mode`` can be "work", "misery",
                    "weather", or "all".  This will enable all corresponding widgets,
                    if applicable.
:disable <mode>:    Stop monitoring ``mode``, and disable corresponding widgets, if applicable.
:stats:             Show statistics summary
:prefs:             Show dwarf preferences summary
:reload:            Reload configuration file (``dfhack-config/dwarfmonitor.json``)

:dfhack-keybind:`dwarfmonitor`

Widget configuration:

The following types of widgets (defined in :file:`hack/lua/plugins/dwarfmonitor.lua`)
can be displayed on the main fortress mode screen:

:date:      Show the in-game date
:misery:    Show overall happiness levels of all dwarves
:weather:   Show current weather (rain/snow)
:cursor:    Show the current mouse cursor position

The file :file:`dfhack-config/dwarfmonitor.json` can be edited to control the
positions and settings of all widgets displayed. This file should contain a
JSON object with the key ``widgets`` containing an array of objects - see the
included file in the ``dfhack-config`` folder for an example:

.. code-block:: lua

    {
        "widgets": [
            {
                "type": "widget type (weather, misery, etc.)",
                "x": X coordinate,
                "y": Y coordinate
                <...additional options...>
            }
        ]
    }

X and Y coordinates begin at zero (in the upper left corner of the screen).
Negative coordinates will be treated as distances from the lower right corner,
beginning at 1 - e.g. an x coordinate of 0 is the leftmost column, while an x
coordinate of 1 is the rightmost column.

By default, the x and y coordinates given correspond to the leftmost tile of
the widget. Including an ``anchor`` option set to ``right`` will cause the
rightmost tile of the widget to be located at this position instead.

Some widgets support additional options:

* ``date`` widget:

  * ``format``: specifies the format of the date. The following characters
    are replaced (all others, such as punctuation, are not modified)

    * ``Y`` or ``y``: The current year
    * ``M``: The current month, zero-padded if necessary
    * ``m``: The current month, *not* zero-padded
    * ``D``: The current day, zero-padded if necessary
    * ``d``: The current day, *not* zero-padded

    The default date format is ``Y-M-D``, per the ISO8601_ standard.

    .. _ISO8601: https://en.wikipedia.org/wiki/ISO_8601

* ``cursor`` widget:

  * ``format``: Specifies the format. ``X``, ``x``, ``Y``, and ``y`` are
    replaced with the corresponding cursor cordinates, while all other
    characters are unmodified.
  * ``show_invalid``: If set to ``true``, the mouse coordinates will both be
    displayed as ``-1`` when the cursor is outside of the DF window; otherwise,
    nothing will be displayed.
