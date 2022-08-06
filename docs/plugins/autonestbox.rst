autonestbox
===========
**Tags:** `tag/fort`, `tag/auto`, `tag/animals`
:dfhack-keybind:`autonestbox`

Auto-assign egg-laying female pets to nestbox zones. Requires that you create
pen/pasture zones above nestboxes. If the pen is bigger than 1x1, the nestbox
must be in the top left corner. Only 1 unit will be assigned per pen, regardless
of the size. The age of the units is currently not checked since most birds grow
up quite fast. Egg layers who are also grazers will be ignored, since confining
them to a 1x1 pasture is not a good idea. Only tame and domesticated own units
are processed since pasturing half-trained wild egg layers could destroy your
neat nestbox zones when they revert to wild.

Usage:

``enable autonestbox``
    Start checking for unpastured egg-layers and assigning them to nestbox
    zones.
``autonestbox``
    Print current status.
``autonestbox now``
    Run a scan and assignment cycle right now. Does not require that the plugin
    is enabled.
``autonestbox ticks <ticks>``
    Change the number of ticks between scan and assignment cycles when the
    plugin is enabled. The default is 6000 (about 8 days).
