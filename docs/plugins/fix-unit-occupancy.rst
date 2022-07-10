fix-unit-occupancy
==================
This plugin fixes issues with unit occupancy, notably phantom
"unit blocking tile" messages (:bug:`3499`). It can be run manually, or
periodically when enabled with the built-in enable/disable commands:

:(no argument):         Run the plugin once immediately, for the whole map.
:-h, here, cursor:      Run immediately, only operate on the tile at the cursor
:-n, dry, dry-run:      Run immediately, do not write changes to map
:interval <X>:          Run the plugin every ``X`` ticks (when enabled).
                        The default is 1200 ticks, or 1 day.
                        Ticks are only counted when the game is unpaused.
