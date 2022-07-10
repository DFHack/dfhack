autonestbox
===========
Assigns unpastured female egg-layers to nestbox zones. Requires that you create
pen/pasture zones above nestboxes. If the pen is bigger than 1x1 the nestbox
must be in the top left corner. Only 1 unit will be assigned per pen, regardless
of the size. The age of the units is currently not checked, most birds grow up
quite fast. Egglayers who are also grazers will be ignored, since confining them
to a 1x1 pasture is not a good idea. Only tame and domesticated own units are
processed since pasturing half-trained wild egglayers could destroy your neat
nestbox zones when they revert to wild. When called without options autonestbox
will instantly run once.

Options:

:start:        Start running every X frames (df simulation ticks).
               Default: X=6000, which would be every 60 seconds at 100fps.
:stop:         Stop running automatically.
:sleep:        Must be followed by number X. Changes the timer to sleep X
               frames between runs.
