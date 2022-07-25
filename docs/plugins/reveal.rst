.. _revflood:

reveal
======
This reveals the map. By default, HFS will remain hidden so that the demons
don't spawn. You can use ``reveal hell`` to reveal everything. With hell revealed,
you won't be able to unpause until you hide the map again. If you really want
to unpause with hell revealed, use ``reveal demons``.

Reveal also works in adventure mode, but any of its effects are negated once
you move. When you use it this way, you don't need to run ``unreveal``.

Usage and related commands:

:reveal:        Reveal the whole map, except for HFS to avoid demons spawning
:reveal hell:   Also show hell, but requires ``unreveal`` before unpausing
:reveal demon:  Reveals everything and allows unpausing - good luck!
:unreveal:      Reverts the effects of ``reveal``
:revtoggle:     Switches between ``reveal`` and ``unreveal``
:revflood:      Hide everything, then reveal tiles with a path to the cursor.
                Note that tiles behind constructed walls are also revealed as a
                workaround for :bug:`1871`.
:revforget:     Discard info about what was visible before revealing the map.
                Only useful where (e.g.) you abandoned with the fort revealed
                and no longer want the data.

nopause
=======
Disables pausing (both manual and automatic) with the exception of pause forced
by `reveal` ``hell``. This is nice for digging under rivers.
