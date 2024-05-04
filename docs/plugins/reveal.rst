.. _revflood:

reveal
======

.. dfhack-tool::
    :summary: Reveal the map.
    :tags: adventure fort armok inspection map

.. dfhack-command:: unreveal
   :summary: Revert a revealed map to its unrevealed state.

.. dfhack-command:: revforget
   :summary: Discard records about what was visible before revealing the map.

.. dfhack-command:: revtoggle
   :summary: Switch between reveal and unreveal.

.. dfhack-command:: revflood
   :summary: Hide everything, then reveal tiles with a path to a unit.

This reveals all z-layers in fort and adventure mode. The effect persists until
you run ``unreveal``.

In graphics mode, solid tiles that are not adjacent to open space will not be
rendered, but they can still be examined by hovering over them with the mouse.
Switching to ASCII mode (in the game settings) will allow the display of the
revealed solid tiles.

For a GUI that auto-restores the map when closed, see `gui/reveal`.

Usage
-----

``reveal [hell|demon]``
    Reveal the whole map. If ``hell`` is specified, also reveal HFS areas, but
    you are required to run ``unreveal`` before unpausing is allowed in order
    to prevent the demons (or treasures) from spawning. If you really want to
    unpause with secrets revealed, specify ``demon`` instead of ``hell``. Note
    that unpausing with secrets revealed may result in a flood of announcements
    about the revealed secrets!
``unreveal``
    Reverts the effects of ``reveal`` if run immediately afterwards. If you
    have saved the fort in a revealed state and want to restore the map, use
    ``revflood``.
``revtoggle``
    Switches between ``reveal`` and ``unreveal``. Convenient to bind to a
    hotkey.
``revforget``
    Discard info about what was visible before revealing the map. Only useful
    where (for example) you abandoned with the fort revealed and no longer need
    the saved map data when you load a new fort.
``revflood``
    Hide everything, then reveal tiles with a path to the keyboard cursor (if
    enabled) or the selected unit (if a unit is selected) or else a random
    citizen. This allows reparing maps that you accidentally saved while they
    were revealed. Note that tiles behind constructed walls are also revealed
    as a workaround for :bug:`1871`.

Caveats
-------

Sometimes, the map generates secret hollows adjacent to caverns in such a way
that the ceiling of the hollow collapses on the first tick of the embark,
leaving the hollow exposed to the caverns. In this case, the secret event will
be triggered as soon as the cavern is discovered and that tile is unhidden.
This would happen anyway even if you don't use `reveal`, but be aware that it
is possible to trigger *some* events when you run `reveal`, even without the
``hell`` option.

When running `unreveal` to restore the map in adventure mode, the vision cone
for the adventurer isn't fully restored until the adventurer takes a step.
