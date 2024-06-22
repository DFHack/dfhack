autonestbox
===========

.. dfhack-tool::
    :summary: Auto-assign egg-laying adult female pets to nestbox zones.
    :tags: fort auto animals

To use this feature, you must create pen/pasture zones on the same tiles as
built nestboxes. If the pen is bigger than 1x1, the nestbox must be in the top
left corner. Only 1 unit will be assigned per pen, regardless of the size. Egg
layers who are also grazers (like elk birds) will be ignored, since confining
them to a 1x1 pasture will starve them. Only domesticated units or tamed units
with actively assigned trainers are pastured since half-trained wild egg layers
could destroy your neat nestbox zones when they revert to wild.

Usage
-----

``enable autonestbox``
    Start checking for unpastured egg-layers and assigning them to nestbox
    zones.
``autonestbox``
    Print current status.
``autonestbox now``
    Run a scan and assignment cycle right now. Does not require that the plugin
    is enabled.
