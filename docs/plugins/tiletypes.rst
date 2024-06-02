.. _tiletypes-here:
.. _tiletypes-here-point:

tiletypes
=========

.. dfhack-tool::
    :summary: Paints tiles of specified types onto the map.
    :tags: adventure fort armok map

.. dfhack-command:: tiletypes-command
   :summary: Run tiletypes commands.

.. dfhack-command:: tiletypes-here
   :summary: Paint map tiles starting from the cursor.

.. dfhack-command:: tiletypes-here-point
   :summary: Paint the map tile under the cursor.

You can use the `probe` command to discover properties of existing tiles that
you'd like to copy. If you accidentally paint over a vein that you want back,
`fixveins` may help.

See `gui/tiletypes` for an in-game interface for this functionality.

The tool works with a brush, a filter, and a paint specification. The brush
determines the shape of the area to affect, the filter selects which tiles to
affect, and the paint specification determines how to affect those tiles.

Both paint and filter can have many different properties, like general shape
(WALL, FLOOR, etc.), general material (SOIL, STONE, MINERAL, etc.), specific
materials (MICROCLINE, MARBLE, etc.), state of 'designated', 'hidden', and
'light' flags, and many others.

Usage
-----

``tiletypes``
    Start the interactive terminal prompt where you can iteratively modify
    the brush, filter, and paint specification and get help on syntax
    elements. When in the interactive prompt, type ``quit`` to get out.
``tiletypes-command <command> [; <command> ...]``
    Run ``tiletypes`` commands from outside the interactive prompt. You can
    use this form from hotkeys or `dfhack-run` to set specific tiletypes
    properties. You can run multiple commands on one line by separating them
    with :literal:`\  ; \ ` -- that's a semicolon with a space on either side.
    See the Commands_ section below for an overview of commands you can run.
``tiletypes-here [<options>]``
    Apply the current options set in ``tiletypes`` and/or ``tiletypes-command``
    at the in-game cursor position, including the brush.  Can be used from a
    hotkey.
``tiletypes-here-point [<options>]``
    Apply the current options set in ``tiletypes`` and/or ``tiletypes-command``
    at the in-game cursor position to a single tile (ignoring brush settings).
    Can be used from a hotkey.

Examples
--------

``tiletypes-command filter material STONE ; f shape WALL ; paint shape FLOOR``
    Turn all stone walls into floors, preserving the material.
``tiletypes-command p any ; p s wall ; p sp normal``
    Clear the paint specification and set it to unsmoothed walls.
``tiletypes-command f any ; p stone marble ; p sh wall ; p sp normal ; r 10 10``
    Prepare to paint a 10x10 area of marble walls, ready for harvesting for
    flux.
``tiletypes-command f any ; f designated 1 ; p any ; p hidden 0 ; block ; run``
    Set the filter to match designated tiles, the paint specification to unhide
    them, and the brush to cover all tiles in the current block. Then run itThis is useful
    for unhiding tiles you wish to dig out of an aquifer so the game doesn't
    pause and undesignate adjacent tiles every time a new damp tile is
    "discovered".

Options
-------

``-c``, ``--cursor <x>,<y>,<z>``
    Use the specified map coordinates instead of the current cursor position. If
    this option is specified, then an active game map cursor is not necessary.
``-q``, ``--quiet``
    Suppress non-error status output.

Commands
--------

Commands can set the brush or modify the filter or paint options. When at the
interactive ``tiletypes>`` prompt, the command ``run`` (or hitting enter on an
empty line) will apply the current filter and paint specification with the
current brush at the current cursor position. The command ``quit`` will exit.

Brush commands
``````````````

``p``, ``point``
    Use the point brush.
``r``, ``range <width> <height> [<depth>]``
    Use the range brush with the specified width, height, and depth. If not
    specified, depth is 1, meaning just the current z-level. The range starts at
    the position of the cursor and goes to the east, south and up (towards the
    sky).
``block``
    Use the block brush, which includes all tiles in the 16x16 block that
    includes the cursor.
``column``
    Use the column brush, which ranges from the current cursor position to the
    first solid tile above it. This is useful for filling the empty space in a
    cavern.

Filter and paint commands
`````````````````````````

The general forms for modifying the filter or paint specification are:

``f``, ``filter <options>``
    Modify the filter.
``p``, ``paint <options>``
    Modify the paint specification.

The options identify the property of the tile and the value of that property:

``any``
    Reset to default (no filter/paint).
``s``, ``sh``, ``shape <shape>``
    Tile shape information. Run ``:lua @df.tiletype_shape`` to see valid shapes,
    or use a shape of ``any`` to clear the current setting.
``m``, ``mat``, ``material <material>``
    Tile material information. Run ``:lua @df.tiletype_material`` to see valid
    materials, or use a material of ``any`` to clear the current setting.
``sp``, ``special <special>``
    Tile special information. Run ``:lua @df.tiletype_special`` to see valid
    special values, or use a special value of ``any`` to clear the current
    setting.
``v``, ``var``, ``variant <variant>``
    Tile variant information. Run ``:lua @df.tiletype_variant`` to see valid
    variant values, or use a variant value of ``any`` to clear the current
    setting.
``a``, ``all [<shape>] [<material>] [<special>] [<variant>]``
    Set values for any or all of shape, material, special, and/or variant, in
    any order.
``d``, ``designated 0|1``
    Only useful for the filter, since you can't "paint" designations.
``h``, ``hidden 0|1``
    Whether a tile is hidden. A value of ``0`` means "revealed".
``l``, ``light 0|1``
    Whether a tile is marked as "Light". A value of ``0`` means "dark".
``st``, ``subterranean 0|1``
    Whether a tile is marked as "Subterranean".
``sv``, ``skyview 0|1``
    Whether a tile is marked as "Outside". A value of ``0`` means "inside".
``aqua``, ``aquifer 0|1|2``
    Whether a tile is marked as an aquifer. A value of ``2`` means "heavy aquifer".
``stone <stone type>``
    Set a particular type of stone, creating veins as required. To see a list of
    valid stone types, run: ``:lua for _,mat in ipairs(df.global.world.raws.inorganics) do if mat.material.flags.IS_STONE and not mat.material.flags.NO_STONE_STOCKPILE then print(mat.id) end end``
    Note that this command paints under ice and constructions, instead of
    overwriting them. Also note that specifying a specific ``stone`` will cancel
    out anything you have specified for ``material``, and vice-versa.
``veintype <vein type>``
    Set a particular vein type for the ``stone`` option to take advantage of the
    different boulder drop rates. To see valid vein types, run
    ``:lua @df.inclusion_type``, or use vein type ``CLUSTER`` to reset to the
    default.
