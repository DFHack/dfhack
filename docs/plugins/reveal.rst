.. _revflood:

reveal
======
**Tags:** `tag/adventure`, `tag/fort`, `tag/inspection`, `tag/armok`, `tag/map`
:dfhack-keybind:`reveal`
:dfhack-keybind:`unreveal`
:dfhack-keybind:`revforget`
:dfhack-keybind:`revtoggle`
:dfhack-keybind:`revflood`
:dfhack-keybind:`nopause`

Reveals the map. This reveals all z-layers in fort mode. It also works in
adventure mode, but any of its effects are negated once you move. When you use
it this way, you don't need to run ``unreveal`` to hide the map again.

Usage:

``reveal [hell|demon]``
    Reveal the whole map. If ``hell`` is specified, also reveal HFS areas, but
    you are required to run ``unreveal`` before unpausing is allowed in order
    to prevent the demons from spawning. If you really want to unpause with hell
    revealed, specify ``demon`` instead of ``hell``.
``unreveal``
    Reverts the effects of ``reveal``.
``revtoggle``
    Switches between ``reveal`` and ``unreveal``. Convenient to bind to a
    hotkey.
``revforget``
    Discard info about what was visible before revealing the map. Only useful
    where (for example) you abandoned with the fort revealed and no longer need
    the saved map data when you load a new fort.
``revflood``
    Hide everything, then reveal tiles with a path to the cursor. This allows
    reparing maps that you accidentally saved while they were revealed. Note
    that tiles behind constructed walls are also revealed as a workaround for
    :bug:`1871`.
``nopause 1|0``
    Disables pausing (both manual and automatic) with the exception of the pause
    forced by `reveal` ``hell``. This is nice for digging under rivers. Use
    ``nopause 1`` to prevent pausing and ``nopause 0`` to allow pausing like
    normal.
