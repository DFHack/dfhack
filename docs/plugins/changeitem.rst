changeitem
==========

.. dfhack-tool::
    :summary: Change item material or base quality.
    :tags: untested adventure fort armok items

By default, a change is only allowed if the existing and desired item materials
are of the same subtype (for example wood -> wood, stone -> stone, etc). But
since some transformations work pretty well and may be desired you can override
this with ``force``. Note that forced changes can possibly result in items that
crafters and haulers refuse to touch.

Usage
-----

``changeitem info``
   Show details about the selected item. Does not change the item. You can use
   this command to discover RAW ids for existing items.
``changeitem [<options>]``
   Change the item selected in the ``k`` list or inside a container/inventory.
``changeitem here [<options>]``
   Change all items at the cursor position. Requires in-game cursor.

Examples
--------

``changeitem here m INORGANIC:GRANITE``
   Change material of all stone items under the cursor to granite.
``changeitem q 5``
   Change currently selected item to masterpiece quality.

Options
-------

``m``, ``material <RAW id>``
   Change material. Must be followed by valid material RAW id.
``s``, ``subtype <RAW id>``
   Change subtype. Must be followed by a valid subtype RAW id."
``q``, ``quality <quality>``
   Change base quality. Must be followed by number (0-5) with 0 being no quality
   and 5 being masterpiece quality.
``force``
   Ignore subtypes and force the change to the new material.
