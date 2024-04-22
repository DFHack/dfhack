cursecheck
==========

.. dfhack-tool::
    :summary: Check for cursed creatures.
    :tags: fort armok inspection units

This command checks a single unit or the whole map for curses (ghosts, vampires,
necromancers, werebeasts, zombies, etc.).

If a unit is selected, only the selected unit will be checked. Otherwise, all
units on the map will be checked.

By default, you will just see the count of cursed creatures in case you just
want to find out if you have any of them running around in your fort. Dead and
passive creatures (ghosts who were put to rest, killed vampires, etc.) are
ignored. Undead skeletons, corpses, bodyparts and the like are all thrown into
the curse category "zombie". Anonymous zombies and resurrected body parts will
show as "unnamed creature".

Usage
-----

::

   cursecheck [<options>]

Examples
--------

- ``cursecheck``
   Display a count of cursed creatures on the map (or under the cursor).
- ``cursecheck detail all``
   Give detailed info about all cursed creatures including deceased ones.
- ``cursecheck nick``
   Give a nickname to all living/active cursed creatures.

.. note::

    If you do a full search (with the option "all") former ghosts will show up
    with the cursetype "unknown" because their ghostly flag is not set.

    If you see any living/active creatures with a cursetype of "unknown", then
    it is most likely a new type of curse introduced by a mod.

Options
-------

``detail``
   Print full name, date of birth, date of curse, and some status info (some
   vampires might use fake identities in-game, though).
``nick``
   Set the type of curse as nickname (does not always show up in-game; some
   vamps don't like nicknames).
``ids``
   Print the creature and race IDs.
``all``
   Include dead and passive cursed creatures (this can result in quite a long
   list after having !!FUN!! with necromancers).
``verbose``
   Print all curse tags (if you really want to know it all).
