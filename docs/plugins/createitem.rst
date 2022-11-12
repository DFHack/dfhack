createitem
==========

.. dfhack-tool::
    :summary: Create arbitrary items.
    :tags: adventure fort armok items

You can create new items of any type and made of any material. A unit must be
selected in-game to use this command. By default, items created are spawned at
the feet of the selected unit.

Specify the item and material information as you would indicate them in custom
reaction raws, with the following differences:

* Separate the item and material with a space rather than a colon
* If the item has no subtype, the ``:NONE`` can be omitted
* If the item is ``REMAINS``, ``FISH``, ``FISH_RAW``, ``VERMIN``, ``PET``, or
  ``EGG``, then specify a ``CREATURE:CASTE`` pair instead of a material token.
* If the item is a ``PLANT_GROWTH``, specify a ``PLANT_ID:GROWTH_ID`` pair
  instead of a material token.

Corpses, body parts, and prepared meals cannot be created using this tool.

Usage
-----

``createitem <item> <material> [<count>]``
    Create <count> copies (default is 1) of the specified item made out of the
    specified material.
``createitem inspect``
    Obtain the item and material tokens of an existing item. Its output can be
    used directly as arguments to ``createitem`` to create new matching items
    (as long as the item type is supported).
``createitem floor|item|building``
    Subsequently created items will be placed on the floor beneath the selected
    unit's, inside the selected item, or as part of the selected building.

.. note::

    ``createitem building`` is good for loading traps, but if you use it with
    workshops, you will have to deconstruct the workshop to access the item.

Examples
--------

``createitem GLOVES:ITEM_GLOVES_GAUNTLETS INORGANIC:STEEL 2``
    Create 2 pairs of steel gauntlets (that is, 2 left gauntlets and 2 right
    gauntlets).
``createitem WOOD PLANT_MAT:TOWER_CAP:WOOD 100``
    Create 100 tower-cap logs.
``createitem PLANT_GROWTH BILBERRY:FRUIT``
    Create a single bilberry.

For more examples, :wiki:`the wiki <Utility:DFHack/createitem>`.
