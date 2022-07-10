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
