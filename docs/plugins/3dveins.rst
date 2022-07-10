3dveins
=======
Removes all existing veins from the map and generates new ones using
3D Perlin noise, in order to produce a layout that smoothly flows between
Z levels. The vein distribution is based on the world seed, so running
the command for the second time should produce no change. It is best to
run it just once immediately after embark.

This command is intended as only a cosmetic change, so it takes
care to exactly preserve the mineral counts reported by `prospect` ``all``.
The amounts of different layer stones may slightly change in some cases
if vein mass shifts between Z layers.

The only undo option is to restore your save from backup.
