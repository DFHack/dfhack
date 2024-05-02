lair
====

.. dfhack-tool::
    :summary: Prevent item scatter when a site is reclaimed or visited.
    :tags: fort armok map

This tools sets map properties to avoid item scatter when the fortress is
abandoned and reclaimed or visited in adventure mode.

It is called "lair" because monster lairs have similar map properties set, but
this tool does not mark the site as an actual monster lair. The representation
of the site on the world map won't change. Additionally, you can't restore the
state of real monster lairs using this tool.

Usage
-----

``lair``
    Mark the map as a monster lair to prevent item scatter.
``lair reset``
    Mark the map as ordinary (not lair).
