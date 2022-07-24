.. _digv:
.. _digtype:

dig
===
Tags:
:dfhack-keybind:`digv`
:dfhack-keybind:`digvx`
:dfhack-keybind:`digl`
:dfhack-keybind:`diglx`
:dfhack-keybind:`digcircle`
:dfhack-keybind:`digtype`
:dfhack-keybind:`digexp`

Make complicated dig patterns easy.

Usage:

``digv [x] [-p<number>]``
    :index:`Designate all of the selected vein for digging.
    <digv; Designate all of the selected vein for digging.>`
``digvx [-p<number>]``
    :index:`Dig a vein across z-levels, digging stairs as needed.
    <digvx; Dig a vein across z-levels, digging stairs as needed.>`
    This is an alias for ``digv x``.
``digl [x] [undo] [-p<number>]``
    :index:`Dig all of the selected layer stone.
    <digl; Dig all of the selected layer stone.>` If ``undo`` is specified,
    removes the designation instead (for if you accidentally set 50 levels at
    once).
``diglx [-p<number>]``
    :index:`Dig layer stone across z-levels, digging stairs as needed.
    <diglx; Dig layer stone across z-levels, digging stairs as needed.>` This
    is an alias for ``digl x``.
``digcircle [<diameter>] [<solidity>] [<action>] [<designation>] [-p<number>]``
    :index:`Designate circles. <digcircles; Designate circles.>` The diameter
    is the number of tiles across the center of the circle that you want to dig.
    See the `digcircle`_ section below for options.
``digtype [<designation>] [-p<number>]``
    :index:`Designate all vein tiles of the selected type.
    <digtype; Designate all vein tiles of the selected type.>` See the `digtype`_
    section below for options.
``digexp [<pattern>] [<filter>] [-p<number>]``
    :index:`Designate dig patterns for exploratory mining.
    <digexp; Designate dig patterns for exploratory mining.>` See the `digexp`_
    section below for options

All commands support specifying the priority of the dig designations with
``-p<number>``, where the number is from 1 to 7. If a priority is not specified,
the priority selected in-game is used as the default.

Examples
--------

``digcircle filled 3 -p2``
    Dig a filled circle with a diameter of 3 tiles at dig priority 2.
``digcircle``
    Do it again (previous parameters are reused).
``expdig diag5 hidden``
    Designate the diagonal 5 pattern over all hidden tiles on the current
    z-level.
``expdig ladder designated``
    Take existing designations on the current z-level and replace them with the
    ladder pattern.
``expdig``
    Do it again (previous parameters are reused).

digcircle
---------

The ``digcircle`` command can accept up to one option of each type below.

Solidity options:

``hollow``
    Designates hollow circles (default).
``filled``
    Designates filled circles.

Action options:

``set``
    Set designation (default).
``unset``
    Unset current designation.
``invert``
    Invert designations already present.

Designation options:

``dig``
    Normal digging designation (default).
``ramp``
    Dig ramps.
``ustair``
    Dig up staircases.
``dstair``
    Dig down staircases.
``xstair``
    Dig up/down staircases.
``chan``
    Dig channels.

After you have set the options, the command called with no options repeats with
the last selected parameters.

digtype
-------

For every tile on the map of the same vein type as the selected tile, this
command designates it to have the same designation as the selected tile. If the
selected tile has no designation, they will be dig designated.

If an argument is given, the designation of the selected tile is ignored, and
all appropriate tiles are set to the specified designation.

Designation options:

``dig``
    Normal digging designation.
``channel``
    Dig channels.
``ramp``
    Dig ramps.
``updown``
    Dig up/down staircases.
``up``
    Dig up staircases.
``down``
    Dig down staircases.
``clear``
    Clear any designations.

digexp
------

This command is for :wiki:`exploratory mining <Exploratory_mining>`.

There are two variables that can be set: pattern and filter.

Patterns:

``diag5``
    Diagonals separated by 5 tiles.
``diag5r``
    The diag5 pattern rotated 90 degrees.
``ladder``
    A 'ladder' pattern.
``ladderr``
    The ladder pattern rotated 90 degrees.
``cross``
    A cross, exactly in the middle of the map.
``clear``
    Just remove all dig designations.

Filters:

``hidden``
    Designate only hidden tiles of z-level (default)
``all``
    Designate the whole z-level.
``designated``
    Take current designation and apply the selected pattern to it.

After you have a pattern set, you can use ``expdig`` to apply it again.
