changeitem
==========
Allows changing item material and base quality. By default the item currently
selected in the UI will be changed (you can select items in the 'k' list
or inside containers/inventory). By default change is only allowed if materials
is of the same subtype (for example wood<->wood, stone<->stone etc). But since
some transformations work pretty well and may be desired you can override this
with 'force'. Note that some attributes will not be touched, possibly resulting
in weirdness. To get an idea how the RAW id should look like, check some items
with 'info'. Using 'force' might create items which are not touched by
crafters/haulers.

Options:

:info:         Don't change anything, print some info instead.
:here:         Change all items at the cursor position. Requires in-game cursor.
:material, m:  Change material. Must be followed by valid material RAW id.
:quality, q:   Change base quality. Must be followed by number (0-5).
:force:        Ignore subtypes, force change to new material.

Examples:

``changeitem m INORGANIC:GRANITE here``
   Change material of all items under the cursor to granite.
``changeitem q 5``
   Change currently selected item to masterpiece quality.
