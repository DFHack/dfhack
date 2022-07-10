misery
======
When enabled, fake bad thoughts will be added to all dwarves.

Usage:

:misery enable n:  enable misery with optional magnitude n. If specified, n must
    be positive.
:misery n:         same as "misery enable n"
:misery enable:    same as "misery enable 1"
:misery disable:   stop adding new negative thoughts. This will not remove
                   existing negative thoughts. Equivalent to "misery 0".
:misery clear:     remove fake thoughts, even after saving and reloading. Does
    not change factor.
