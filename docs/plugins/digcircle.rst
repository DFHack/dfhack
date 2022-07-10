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
