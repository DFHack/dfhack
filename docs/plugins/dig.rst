.. _digv:
.. _digtype:

dig
===
This plugin makes many automated or complicated dig patterns easy.

Basic commands:

:digv:      Designate all of the selected vein for digging.
:digvx:     Also cross z-levels, digging stairs as needed.  Alias for ``digv x``.
:digl:      Like ``digv``, for layer stone.  Also supports an ``undo`` option
            to remove designations, for if you accidentally set 50 levels at once.
:diglx:     Also cross z-levels, digging stairs as needed.  Alias for ``digl x``.

:dfhack-keybind:`digv`

.. note::

    All commands implemented by the `dig` plugin (listed by ``ls dig``) support
    specifying the designation priority with ``-p#``, ``-p #``, or ``p=#``,
    where ``#`` is a number from 1 to 7. If a priority is not specified, the
    priority selected in-game is used as the default.
digcircle
=========
A command for easy designation of filled and hollow circles.
It has several types of options.

Shape:

:hollow:   Set the circle to hollow (default)
:filled:   Set the circle to filled
:#:        Diameter in tiles (default = 0, does nothing)

Action:

:set:      Set designation (default)
:unset:    Unset current designation
:invert:   Invert designations already present

Designation types:

:dig:      Normal digging designation (default)
:ramp:     Ramp digging
:ustair:   Staircase up
:dstair:   Staircase down
:xstair:   Staircase up/down
:chan:     Dig channel

After you have set the options, the command called with no options
repeats with the last selected parameters.

Examples:

``digcircle filled 3``
        Dig a filled circle with diameter = 3.
``digcircle``
        Do it again.
digtype
=======
For every tile on the map of the same vein type as the selected tile,
this command designates it to have the same designation as the
selected tile. If the selected tile has no designation, they will be
dig designated.
If an argument is given, the designation of the selected tile is
ignored, and all appropriate tiles are set to the specified
designation.

Options:

:dig:
:channel:
:ramp:
:updown: up/down stairs
:up:     up stairs
:down:   down stairs
:clear:  clear designation
digexp
======
This command is for :wiki:`exploratory mining <Exploratory_mining>`.

There are two variables that can be set: pattern and filter.

Patterns:

:diag5:            diagonals separated by 5 tiles
:diag5r:           diag5 rotated 90 degrees
:ladder:           A 'ladder' pattern
:ladderr:          ladder rotated 90 degrees
:clear:            Just remove all dig designations
:cross:            A cross, exactly in the middle of the map.

Filters:

:all:              designate whole z-level
:hidden:           designate only hidden tiles of z-level (default)
:designated:       Take current designation and apply pattern to it.

After you have a pattern set, you can use ``expdig`` to apply it again.

Examples:

``expdig diag5 hidden``
  Designate the diagonal 5 patter over all hidden tiles
``expdig``
  Apply last used pattern and filter
``expdig ladder designated``
  Take current designations and replace them with the ladder pattern
