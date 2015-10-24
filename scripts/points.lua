-- by Meph
-- http://www.bay12forums.com/smf/index.php?topic=135506.msg4925005#msg4925005
--[[=begin

points
======
Sets available points at the embark screen to the specified number.  Eg.
``points 1000000`` would allow you to buy everything, or ``points 0`` would
make life quite difficult.

=end]]

df.global.world.worldgen.worldgen_parms.embark_points=tonumber(...)
