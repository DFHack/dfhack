digFlood
========
Automatically digs out specified veins as they are discovered. It runs once
every time a dwarf finishes a dig job. It will only dig out appropriate tiles
that are adjacent to the finished dig job. To add a vein type, use ``digFlood 1 [type]``.
This will also enable the plugin. To remove a vein type, use ``digFlood 0 [type] 1``
to disable, then remove, then re-enable.

Usage:

:help digflood:     detailed help message
:digFlood 0:        disable the plugin
:digFlood 1:        enable the plugin
:digFlood 0 MICROCLINE COAL_BITUMINOUS 1:
                    disable plugin, remove microcline and bituminous coal from monitoring, then re-enable the plugin
:digFlood CLEAR:    remove all inorganics from monitoring
:digFlood digAll1:  ignore the monitor list and dig any vein
:digFlood digAll0:  disable digAll mode
