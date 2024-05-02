autonestbox
===========

.. dfhack-tool::
    :summary: Auto-assign egg-laying female pets to nestbox zones.
    :tags: fort auto animals

To use this feature, you must create pen/pasture zones on the same tiles as
built nestboxes. If the pen is bigger than 1x1, the nestbox must be in the top
left corner. Only 1 unit will be assigned per pen, regardless of the size. Egg
layers who are also grazers will be ignored, since confining them to a 1x1
pasture is not a good idea. Only tame and domesticated own units are processed
since pasturing half-trained wild egg layers could destroy your neat nestbox
zones when they revert to wild.

Note that the age of the units is not checked, so you might get some egg-laying
kids assigned to the nestbox zones. Most birds grow up quite fast, though, so
they should be adults and laying eggs soon enough.

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
