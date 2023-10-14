digFlood
========

.. dfhack-tool::
    :summary: Digs out veins as they are discovered.
    :tags: unavailable

Once you register specific vein types, this tool will automatically designate
tiles of those types of veins for digging as your miners complete adjacent
mining jobs. Note that it will *only* dig out tiles that are adjacent to a
just-finished dig job, so if you want to autodig a vein that has already been
discovered, you may need to manually designate one tile of the tile for digging
to get started.

Usage
-----

``enable digflood``
    Enable the plugin.
``digflood 1 <vein type> [<vein type> ...]``
    Start monitoring for the specified vein types.
``digFlood 0 <vein type> [<vein type> ...] 1``
    Stop monitoring for the specified vein types. Note the required ``1`` at the
    end.
``digFlood CLEAR``
    Remove all inorganics from monitoring.
``digFlood digAll1``
    Ignore the monitor list and dig any vein.
``digFlood digAll0``
    Disable digAll mode.

You can get the list of valid vein types with this command::

    lua "for i,mat in ipairs(df.global.world.raws.inorganics) do if mat.material.flags.IS_STONE and not mat.material.flags.NO_STONE_STOCKPILE then print(i, mat.id) end end"

Examples
--------

``digFlood 1 MICROCLINE COAL_BITUMINOUS``
    Automatically dig microcline and bituminous coal veins.
``digFlood 0 MICROCLINE 1``
    Stop automatically digging microcline.
