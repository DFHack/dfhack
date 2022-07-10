diggingInvaders
===============
Makes invaders dig or destroy constructions to get to your dwarves.

To enable/disable the pluging, use: ``diggingInvaders (1|enable)|(0|disable)``

Basic usage:

:add GOBLIN:        registers the race GOBLIN as a digging invader. Case-sensitive.
:remove GOBLIN:     unregisters the race GOBLIN as a digging invader. Case-sensitive.
:now:               makes invaders try to dig now, if plugin is enabled
:clear:             clears all digging invader races
:edgesPerTick n:    makes the pathfinding algorithm work on at most n edges per tick.
                    Set to 0 or lower to make it unlimited.

You can also use ``diggingInvaders setCost (race) (action) n`` to set the
pathing cost of particular action, or ``setDelay`` to set how long it takes.
Costs and delays are per-tile, and the table shows default values.

============================== ======= ====== =================================
Action                         Cost    Delay  Notes
============================== ======= ====== =================================
``walk``                       1       0      base cost in the path algorithm
``destroyBuilding``            2       1,000  delay adds to the job_completion_timer of destroy building jobs that are assigned to invaders
``dig``                        10,000  1,000  digging soil or natural stone
``destroyRoughConstruction``   1,000   1,000  constructions made from boulders
``destroySmoothConstruction``  100     100    constructions made from blocks or bars
============================== ======= ====== =================================
