.. _revflood:

reveal
======

.. dfhack-tool::
    :summary: Reveals the map.
    :tags: adventure fort armok inspection map

.. dfhack-command:: unreveal
   :summary: Hides previously hidden tiles again.

.. dfhack-command:: revforget
   :summary: Discard records about what was visible before revealing the map.

.. dfhack-command:: revtoggle
   :summary: Switch between reveal and unreveal.

.. dfhack-command:: revflood
   :summary: Hide everything, then reveal tiles with a path to a unit.

.. dfhack-command:: nopause
   :summary: Disable pausing.

This reveals all z-layers in fort mode. It also works in adventure mode, but any
of its effects are negated once you move. When you use it this way, you don't
need to run ``unreveal`` to hide the map again.

Usage
-----

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
    Hide everything, then reveal tiles with a path to the keyboard cursor (if
    enabled) or the selected unit (if a unit is selected) or else a random citizen.
    This allows reparing maps that you accidentally saved while they were revealed.
    Note that tiles behind constructed walls are also revealed as a workaround for
    :bug:`1871`.
``nopause 1|0``
    Disables pausing (both manual and automatic) with the exception of the pause
    forced by `reveal` ``hell``. This is nice for digging under rivers. Use
    ``nopause 1`` to prevent pausing and ``nopause 0`` to allow pausing like
    normal.
