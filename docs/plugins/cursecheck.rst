cursecheck
==========
Checks a single map tile or the whole map/world for cursed creatures (ghosts,
vampires, necromancers, werebeasts, zombies).

With an active in-game cursor only the selected tile will be observed.
Without a cursor the whole map will be checked.

By default cursed creatures will be only counted in case you just want to find
out if you have any of them running around in your fort. Dead and passive
creatures (ghosts who were put to rest, killed vampires, ...) are ignored.
Undead skeletons, corpses, bodyparts and the like are all thrown into the curse
category "zombie". Anonymous zombies and resurrected body parts will show
as "unnamed creature".

Options:

:detail:      Print full name, date of birth, date of curse and some status
              info (some vampires might use fake identities in-game, though).
:ids:         Print the creature and race IDs.
:nick:        Set the type of curse as nickname (does not always show up
              in-game, some vamps don't like nicknames).
:all:         Include dead and passive cursed creatures (can result in a quite
              long list after having FUN with necromancers).
:verbose:     Print all curse tags (if you really want to know it all).

Examples:

``cursecheck detail all``
   Give detailed info about all cursed creatures including deceased ones (no
   in-game cursor).
``cursecheck nick``
   Give a nickname all living/active cursed creatures on the map(no in-game
   cursor).

.. note::

    If you do a full search (with the option "all") former ghosts will show up
    with the cursetype "unknown" because their ghostly flag is not set.

    Please report any living/active creatures with cursetype "unknown" -
    this is most likely with mods which introduce new types of curses.
