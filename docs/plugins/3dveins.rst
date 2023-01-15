3dveins
=======

.. dfhack-tool::
    :summary: Rewrite layer veins to expand in 3D space.
    :tags: untested fort gameplay map

Existing, flat veins are removed and new 3D veins that naturally span z-levels
are generated in their place. The transformation preserves the mineral counts
reported by `prospect`.

Usage
-----

::

    3dveins [verbose]

The ``verbose`` option prints out extra information to the console.

Example
-------

::

    3dveins

New veins are generated using natural-looking 3D Perlin noise in order to
produce a layout that flows smoothly between z-levels. The vein distribution is
based on the world seed, so running the command for the second time should
produce no change. It is best to run it just once immediately after embark.

This command is intended as only a cosmetic change, so it takes care to exactly
preserve the mineral counts reported by ``prospect all``. The amounts of layer
stones may slightly change in some cases if vein mass shifts between z layers.

The only undo option is to restore your save from backup.
