createitem
==========
Allows creating new items of arbitrary types and made of arbitrary materials. A
unit must be selected in-game to use this command. By default, items created are
spawned at the feet of the selected unit.

Specify the item and material information as you would indicate them in
custom reaction raws, with the following differences:

* Separate the item and material with a space rather than a colon
* If the item has no subtype, the ``:NONE`` can be omitted
* If the item is ``REMAINS``, ``FISH``, ``FISH_RAW``, ``VERMIN``, ``PET``, or ``EGG``,
  specify a ``CREATURE:CASTE`` pair instead of a material token.
* If the item is a ``PLANT_GROWTH``, specify a ``PLANT_ID:GROWTH_ID`` pair
  instead of a material token.

Corpses, body parts, and prepared meals cannot be created using this tool.

To obtain the item and material tokens of an existing item, run
``createitem inspect``. Its output can be passed directly as arguments to
``createitem`` to create new matching items, as long as the item type is
supported.

Examples:

* Create 2 pairs of steel gauntlets::

    createitem GLOVES:ITEM_GLOVES_GAUNTLETS INORGANIC:STEEL 2

* Create tower-cap logs::

    createitem WOOD PLANT_MAT:TOWER_CAP:WOOD

* Create bilberries::

    createitem PLANT_GROWTH BILBERRY:FRUIT

For more examples, :wiki:`see this wiki page <Utility:DFHack/createitem>`.

To change where new items are placed, first run the command with a
destination type while an appropriate destination is selected.

Options:

:floor:     Subsequent items will be placed on the floor beneath the selected unit's feet.
:item:      Subsequent items will be stored inside the currently selected item.
:building:  Subsequent items will become part of the currently selected building.
            Good for loading traps; do not use with workshops (or deconstruct to use the item).
