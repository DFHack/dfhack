.. _quickfort-blueprint-guide:
.. _quickfort-user-guide:

Quickfort Blueprint Editing Guide
=================================

`Quickfort <quickfort>` is a DFHack script that helps you build fortresses from
"blueprint" .csv and .xlsx files. Many applications exist to edit these files,
such as MS Excel and `Google Sheets <https://sheets.new>`__. Most layout and
building-oriented DF commands are supported through the use of multiple files or
spreadsheets, each describing a different phase of DF construction: designation,
building, placing stockpiles/zones, and setting configuration.

The original idea came from :wiki:`Valdemar's <User:Valdemar>` auto-designation
macro. Joel Thornton reimplemented the core logic in Python and extended its
functionality with `Quickfort 2.0 <https://github.com/joelpt/quickfort>`__. This
DFHack-native implementation, called "DFHack Quickfort" or just "quickfort",
builds upon Quickfort 2.0's formats and features. Any blueprint that worked in
Python Quickfort 2.0 should work with DFHack Quickfort. DFHack Quickfort
interacts with Dwarf Fortress memory structures directly, allowing for
instantaneous blueprint application, error checking and recovery, and many other
advanced features.

This guide focuses on DFHack Quickfort's capabilities and teaches players how
to understand and create blueprint files. Some of the text was originally
written by Joel Thornton, reused here with his permission.

For those just looking to apply existing blueprints, check out the `quickfort
command's documentation <quickfort>` for syntax. There are also many
ready-to-use blueprints available in the ``blueprints/library`` subfolder in
your DFHack installation. Browse them on your computer or
:source:`online <data/blueprints/library>`, or run ``quickfort list -l`` at the
``[DFHack]#`` prompt to list them, and then ``quickfort run`` to apply them to
your fort!

Before you become an expert at writing blueprints, though, you should know that
the easiest way to make a quickfort blueprint is to build your plan "for real"
in Dwarf Fortress and then export your map using the DFHack `blueprint` plugin.
You can apply those blueprints as-is in your next fort, or you can fine-tune
them with additional features from this guide.

See the `Links`_ section for more information and online resources.


.. contents:: Table of Contents
   :local:
   :depth: 2


Features
--------

-  General

   -  Manages blueprints to handle all phases of DF construction
   -  Supports .csv and multi-worksheet .xlsx blueprint files
   -  Near-instant application, even for very large and complex blueprints
   -  Blueprints can span multiple z-levels
   -  You can package all blueprints and keystroke aliases needed for an entire
      fortress in a single file for easy sharing
   -  "meta" blueprints that simplify the application of sequences of blueprints
   -  Undo functionality for dig, build, place, and zone blueprints
   -  Rotate blueprints or flip them around to your preference when you apply
      them to the map
   -  Automatic cropping of blueprints so you don't get errors if the blueprint
      extends off the map
   -  Can generate manager orders for everything required by a build blueprint
   -  Includes a library of ready-to-use blueprints
   -  Blueprint debugging features

-  Dig mode

   -  Supports all types of designations, including dumping/forbidding items and
      setting traffic settings
   -  Supports setting dig priorities
   -  Supports applying dig blueprints in marker mode
   -  Handles carving arbitrarily complex minecart tracks, including tracks that
      cross other tracks

-  Build mode

   -  Fully integrated with DFHack buildingplan: you can place buildings before
      manufacturing building materials and you can use the buildingplan UI for
      setting materials preferences
   -  Designate entire constructions in mid-air without having to wait for each
      tile to become supported
   -  Automatic expansion of building footprints to their minimum dimensions, so
      only the center tile of a multi-tile building needs to be recorded in the
      blueprint
   -  Tile occupancy and validity checking so, for example, buildings that
      cannot be placed on a target tile will be skipped instead of messing up
      the blueprint. Blueprints that are only partially applied for any reason
      (e.g. you need to dig out some more tiles) can be safely reapplied to
      build the remaining buildings.
   -  Relaxed rules for farm plot and road placement: you can still place the
      building even if an invalid tile (e.g. stone tiles for farm plots) splits
      the designated area into two disconnected parts
   -  Intelligent boundary detection for adjacent buildings of the same type
      (e.g. a 6x6 block of ``wj`` cells will be correctly split into 4 jeweler's
      workshops)

-  Place and zone modes

   -  Define stockpiles and zones of any shape, not just rectangles
   -  Configurable numbers of bins, barrels and wheelbarrows assigned to created
      stockpiles
   -  Automatic splitting of stockpiles and zones that exceed maximum dimension
      limits
   -  Fully configurable zone settings, such as pit/pond and hospital supply
      counts

-  Query mode

   -  Send arbitrary keystroke sequences to the UI -- *anything* you can do
      through the UI is supported
   -  Supports aliases to simplify frequent keystroke combos
   -  Includes a library of pre-made and tested aliases to simplify most common
      tasks, such as configuring stockpiles for important item types or creating
      hauling routes for quantum stockpiles.
   -  Supports expanding aliases in other aliases for easy management of common
      subsequences
   -  Supports repeating key sequences a specified number of times
   -  Skips sending keys when the cursor is over a tile that does not have a
      stockpile or building, so missing buildings won't desynchronize your
      blueprint
   -  Instant halting of query blueprint application when keystroke errors are
      detected, such as when a mistake in a key sequence leaves us stuck in a
      submenu, to make query blueprints easier to debug

Creating blueprints
-------------------

We recommend using a spreadsheet editor such as Excel, `Google
Sheets <https://sheets.new>`__, or `LibreOffice <https://www.libreoffice.org>`__
to edit blueprint files, but any text editor will do.

The format of Quickfort-compatible blueprint files is straightforward. The first
line (or upper-left cell) of the spreadsheet should look like this::

   #dig

The keyword ``dig`` tells Quickfort we are going to be using the Designations
menu in DF. The following "mode" keywords are understood:

==============  ===========
Blueprint mode  Description
==============  ===========
dig             Designations menu (:kbd:`d`)
build           Build menu (:kbd:`b`)
place           Place stockpiles menu (:kbd:`p`)
zone            Activity zones menu (:kbd:`i`)
query           Set building tasks/prefs menu (:kbd:`q`)
==============  ===========

If no modeline appears in the first cell, Quickfort assumes that it's looking at
a ``#dig`` blueprint.

There are also other modes that don't directly correspond to Dwarf Fortress
menus, but we'll talk about those `later <quickfort-other-modes>`.

If you like, you may enter a comment after the mode keyword. This comment will
appear in the output of ``quickfort list`` when run from the ``DFHack#`` prompt
or in the dialog window when running `gui/quickfort`. You can use this space for
explanations, attribution, etc.::

   #dig grand dining room

Below this line, begin entering keys in each spreadsheet cell that represent
what you want designated in the corresponding game map tile. For example, we
could dig out a 4x4 room like so (spaces are used as column separators here for
readability, but a real .csv file would have commas)::

   #dig
   d d d d #
   d d d d #
   d d d d #
   d d d d #
   # # # # #

Note the :kbd:`#` symbols at the right end of each row and below the last row.
These are completely optional, but can be helpful to make the row and column
positions clear.

Once the dwarves have that dug out, let's build a walled-in bedroom within our
dug-out area::

   #build
   Cw Cw Cw Cw #
   Cw b  h  Cw #
   Cw       Cw #
   Cw Cw    Cw #
   #  #  #  #  #

Note my generosity -- in addition to the bed (:kbd:`b`) I've built a chest
(:kbd:`c`) here for the dwarf as well. You must use the full series of keys
needed to build something in each cell, e.g. :kbd:`C`:kbd:`w` indicates we
should enter DF's constructions submenu (:kbd:`C`) and select walls (:kbd:`w`).

I'd also like to place a booze stockpile in the 2 unoccupied tiles in the room::

   #place Place a food stockpile
   ` ` ` ` #
   ` ~ ~ ` #
   ` f f ` #
   ` `   ` #
   # # # # #

This illustration may be a little hard to understand. The two :kbd:`f`
characters are in row 3, columns 2 and 3. All the other cells are empty. QF
considers both :kbd:`\`` (backtick -- the character under the tilde) and
:kbd:`~` (tilde) characters within cells to be empty cells; this can help with
multilayer or fortress-wide blueprint layouts as "chalk lines".

QF is smart enough to recognize this as a 2x1 food stockpile, and creates it as
such rather than as two 1x1 food stockpiles. Quickfort treats any connected
region of identical designations as a single entity. The tiles can be connected
orthogonally or diagonally, just as long as they are touching.

Lastly, let's turn the bed into a bedroom and set the food stockpile to hold
only booze.

::

   #query
   ` ` ` ` #
   ` r&  ` #
   ` booze #
   ` ` ` ` #
   # # # # #

In row 2, column 2 we have ``r&``. This sends the :kbd:`r` key to DF when the
cursor is over the bed, causing us to "make room" and :kbd:`Enter`, represented
by special ``&`` alias, to indicate that we're done setting the size (the
default room size is fine here).

In column 2, row 3 we have ``booze``. This is one of many alias keywords defined
in the included :source:`aliases library <data/quickfort/aliases-common.txt>`.
This particular alias sets a food stockpile to accept only booze. It sends the
keys needed to navigate DF's stockpile settings menu, and then it sends an
Escape character to exit back to the map. It is important to exit out of any
menus that you enter while in query mode so that the cursor can move to the next
tile when it is done with the current tile.

If there weren't an alias named ``booze`` then the literal characters
:kbd:`b`:kbd:`o`:kbd:`o`:kbd:`z`:kbd:`e` would have been sent, so be sure to
spell those aliases correctly!

You can save a lot of time and effort by using aliases instead of adding all
key seqences directly to your blueprints. For more details, check out the
`quickfort-alias-guide`. You can also see examples of aliases being used in the
query blueprints in the
:source:`DFHack blueprint library <data/blueprints/library>`. You can create
your own aliases by adding them to :source:`dfhack-config/quickfort/aliases.txt`
in your DFHack folder or you can package them
`together with your blueprint files <quickfort-aliases-blueprints>`.

Area expansion syntax
~~~~~~~~~~~~~~~~~~~~~

In Quickfort, the following blueprints are equivalent::

   #dig a 3x3 area
   d d d #
   d d d #
   d d d #
   # # # #

   #dig the same area with d(3x3) specified in row 1, col 1
   d(3x3)#
   ` ` ` #
   ` ` ` #
   # # # #

The second example uses Quickfort's "area expansion syntax", which takes the
form::

   keys(WxH)

Note that area expansion syntax can only specify rectangular areas. If you want
to create extent-based structures (e.g. farm plots or stockpiles) in different
shapes, use the first format above. For example::

   #place A single L shaped food stockpile
   f f ` ` #
   f f ` ` #
   f f f f #
   f f f f #
   # # # # #

Area expansion syntax also sets boundaries, which can be useful if you want
adjacent, but separate, stockpiles of the same type::

   #place Two touching but separate food stockpiles
   f(2x2)  #
   ~ ~ ` ` #
   f(4x2)  #
   ~ ~ ~ ~ #
   # # # # #

As mentioned previously, :kbd:`~` characters are ignored as comment characters
and can be used for visualizing the blueprint layout. This blueprint can be
equivalently written as::

   #place Two touching but separate food stockpiles
   f(2x2)  #
   ~ ~ ` ` #
   f f f f #
   f f f f #
   # # # # #

since the area expansion syntax of the upper stockpile prevents it from
combining with the lower, freeform syntax stockpile.

Area expansion syntax can also be used for buildings which have an adjustable
size, like bridges. The following blueprints are equivalent::

   #build a 4x2 bridge from row 1, col 1
   ga(4x2)  `  #
   `  `  `  `  #
   #  #  #  #  #

   #build a 4x2 bridge from row 1, col 1
   ga ga ga ga #
   ga ga ga ga #
   #  #  #  #  #

If it is convenient to do so, you can place the cell with the expansion syntax
in any corner of the resulting rectangle. Just use negative numbers to indicate
which direction the designation should expand in. For example, the previous
blueprint could also be written as::

   #build a 4x2 bridge from row 2, col 4
   `  `  `  `  #
   ga(4x-2) `  #
   #  #  #  #  #

Automatic area expansion
~~~~~~~~~~~~~~~~~~~~~~~~

Buildings larger than 1x1, like workshops, can be represented in any of three
ways. You can designate just their center tile with empty cells around it to
leave room for the footprint, like this::

   #build a mason workshop in row 2, col 2 that will occupy the 3x3 area
   ` `  ` #
   ` wm ` #
   ` `  ` #
   # #  # #

Or you can fill out the entire footprint like this::

   #build a mason workshop
   wm wm wm #
   wm wm wm #
   wm wm wm #
   #  #  #  #

This format may be verbose for regular workshops, but it can be very helpful for
laying out structures like screw pump towers and waterwheels, whose "center
point" can be non-obvious.

Or you can use area expansion syntax::

   #build a mason workshop
   wm(3x3)  #
   `  `  `  #
   `  `  `  #
   #  #  #  #

This style can be convenient for laying out multiple buildings of the same type.
If you are building a large-scale block factory, for example, this will create
20 mason workshops all in a row::

   #build line of 20 mason workshops
   wm(60x3)

Quickfort will intelligently break large areas of the same designation into
appropriately-sized chunks.

Multilevel blueprints
~~~~~~~~~~~~~~~~~~~~~

Multilevel blueprints are accommodated by separating Z-levels of the blueprint
with ``#>`` (go down one z-level) or ``#<`` (go up one z-level) at the end of
each floor.

::

   #dig Stairs leading down to a small room below
   j  `  `  #
   `  `  `  #
   `  `  `  #
   #> #  #  #
   u  d  d  #
   d  d  d  #
   d  d  d  #
   #  #  #  #

The marker must appear in the first column of the row to be recognized, just
like a modeline.

You can go up or down multiple levels by adding a number after the ``<`` or
``>``. For example::

    #dig Two double-level quarries
    r(10x10)
    #>2
    r(10x10)

.. _quickfort-dig-priorities:

Dig priorities
~~~~~~~~~~~~~~

DF designation priorities are supported for ``#dig`` blueprints. The full syntax
is ``[letter][number][expansion]``, where if the ``letter`` is not specified,
``d`` is assumed, and if ``number`` is not specified, ``4`` is assumed (the
default priority). So each of these blueprints is equivalent::

   #dig dig the interior of the room at high priority
   d  d  d  d  d  #
   d  d1 d1 d1 d  #
   d  d1 d1 d1 d  #
   d  d1 d1 d1 d  #
   d  d  d  d  d  #
   #  #  #  #  #  #

   #dig dig the interior of the room at high priority
   d  d  d  d  d  #
   d  d1(3x3)  d  #
   d  `  `  `  d  #
   d  `  `  `  d  #
   d  d  d  d  d  #
   #  #  #  #  #  #

   #dig dig the interior of the room at high priority
   4  4  4  4  4  #
   4  1  1  1  4  #
   4  1  1  1  4  #
   4  1  1  1  4  #
   4  4  4  4  4  #
   #  #  #  #  #  #

Marker mode
~~~~~~~~~~~

Marker mode is useful for when you want to plan out your digging, but you don't
want to dig everything just yet. In ``#dig`` mode, you can add a :kbd:`m` before
any other designation letter to indicate that the tile should be designated in
marker mode. For example, to dig out the perimeter of a room, but leave the
center of the room marked for digging later::

   #dig
   d  d  d  d d #
   d md md md d #
   d md md md d #
   d md md md d #
   d  d  d  d d #
   #  #  #  # # #

Then you can use "Toggle Standard/Marking" (:kbd:`d`:kbd:`M`) to convert the
center tiles to regular designations at your leisure.

To apply an entire dig blueprint in marker mode, regardless of what the
blueprint itself says, you can set the global quickfort setting
``force_marker_mode`` to ``true`` before you apply the blueprint.

Note that the in-game UI setting "Standard/Marker Only" (:kbd:`d`:kbd:`m`) does
not have any effect on quickfort.

Stockpiles and zones
~~~~~~~~~~~~~~~~~~~~

It is very common to have stockpiles that accept multiple categories of items or
zones that permit more than one activity. Although it is perfectly valid to
declare a single-purpose stockpile or zone and then modify it with a ``#query``
blueprint, quickfort also supports directly declaring all the types in the
``#place`` and ``#zone`` blueprints. For example, to declare a 20x10 stockpile
that accepts both corpses and refuse, you could write::

   #place refuse heap
   yr(20x10)

And similarly, to declare a zone that is a pasture, a fruit picking area, and a
meeting area all at once::

   #zone main pasture and picnic area
   nmg(10x10)

The order of the individual letters doesn't matter. If you want to configure the
stockpile from scratch in a ``#query`` blueprint, you can place unconfigured
"custom" stockpiles with (:kbd:`c`). It is more efficient, though, to place
stockpiles using the keys that represent the categories of items that you want
to store, and then only use a ``#query`` blueprint if you need fine-grained
customization.

.. _quickfort-place-containers:

Stockpile bins, barrels, and wheelbarrows
`````````````````````````````````````````

Quickfort has global settings for default values for the number of bins,
barrels, and wheelbarrows assigned to stockpiles, but these numbers can be set
for individual stockpiles as well.

To set the number of bins, barrels, or wheelbarrows, just add a number after the
letter that indicates what type of stockpile it is. For example::

    #place a stone stockpile with 5 wheelbarrows
    s5(3x3)

    #place a bar, ammo, weapon, and armor stockpile with 20 bins
    bzpd20(5x5)

If the specified number exceeds the number of available stockpile tiles, the
number of available tiles is used. For wheelbarrows, that limit is reduced by 1
to ensure there is at least one non-wheelbarrow tile available in the stockpile.
Otherwise no stone would ever be brought to the stockpile since all tiles would
be occupied by wheelbarrows!

Quickfort figures out which container type is being set by looking at the letter
that comes just before the number. For example ``zf10`` means 10 barrels in a
stockpile that accepts both ammo and food, whereas ``z10f`` means 10 bins. If
the stockpile category doesn't usually use any container type, like refuse or
corpses, wheelbarrows are assumed::

    #place a corpse stockpile with 3 wheelbarrows
    y3(3x3)

Note that if you are not using expansion syntax, each tile of the stockpile must
have the same text. Otherwise the stockpile boundaries will not be detected
properly::

    #place a non-rectangular animal stockpile with 5 wheelbarrows
    a5,a5,a5,a5
    a5,  ,  ,a5
    a5,  ,  ,a5
    a5,a5,a5,a5

Running ``quickfort orders`` on a ``#place`` blueprint with explicitly set
container/wheelbarrow counts will enqueue manager orders for the specified
number of containers or wheelbarrows, even if that number exceeds the in-game
size of the stockpile. For example, ``quickfort orders`` on the following
blueprint will enqueue 10 rock pots, even though the stockpile only has 9
tiles::

    #place
    f10(3x3)

Zone detailed configuration
```````````````````````````

Detailed configuration for zones, such as the pit/pond toggle, can also be set
by mimicking the hotkeys used to set them. Note that gather flags default to
true, so specifying them in a blueprint will turn the toggles off. If you need
to set configuration from multiple zone subscreens, separate the key sections
with :kbd:`^`. Note the special syntax for setting hospital supply levels, which
have no in-game hotkeys::

   #zone a combination hospital and shrub (but not fruit) gathering zone
   gGtf^hH{hospital buckets=5 splints=20}(10x10)

The valid hospital settings (and their maximum values) are::

    thread   (1500000)
    cloth    (1000000)
    splints  (100)
    crutches (100)
    plaster  (15000)
    buckets  (100)
    soap     (15000)

To toggle the ``active`` flag for zones, add an :kbd:`a` character to the
string. For example, to create a *disabled* pond zone (that you later intend to
carefully fill with 3-depth water for a dwarven bathtub)::

   #zone disabled pond zone
   apPf(1x3)

Minecart tracks
~~~~~~~~~~~~~~~

There are two ways to produce minecart tracks, and they are handled very
differently by the game. You can carve them into hard natural floors or you can
construct them out of building materials. Constructed tracks are conceptually
simpler, so we'll start with them.

Constructed tracks
``````````````````

Quickfort supports the designation of track stops and rollers in ``#build``
blueprints. You can build a track stop with :kbd:`C`:kbd:`S` and some number of
:kbd:`d` and :kbd:`a` characters for selecting dump direction and friction. You
can build a roller with :kbd:`M`:kbd:`r` and some number of :kbd:`s` and
:kbd:`q` characters for direction and speed. However, this can get confusing
very quickly and is very difficult to read in a blueprint. Moreover, constructed
track segments don't even have keys associated with them at all!

To solve this problem, Quickfort provides the following keywords for use in
build blueprints::

   -- Track segments --
   trackN
   trackS
   trackE
   trackW
   trackNS
   trackNE
   trackNW
   trackSE
   trackSW
   trackEW
   trackNSE
   trackNSW
   trackNEW
   trackSEW
   trackNSEW

   -- Track/ramp segments --
   trackrampN
   trackrampS
   trackrampE
   trackrampW
   trackrampNS
   trackrampNE
   trackrampNW
   trackrampSE
   trackrampSW
   trackrampEW
   trackrampNSE
   trackrampNSW
   trackrampNEW
   trackrampSEW
   trackrampNSEW

   -- Horizontal and vertical roller segments --
   rollerH
   rollerV
   rollerNS
   rollerSN
   rollerEW
   rollerWE

   Note: append up to four 'q' characters to roller keywords to set roller
   speed. E.g. a roller that propels from East to West at the slowest speed can
   be specified with 'rollerEWqqqq'.

   -- Track stops that (optionally) dump to the N/S/E/W --
   trackstop
   trackstopN
   trackstopS
   trackstopE
   trackstopW

   Note: append up to four 'a' characters to trackstop keywords to set friction
   amount. E.g. a stop that applies the smallest amount of friction can be
   specified with 'trackstopaaaa'.

As an example, you can create an E-W track with stops at each end that dump to
their outside directions with the following blueprint::

   #build Example track
   trackstopW trackEW trackEW trackEW trackstopE

Note that the **only** way to build track and track/ramp segments is with the
keywords. The UI method of using :kbd:`+` and :kbd:`-` keys to select the track
type from a list does not work since DFHack Quickfort doesn't actually send keys
to the UI to build buildings. The text in your spreadsheet cells is mapped
directly onto DFHack API calls. Only ``#query`` blueprints send actual keycodes
to the UI.

Carved tracks
`````````````

In the game, you carve a minecart track by specifying a beginning and ending
tile and the game "adds" the designation to the tiles in between. You cannot
designate single tiles because DF needs a multi-tile track to figure out which
direction the track should go on each tile. For example to carve two track
segments that cross each other, you might use the cursor to designate a line of
three vertical tiles like this::

   ` start here ` #
   ` `          ` #
   ` end here   ` #
   # #          # #

Then to carve the cross, you'd do a horizonal segment::

   `          ` `        #
   start here ` end here #
   `          ` `        #
   #          # #        #

This will result in a carved track that would be equivalent to a constructed
track of the form::

   #build
   `      trackS    `      #
   trackE trackNSEW trackW #
   `      trackN    `      #
   #      #         #      #

Quickfort supports both styles of specification for carving tracks with ``#dig``
blueprints. You can use the "additive" style to carve tracks in segments or you
can use the aliases to specify the track tile by tile. To designate track
segments, use area expansion syntax with a height or width of 1::

   #dig
   `      T(1x3) ` #
   T(3x1) `      ` #
   `      `      ` #
   #      #      # #

"But wait!", I can hear you say, "How do you designate a track corner that opens
to the South and East? You can't put both T(1xH) and T(Wx1) in the same cell!"
This is true, but you can specify both width and height greater than 1, and for
tracks, QF interprets it as an upper-left corner extending to the right W tiles
and down H tiles. For example, to carve a track in a closed ring, you'd write::

   #dig
   T(3x3) ` T(1x3) #
   `      ` `      #
   T(3x1) ` `      #
   #      # #      #

You can also use negative numbers in the expansion syntax to indicate corners
that are not upper-left corners. This blueprint will also carve a closed ring::

   #dig
   T(3x3) ` `        #
   `      ` `        #
   `      ` T(-3x-3) #
   #      # #        #

Or you could use the aliases to specify tile by tile::

   #dig
   trackSE trackEW trackSW #
   trackNS `       trackNS #
   trackNE trackEW trackNW #
   #       #       #       #

The aliases can also be used to designate a solid block of track. This is
epecially useful for obliterating low-quality engravings so you can re-smooth
and re-engrave with higher quality. For example, you could use the following
sequence of blueprints to ensure a 10x10 floor area contains only masterwork
engravings::

    #dig smooth floor
    s(10x10)
    #dig engrave floor
    e(10x10)
    #dig erase low-quality engravings
    trackNSEW(10x10)

The tracks only remove low-quality engravings since quickfort won't designate
masterwork engravings for destruction (unless forced to by a commandline
parameter). You would run (and let your dwarves complete the jobs for) the
sequence of blueprints until no tiles are designated by the "erase" blueprint.

.. _quickfort-modeline:

Modeline markers
~~~~~~~~~~~~~~~~

The modeline has some additional optional components that we haven't talked
about yet. You can:

-  give a blueprint a label by adding a ``label()`` marker
-  set a cursor offset and/or cursor placement hint by adding a ``start()``
   marker
-  hide a blueprint from being listed with a ``hidden()`` marker
-  register a message to be displayed after the blueprint is successfully
   applied with a ``message()`` marker

The full modeline syntax, when all optional elements are specified, is::

   #mode label(mylabel) start(X;Y;STARTCOMMENT) hidden() message(mymessage) comment

Note that all elements are optional except for the initial ``#mode`` (though, as
mentioned in the first section, if a modeline doesn't appear at all in the first
cell of a spreadsheet, the blueprint is interpreted as a ``#dig`` blueprint with
no optional markers). Here are a few examples of modelines with optional
elements before we discuss them in more detail::

   #dig start(3; 3; Center tile of a 5-tile square) Regular blueprint comment
   #build label(noblebedroom) start(10;15)
   #query label(configstockpiles) No explicit 'start()' means cursor is at upper left corner
   #meta label(digwholefort) start(center of stairs on surface)
   #dig label(digdining) hidden() called by the digwholefort meta blueprint
   #zone label(pastures) message(remember to assign animals to the new pastures)

.. _quickfort-label:

Blueprint labels
````````````````

Labels are displayed in the ``quickfort list`` output and are used for
addressing specific blueprints when there are multiple blueprints in a single
file or spreadsheet sheet (see `Packaging a set of blueprints`_ below). If a
blueprint has no label, the label becomes the ordinal of the blueprint's
position in the file or sheet. For example, the label of the first blueprint
will be "1" if it is not otherwise set, the label of the second blueprint will
be "2" if it is not otherwise set, etc. Labels that are explicitly defined must
start with a letter to ensure the auto-generated labels don't conflict with
user-defined labels.

.. _quickfort-start:

Start positions
```````````````

Start positions specify a cursor offset for a particular blueprint, simplifying
the task of blueprint alignment. This is very helpful for blueprints that are
based on a central staircase, but it comes in handy whenever a blueprint has an
obvious "center". For example::

   #build start(2;2;center of workshop) label(masonw) a mason workshop
   wm wm wm #
   wm wm wm #
   wm wm wm #
   #  #  #  #

will build the workshop *centered* on the cursor, not down and to the right of
the cursor.

The two numbers specify the column and row (or 1-based X and Y offset) where the
cursor is expected to be when you apply the blueprint. Position ``1;1`` is the
top left cell. The optional comment will show up in the ``quickfort list``
output and should contain information about where to position the cursor. If the
start position is ``1;1``, you can omit the numbers and just add a comment
describing where to put the cursor. This is also useful for meta blueprints that
don't actually care where the cursor is, but that refer to other blueprints that
have fully-specified ``start()`` markers. For example, a meta blueprint that
refers to the ``masonw`` blueprint above could look like this::

   #meta start(center of workshop) a mason workshop
   /masonw

You can use semicolons, commas, or spaces to separate the elements of the
``start()`` marker, whatever is most convenient.

.. _quickfort-hidden:

Hiding blueprints
`````````````````

A blueprint with a ``hidden()`` marker won't appear in ``quickfort list`` output
unless the ``--hidden`` flag is specified. The primary reason for hiding a
blueprint (rather than, say, deleting it or moving it out of the ``blueprints/``
folder) is if a blueprint is intended to be run as part of a larger sequence
managed by a `meta blueprint <quickfort-meta>`.

.. _quickfort-message:

Messages
````````

A blueprint with a ``message()`` marker will display a message after the
blueprint is applied with ``quickfort run``. This is useful for reminding
players to take manual steps that cannot be automated, like assigning minecarts
to a route, or listing the next step in a series of blueprints. For long or
multi-part messages, you can embed newlines::

   "#meta label(surface1) message(This would be a good time to start digging the industry level.
   Once the area is clear, continue with /surface2.) clear the embark site and set up pastures"

The quotes surrounding the cell text are only necessary if you are writing a
.csv file by hand. Spreadsheet applications will surround multi-line text with
quotes automatically when they save/export the file.

.. _quickfort-meta:

Meta blueprints
~~~~~~~~~~~~~~~

Meta blueprints are blueprints that control how other blueprints are applied.
For example, meta blueprints can bundle a group of other blueprints so that they
can be run with a single command. They can also encode logic, like rotating the
blueprint or duplicating it across a specified number of z-levels.

A common scenario where meta blueprints are useful is when you have several
phases to link together. For example you might:

1.  Apply a dig blueprint to designate dig areas
#.  Wait for miners to dig
#.  **Apply a build buildprint** to designate buildings
#.  **Apply a place buildprint** to designate stockpiles
#.  **Apply a query blueprint** to configure stockpiles
#.  Wait for buildings to get built
#.  Apply a different query blueprint to configure rooms

Those three "apply"s in the middle might as well get done in one command instead
of three. A ``#meta`` blueprint can help with that. A meta blueprint refers to
other blueprints in the same file by their label (see the `Modeline markers`_
section above) in the same format used by the `quickfort` command:
``<sheet name>/<label>``, or just ``/<label>`` for blueprints in .csv files or
blueprints in the same spreadsheet sheet as the ``#meta`` blueprint that
references them.

A few examples might make this clearer. Say you have a .csv file with blueprints
that prepare bedrooms for your dwarves::

   #dig label(bed1) dig out the rooms
   ...
   #build label(bed2) build the furniture
   ...
   #place label(bed3) add food stockpiles
   ...
   #query label(bed4) configure stockpiles
   ...
   #query label(bed5) set the built beds as rooms
   ...

Note how I've given them all labels so we can address them safely. If I hadn't
given them labels, they would receive default labels of "1", "2", "3", etc, but
those labels would change if I ever add more blueprints at the top. This is not
a problem if we're just running the blueprints individually from the
``quickfort list`` command, but meta blueprints need a label name that isn't
going to change over time.

So let's add a meta blueprint to this file that will combine the middle three
blueprints into one::

   "#meta label(bed234) combines build, place, and stockpile config blueprints"
   /bed2
   /bed3
   /bed4

Now your sequence is shortened to:

1.  Run /bed1 to designate dig areas
#.  Wait for miners to dig
#.  **Run /bed234 meta buildprint** to build buildings and designate/configure
    stockpiles
#.  Wait for buildings to get built
#.  Run /bed5 to configure the rooms as bedrooms

You can use meta blueprints to lay out your fortress at a larger scale as well.
The ``#<`` and ``#>`` notation is valid in meta blueprints, so you can, for
example, store the dig blueprints for all the levels of your fortress in
different sheets in a spreadsheet, and then use a meta blueprint to designate
your entire fortress for digging at once. For example, say you have a .xlsx
spreadsheet with the following layout:

=============  ========
Sheet name     Contents
=============  ========
dig_farming    one #dig blueprint, no label
dig_industry   one #dig blueprint, no label
dig_dining     four #dig blueprints, with labels "main", "basement",
               "waterway", and "cistern"
dig_guildhall  one #dig blueprint, no label
dig_suites     one #dig blueprint, no label
dig_bedrooms   one #dig blueprint, no label
=============  ========

We can add a sheet named "dig_all" with the following contents (we're expecting
a big fort, so we're digging 5 levels of bedrooms)::

   #meta dig the whole fortress
   dig_farming/1
   #>
   dig_industry/1
   #>
   dig_dining/main
   #>
   dig_dining/basement
   #>
   dig_dining/waterway
   #>
   dig_dining/cistern
   #>
   dig_guildhall/1
   #>
   dig_suites/1
   #>
   dig_bedrooms/1 repeat(down 5)

Note that for blueprints without an explicit label, we still need to address
them by their auto-generated numeric label.

The command to run the meta blueprint above would be::

    quickfort run myfort.xlsx -n dig_all

It's worth repeating that ``#meta`` blueprints can only refer to blueprints that
are defined in the same file. This means that all blueprints that a meta
blueprint needs to run must be in sheets within the same .xlsx spreadsheet or
concatenated into the same .csv file.

You can then hide the blueprints that you now manage with the meta blueprint
from ``quickfort list`` by adding a ``hidden()`` marker to their modelines. That
way the output of ``quickfort list`` won't be cluttered by blueprints that you
don't need to run directly. If you ever *do* need to access the meta-managed
blueprints individually, you can still see them with
``quickfort list --hidden``.

Meta markers
````````````

In meta blueprints, you can tag referenced blueprints with markers to modify how
they are applied. These markers are similar to `Modeline markers`_, but are only
usable in meta blueprints. Here's a quick list of examples, with more details
below:

===================  ===========
Example              Description
===================  ===========
repeat(down 10)      Repeats a blueprint down z-levels 10 times
shift(0 10)          Adds 10 to the y coordinate of each blueprint tile
transform(cw flipv)  Rotates a blueprint clockwise and then flips it vertically
===================  ===========

**Repeating blueprints**

Syntax: repeat(<direction>[, ]<number>)

The direction can be ``up`` or ``down``, and the repetition works even for
blueprints that are themselves multi-level. For example::

    #meta label(2beds) dig 2 levels of bedrooms
    dig_bedrooms/1 repeat(down 2)

    #meta label(6beds) dig 6 levels of bedrooms
    /2beds repeat(down 3)

You can use ``<`` and ``>`` for short, instead of ``up`` and ``down``. The comma
or space between the direction and the number is optional as well. The following
lines are all equivalent::

    /2beds repeat(down 3)
    /2beds repeat(down, 3)
    /2beds repeat(>3)

**Shifting blueprints**

Syntax: shift(<x shift>[[,] <y shift>])

The values can be positive or negative. Negative values for x shift to the left,
positive to the right. Negative values for y shift up, positive down. Note the
semantics for the y axis are opposite compared to regular graphs on paper. This
is because the y coordinates in the DF game map start a 0 at the top and
increase as they go down.

**Transforming blueprints**

Syntax: transform(<transformation>[[,] <transformation>...])

Applies a geometric transformation to the blueprint. The supported
transformations are:

:rotcw or cw:   Rotates the blueprint 90 degrees clockwise.
:rotccw or ccw: Rotates the blueprint 90 degrees counterclockwise.
:fliph:         Flips the blueprint horizontally (left edge becomes right edge).
:flipv:         Flips the blueprint vertically (top edge becomes bottom edge).

If you specify more than one transformation, they will be applied in the order
they appear in.

If you use both ``shift()`` and ``transform()`` markers on the same blueprint,
shifting is applied after all transformations are complete. If you want shifting
to be applied before the transformations, or in between transformations, you can
use nested meta blueprints. For example, the following blueprint will shift the
``/hallway`` blueprint to the right by 20 units and then rotate it clockwise::

    #meta label(shift_right) hidden()
    /hallway shift(20)
    #meta label(rotate_after_shift)
    /shift_right transform(cw)

Transforming build blueprints will also change the properties of buildings that
care about direction. For example, a bridge that opens to the North, rotated
clockwise, will open to the East when applied to the map.

Direction keys that move the cursor on the map will also be transformed. For
example, the keys ``g{Up 4}&`` that would cause a stockpile to give to a
workshop 4 tiles to the North become ``g{Right 4}&`` when played back on a
clockwise-rotated ``#query`` blueprint. Direction keys that don't move the map
cursor, for example when on the stockpile configuration screen, are not changed
by blueprint rotation.

.. _quickfort-other-modes:

Other blueprint modes
~~~~~~~~~~~~~~~~~~~~~

In addition to the powerful ``#meta`` mode described above, there are a few
additional blueprint modes that become useful when you are sharing your
blueprints with others or managing complex blueprint sets. Instead of mapping
tile positions to map modifications like the basic modes do, these "blueprints"
have specialized, higher-level uses:

==============  ===========
Blueprint mode  Description
==============  ===========
config          Play back key sequences that are not related to map tiles
notes           Display long messages, such as help text or blueprint
                walkthroughs
aliases         Define aliases that can be used by other ``#query`` blueprints
                in the same file
ignore          Hide a section of your spreadsheet from quickfort, useful for
                scratch space or personal notes
==============  ===========

.. _quickfort-config-blueprints:

Config blueprints
`````````````````

A ``#config`` blueprint is used to send unfiltered keystrokes directly to the
DF UI without interacting with specific map tiles. They have access to the same
keystroke aliases as ``#query`` blueprints, but ``#config`` blueprints differ
from ``#query`` blueprints in a few critical ways:

- Whereas the "home" mode for ``#query`` blueprints is the "query" mode
  (:kbd:`q`), ``#config`` blueprints start on the default map screen -- the view
  you have when you're looking at the map with no sidebar visible. The keystroke
  or alias sequence in each spreadsheet cell in a ``#config`` blueprint must
  begin and end on the default map screen.
- The cursor position is not set for ``#config`` blueprints. This means that it
  doesn't matter what spreadsheet cell you put your text in. The blueprint cell
  location does not correspond to a map tile.

A ``#config`` blueprint is best used for accessing game menus that are not
associated with map tiles, such as the hotkey menu (:kbd:`H`), the military
menu (:kbd:`m`), or the standing orders menu (:kbd:`o`). In other words, use a
``#config`` blueprint when you want to configure the game itself, not the tiles
on the map. A ``#config`` blueprint is better for these menus than a ``#query``
blueprint because the cursor can jump around in unpredictable ways when
accessing these non-cursor modes and then re-entering query mode. This will
cause quickfort to detect a ``#query`` blueprint error and stop executing.
Also, ``#query`` blueprints will skip playing back a key sequence entirely if
it doesn't detect a building or zone on the target tile. A ``#config``
blueprint doesn't need a building or zone to exist in order to run.

Note that you *can* enter any mode you want during a ``#config`` blueprint
keystroke sequence (as long as you get back to the default map screen by the end
of the sequence), even modes that provide a cursor on the screen. It's just that
the position of that cursor is not guaranteed to be on any specific tile. If you
need access to a cursor, you probably should be using a ``#query`` blueprint
instead.

.. _quickfort-notes:

Notes blueprints
````````````````

Sometimes you just want to record some information about your blueprints, such
as when to apply them, what preparations you need to make, or what the
blueprints contain. The `message() <quickfort-message>` modeline marker is
useful for small, single-line messages, but a ``#notes`` blueprint is more
convenient for long messages or messages that span many lines. The lines in a
``#notes`` blueprint are output as if they were contained within one large
multi-line ``message()`` marker. For example, the following (empty) ``#meta``
blueprint::

   "#meta label(help) message(This is the help text for the blueprint set
   contained in this file.

   First, make sure that you embark in...) blueprint set walkthough"

could more naturally be written as a ``#notes`` blueprint::

   #notes label(help) blueprint set walkthrough
   This is the help text for the blueprint set
   contained in this file

   First, make sure that you embark in...

The ``#meta`` blueprint is all squashed into a single spreadsheet cell, using
embedded newlines. Each line of the ``#notes`` "blueprint", however, is in a
separate cell, allowing for much easier viewing and editing.

.. _quickfort-aliases-blueprints:

Aliases blueprints
``````````````````

There are keystroke aliases that `come with DFHack <quickfort-alias-guide>` that
are usable by all blueprints, and you have the ability to define custom aliases
in :source:`dfhack-config/quickfort/aliases.txt` that are visible to all your
blueprints as well. An ``#aliases`` blueprint can define custom aliases that are
only visible to the current ``.csv`` or ``.xlsx`` file. Packaging aliases in the
same file that uses them is convenient for specialized aliases that are only
useful to a particular blueprint. Also, if you want to share your blueprint with
others, defining your aliases in an ``#aliases`` blueprint will help your
blueprint to work "out of the box", and you won't need others to add your custom
aliases to their ``dfhack-config/quickfort/aliases.txt`` files.

Although we're calling them "blueprints", ``#aliases`` blueprints are not actual
blueprints, and they don't show up when you run ``quickfort list``. The aliases
are just automatically read in when you run any ``#query`` blueprint that is
defined in the same file.

Aliases can be in either of two formats, and you can mix formats freely within
the same ``#aliases`` section. The first format is the same as what is used in
the ``aliases.txt`` files::

    #aliases
    aliasname: aliasdefinition

Aliases in this format must appear in the first column of a row.

The second format has the alias name in the first column and the alias
definition in the second column, with no ``:`` separator::

    #aliases
    aliasname,aliasdefinition

There can be multiple #aliases sections defined in a .csv file or .xlsx
spreadsheet. The aliases are simply combined into one list. If an alias is
defined multiple times, the last definition wins.

See the `quickfort-alias-guide` for help with the alias definition syntax.

Ignore blueprints
`````````````````

If you don't want some data to be visible to quickfort at all, use an
``#ignore`` blueprint. All lines until the next modeline in the file or sheet
will be completely ignored. This can be useful for personal notes, scratch
space, or temporarily "commented out" blueprints.

.. _quickfort-packaging:

Packaging a set of blueprints
-----------------------------

A complete specification for a section of your fortress may contain 5 or more
separate blueprints, one for each "phase" of construction (dig, build, place
stockpiles, designate zones, and query adjustments).

To manage all the separate blueprints, it is often convenient to keep related
blueprints in a single file. For .xlsx spreadsheets, you can keep each blueprint
in a separate sheet. Online spreadsheet applications like `Google
Sheets <https://sheets.new>`__ make it easy to work with multiple related
blueprints, and, as a bonus, they retain any formatting you've set, like column
sizes and coloring.

For both .csv files and .xlsx spreadsheets you can also add as many blueprints
as you want in a single file or sheet. Just add a modeline in the first column
to indicate the start of a new blueprint. Instead of multiple .csv files, you
can concatenate them into one single file. This is especially useful when you
are sharing your blueprints with others. A single file is much easier to manage
than a directory of files.

For example, you can write multiple blueprints in one file like this::

   #dig label(bed1)
   d d d d #
   d d d d #
   d d d d #
   d d d d #
   # # # # #
   #build label(bed2)
   b   f h #
           #
           #
   n       #
   # # # # #
   #place label(bed3)
           #
   f(2x2)  #
           #
           #
   # # # # #
   #query label(bed4)
           #
   booze   #
           #
           #
   # # # # #
   #query label(bed5)
   r{+ 3}& #
           #
           #
           #
   # # # # #

Of course, you could still choose to keep your blueprints in separate files and
just give related blueprints similar names::

   bedroom.1.dig.csv
   bedroom.2.build.csv
   bedroom.3.place.csv
   bedroom.4.query.csv
   bedroom.5.query2.csv

The naming and organization is completely up to you.

Buildingplan integration
------------------------

Buildingplan is a DFHack plugin that keeps building construction jobs in a
suspended state until the materials required for the job are available. This
prevents a building designation from being canceled when a dwarf picks up the
job but can't find the materials.

As long as the `buildingplan` plugin is enabled, quickfort will use it to manage
construction. The buildingplan plugin has an `"enabled" setting
<buildingplan-settings>` for each building type, but those settings only apply
to buildings created through the buildingplan user interface. Quickfort will
still use buildingplan to plan buildings even if the buildingplan UI says that
building type is not "enabled".

In addition, buildingplan has a "quickfort_mode" setting for compatibility with
legacy Python Quickfort. This setting has no effect on DFHack Quickfort, which
will use buildingplan to manage everything designated in a ``#build`` blueprint
regardless of the buildingplan UI settings.

However, quickfort *does* use `buildingplan's filters <buildingplan-filters>`
for each building type. For example, you can use the buildingplan UI to set the
type of stone you want your walls made out of. Or you can specify that all
buildingplan-managed chairs and tables must be of Masterful quality. The current
filter settings are saved with planned buildings when the ``#build`` blueprint
is run. This means you can set the filters the way you want for one blueprint,
run the blueprint, and then freely change them again for the next blueprint,
even if the first set of buildings haven't been built yet.

Note that buildings are still constructed immediately if you already have the
materials. However, with buildingplan you now have the freedom to apply
``#build`` blueprints before you manufacture the resources. The construction
jobs will be fulfilled whenever the materials become available.

Since it can be difficult to figure out exactly what source materials you need
for a ``#build`` blueprint, quickfort supplies the ``orders`` command. It
enqueues manager orders for everything that the buildings in a ``#build``
blueprint require. See the `next section <generating-manager-orders>`_ for more
details on this.

Alternately, if you know you only need a few types of items, the `workflow`
plugin can be configured to build those items continuously for as long as they
are needed.

If you do not want to enable the buildingplan plugin, run ``quickfort orders``
and make sure all manager orders are fulfilled before applying a ``#build``
blueprint. Otherwise you will get job cancellation spam when the buildings can't
be built with available materials.

Generating manager orders
-------------------------

Quickfort can generate manager orders to make sure you have the proper items in
stock for a ``#build`` blueprint.

Many items can be manufactured from different source materials. Orders will
always choose rock when it can, then wood, then cloth, then iron. You can always
remove orders that don't make sense for your fort and manually enqueue a similar
order more to your liking. For example, if you want silk ropes instead of cloth
ropes, make a new manager order for an appropriate quantity of silk ropes, and
then remove the generated cloth rope order.

Anything that requires generic building materials (workshops, constructions,
etc.) will result in an order for a rock block. One "Make rock blocks" job
produces four blocks per boulder, so the number of jobs ordered will be the
number of blocks you need divided by four (rounded up). You might end up with a
few extra blocks, but not too many.

If you want your constructions to be in a consistent color, be sure to choose a
rock type for all of your 'Make rock blocks' orders by selecting the order and
hitting :kbd:`d`. You might want to set the rock type for other non-block orders
to something different if you fear running out of the type of rock that you want
to use for blocks. You should also set the `buildingplan` material filter for
construction building types to that type of rock as well so other blocks you
might have lying around aren't used.

Extra Manager Orders
~~~~~~~~~~~~~~~~~~~~

In ``#build`` blueprints, there are a few building types that will generate
extra manager orders for related materials:

-  Track stops will generate an order for a minecart
-  Traction benches will generate orders for a table, mechanism, and rope
-  Levers will generate an order for an extra two mechanisms for connecting the
   lever to a target
-  Cage traps will generate an order for a cage


Stockpiles in ``#place`` blueprints that `specify wheelbarrow or container
counts <quickfort-place-containers>` will generate orders for the appropriate
number of bins, pots, or wheelbarrows.

Tips and tricks
---------------

-  During blueprint application, especially query blueprints, don't click the
   mouse on the DF window or type any keys. They can change the state of the
   game while the blueprint is being applied, resulting in strange errors.

-  After digging out an area, you may wish to smooth and/or engrave the area
   before starting the build phase, as dwarves may be unable to access walls or
   floors that are behind/under built objects.

-  If you are designating more than one level for digging at a time, you can
   make your miners more efficient by using marker mode on all levels but one.
   This prevents your miners from digging out a few tiles on one level, then
   running down/up the stairs to do a few tiles on an adjacent level. With only
   one level "live" and all other levels in marker mode, your miners can
   concentrate on one level at a time. You just have to remember to "unmark" a
   new level when your miners are done with their current one. Alternately, if
   you have a chokepoint between levels (e.g. a central staircase), you can set
   the chokepoint to be dug at a lower priority than all the other tiles on the
   level. This will ensure your miners complete digging out a level before
   continuing on to the next.

-  As of DF 0.34.x, it is no longer possible to build doors (:kbd:`d`) at the
   same time that you build adjacent walls (:kbd:`C`:kbd:`w`). Doors must now be
   built *after* adjacent walls are constructed. This does not affect the more
   common case where walls exist as a side-effect of having dug-out a room in a
   ``#dig`` blueprint, but if you are building your own walls, be aware that
   walls must be built before you run the blueprint to designate attached doors.

- Quickfort is a very powerful tool. See the `case study <dreamfort-case-study>`
  below for more ideas on how to build awesome blueprints!

Caveats and limitations
-----------------------

-  If you use the the ``bags`` alias, be aware that the game does not
   differentiate between empty and full bags. Therefore, you can get bags of
   gypsum power in your "bags" stockpile unless you are careful to assign all
   your gypsum to your hospital.

-  Weapon traps and upright spear/spike traps can currently only be built with a
   single weapon.

-  Pressure plates can be built, but they cannot be usefully configured yet.

-  Building instruments is not yet supported.

-  DFHack Quickfort is a large project, and there are bound to be bugs! Please
   report them at the :issue:`DFHack issue tracker <>` so they can be addressed.

.. _dreamfort-case-study:

Dreamfort case study: a practical guide to advanced blueprint design
--------------------------------------------------------------------

While syntax definitions and toy examples will certainly get you started with
your blueprints, it may not be clear how all the quickfort features fit together
or what the best practices are, especially for large and complex blueprint sets.
This section walks through the "Dreamfort" blueprints found in the `DFHack
blueprint library <dreamfort>`, highlighting design choices and showcasing
practical techniques that can help you create better blueprints. Note that this
is not a guide for how to design the best *fort* (there is plenty about that
:wiki:`on the wiki <Design strategies>`). This is essentially an extended tips
and tricks section focused on how to make usable and useful quickfort blueprints
that will save you time and energy.

Almost every quickfort feature is used somewhere in Dreamfort, so the blueprints
are very useful as reference examples. You can copy the Dreamfort blueprints and
use them as starting points for your own, or just refer to them when you create
something similar.

In this case study, we'll start by discussing the high level organization of the
Dreamfort blueprint set. Then we'll walk through the spreadsheets for each of
the fort levels in turn, calling out feature usage examples and explaining the
parts that might not be obvious just from looking at them.

If you haven't built Dreamfort before, maybe try an embark in a flat area and
take it for a spin! It will help put the following sections in context. There is
also a pre-built Dreamfort available for download on
:dffd:`dffd <15434>` if you just want an interactive reference.

Dreamfort organization and packaging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Dreamfort blueprints are distributed with DFHack as
:source:`one large .csv file <data/blueprints/library/dreamfort.csv>`, but
editing in that format would be frustrating. Instead, the blueprints are
edited `online as Google drive spreadsheets
<https://drive.google.com/drive/folders/1iS90EEVqUkxTeZiiukVj1pLloZqabKuP>`__.
Either the .csv file or the .xlsx files can be read and applied by quickfort,
but it made more sense to distribute the blueprints as a .csv so users would
only have to remember one filename. Also, .csv files are text-based, which works
more naturally with the DFHack source control system. We use the
`xlsx2csv <https://github.com/dilshod/xlsx2csv>`__ utility to do the conversion
from .xlsx to .csv format.

.. topic:: Tip

    Include a ``#notes`` section with information about how to use your
    blueprint.

Each spreadsheet has a "help" sheet with a ``#notes`` blueprint that displays a
walkthrough and other useful details. This is the first sheet in each
spreadsheet so it will be selected by default if the user doesn't specify a
label name. For example, just running ``quickfort run
library/dreamfort.csv`` will display Dreamfort's `introduction text
<https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw>`__.

Do not neglect writing the help text! Not only will it give others a chance to
use your blueprints appropriately, but the help you write will remind *you* what
you were thinking when you wrote the blueprint in the first place.

.. topic:: Tip

    Include custom alias definitions in the same file as the blueprint.

If any blueprint in the set uses custom aliases that other users won't have in
their :source:`data/quickfort/aliases-common.txt` files, be sure to define them
in the blueprint itself in an `quickfort-aliases-blueprints` section. Then other
people can use your blueprint right away without having to manually copy aliases
into their personal :source:`dfhack-config/quickfort/aliases.txt` files.

The surface_ level: how to manage complexity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _surface: https://docs.google.com/spreadsheets/d/1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU

.. image:: https://drive.google.com/uc?export=download&id=1YL_vQJLB2YnUEFrAg9y3HEdFq3Wpw9WP
  :alt: Annotated screenshot of the dreamfort surface level
  :target: https://drive.google.com/file/d/1YL_vQJLB2YnUEFrAg9y3HEdFq3Wpw9WP
  :align: center

For smaller blueprints, packaging and usability are not really that important -
just write it, run it, and you're done. However, as your blueprints become
larger and more detailed, there are some best practices that can help you deal
with the added complexity. Dreamfort's surface level is many steps long since
there are trees to be cleared, holes to be dug, flooring to be laid, and
bridges to be built, and each step requires the previous step to be completely
finished before it can begin. Therefore, a lot of thought went into minimizing
the toil associated with applying so many blueprints.

.. topic:: Tip

    Use meta blueprints to script blueprint sequences and reduce the number of
    quickfort commands you have to run.

The single most effective way to make your blueprint sets easier to use is to
group them with `meta blueprints <quickfort-meta>`. For the Dreamfort set of
blueprints, each logical "step" generally takes more than one blueprint. For
example, with ``#meta`` blueprints, setting up pastures with a ``#zone``
blueprint, placing starting stockpiles with a ``#place`` blueprint, building
starting workshops with a ``#build`` blueprint, and configuring the stockpiles
with a ``#query`` blueprint can all be done with a single command. Bundling
blueprints with ``#meta`` blueprints reduced the number of steps in Dreamfort
from 61 to 30, and it also made it much clearer to see which blueprints can be
applied at once without unpausing the game. Check out dreamfort_surface's "`meta
<https://docs.google.com/spreadsheets/d/1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit#gid=972927200>`__"
sheet to see how much meta blueprints can simplify your life.

You can define `as many blueprints as you want <quickfort-packaging>` on one
sheet, but this is especially useful when writing meta blueprints. It's like
having a bird's eye view of your entire plan in one sheet.

.. topic:: Tip

    Keep the blueprint list uncluttered by using ``hidden()`` markers.

If a blueprint is bundled into a meta blueprint, it does not need to appear in
the ``quickfort list`` output since you won't be running it directly. Add a
`hidden() marker <quickfort-hidden>` to those blueprints to keep the list
output tidy. You can still access hidden blueprints with ``quickfort list
--hidden`` if you need to -- for example to reapply a partially completed
``#build`` blueprint -- but now they won’t clutter up the normal blueprint list.

.. topic:: Tip

    Name your blueprints with a common prefix so you can find them easily.

This goes for both the file name and the `modeline label() <quickfort-label>`.
Searching and filtering is implemented for both the
``quickfort list`` command and the quickfort interactive dialog. If you give
related blueprints a common prefix, it makes it easy to set the filters to
display just the blueprints that you're interested in. If you have a lot of
blueprints, this can save you a lot of time. Dreamfort uses the level name as a
prefix for the labels, like "surface1", "surface2", "farming1", etc. So if I’m
in the middle of applying the surface blueprints, I’d set the filter to
``dreamfort surface`` to just display the relevant blueprints.

.. topic:: Tip

    Add descriptive comments that remind you what the blueprint contains.

If you've been away from Dwarf Fortress for a while, it's easy to forget what
your blueprints actually do. Make use of `modeline comments
<quickfort-modeline>` so your descriptions are visible in the blueprint list.
If you use meta blueprints, all your comments can be conveniently edited on one
sheet, like in surface's meta sheet.

.. topic:: Tip

    Use ``message()`` markers to remind yourself what to do next.

`Messages <quickfort-message>` are displayed after a blueprint is applied. Good
things to include in messages are:

* The name of the next blueprint to apply and when to run it
* Whether ``quickfort orders`` should be run for the current or an upcoming step
* Any actions that you have to perform manually after running the blueprint,
  like assigning minecarts to hauling routes or pasturing animals in
  newly-created zones

These things are just too easy to forget. Adding a ``message()`` can save you
from time-wasting mistakes. Note that ``message()`` markers can still appear on
the ``hidden()`` blueprints, and they'll still get shown when the blueprint is
run via a ``#meta`` blueprint. For an example of this, check out the `zones
sheet <https://docs.google.com/spreadsheets/d/1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit#gid=1226136256>`__
where the pastures are defined.

The farming_ level: fun with stockpiles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _farming: https://docs.google.com/spreadsheets/d/1iuj807iGVk6vsfYY4j52v9_-wsszA1AnFqoxeoehByg

.. image:: https://drive.google.com/uc?export=download&id=1fBC3G5Y888l4tVe5REAyAd_zeojADVme
  :alt: Annotated screenshot of the dreamfort farming level
  :target: https://drive.google.com/file/d/1fBC3G5Y888l4tVe5REAyAd_zeojADVme
  :align: center

It is usually convenient to store closely associated blueprints in the same
spreadsheet. The farming level is very closely tied to the surface because the
miasma vents dug on the surface have to perfectly line up with where waste
products are placed on the farming level. However, surface is a separate z-level
and, more importantly, already has many many blueprints of its own. Farming is
therefore split into a separate file.

.. topic:: Tip

    Automate stockpile chains when you can, and write ``message()`` reminders
    when you can't.

The farming level starts doing interesting things with ``#query`` blueprints and
stockpiles. Note the `careful customization
<https://docs.google.com/spreadsheets/d/1iuj807iGVk6vsfYY4j52v9_-
wsszA1AnFqoxeoehByg/edit#gid=486506218>`__ of the food stockpiles and the
stockpile chains set up with the ``give*`` aliases. This is so when multiple
stockpiles can hold the same item, the largest can keep the smaller ones filled.
For example the ``give2up`` alias funnels seeds from the seeds feeder pile to
the container-enabled seed storage pile. If you have multiple stockpiles holding
the same type on different z-levels, though, this can be tricky to set up with a
blueprint. Here, the jugs and pots stockpiles must be manually linked to the
quantum stockpile on the industry level, since we can't know beforehand how many
z-levels away that is. Note how we call that out in the ``#query`` blueprint's
``message()``.

.. topic:: Tip

    Use aliases to set up hauling routes and quantum stockpiles.

Hauling routes are notoriously fiddly to set up, but they can be automated with
blueprints. Check out the Southern area of the ``#place`` and ``#query``
blueprints for how the quantum refuse dump is configured with simple aliases
from the alias library.

The industry_ level: when not to use aliases
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _industry: https://docs.google.com/spreadsheets/d/1gvTJxxRxZ5V4vXkqwhL-qlr_lXCNt8176TK14m4kSOU

.. image:: https://drive.google.com/uc?export=download&id=1emMaHHCaUPcdRbkLQqvr-0ZCs2tdM5X7
  :alt: Annotated screenshot of the dreamfort industry level
  :target: https://drive.google.com/file/d/1emMaHHCaUPcdRbkLQqvr-0ZCs2tdM5X7
  :align: center

The industry level is densely packed and has more complicated examples of
stockpile configurations and quantum dumps. However, what I'd like to call out
first are the key sequences that are *not* in aliases.

.. topic:: Tip

     Don't use aliases for ad-hoc cursor movements.

It may be tempting to put all query blueprint key sequences into aliases to make
them easier to edit, keep them all in one place, and make them reusable, but
some key sequences just aren't very valuable as aliases.

`Check out <https://docs.google.com/spreadsheets/d/1gvTJxxRxZ5V4vXkqwhL-qlr_lXCNt8176TK14m4kSOU/edit#gid=787640554>`__
the Eastern (goods) and Northern (stone and gems) quantum stockpiles -- cells
I19 and R10. They give to the jeweler's workshop to prevent the jeweler from
using the gems held in reserve for strange moods. The keys are not aliased since
they're dependent on the relative positions of the tiles where they are
interpreted, which is easiest to see in the blueprint itself. Also, if you move
the workshop, it's easier to fix the stockpile link right there in the blueprint
instead of editing a separate alias definition.

There are also good examples in the ``#query`` blueprint for how to use the
``permit`` and ``forbid`` stockpile aliases.

.. topic:: Tip

     Put all configuration that must be applied in a particular order in the
     same spreadsheet cell.

Most of the baseline aliases distributed with DFHack fall into one of three
categories:

1. Make a stockpile accept only a particular item type in a category
2. Permit an item type, but do not otherwise change the stockpile configuration
3. Forbid an item type, but do not otherwise change the stockpile configuration

If you have a stockpile that covers multiple tiles, it might seem natural to put
one alias per spreadsheet cell. The aliases still all get applied to the
stockpile, and with only one alias per cell, you can just type the alias name
and avoid having to use the messier-looking ``{aliasname}`` syntax::

    #place Declare a food stockpile
    f(3x3)
    #query Incorrectly configure a food stockpile to accept tallow and dye
    tallow
    permitdye

However, in quickfort there are no guarantees about which cell will be
processed first. In the example above, we obviously intend for the food
stockpile to have tallow exclusively permitted, then to add dye. It could happen
that the two aliases are applied in the opposite order, though, and we'd end up
with dye being permitted, then everything (including dye) being forbidden, and,
finally, tallow being enabled. To make sure you always get what you want, write
order-sensitive aliases on the same line::

    #place Declare a food stockpile
    f(3x3)
    #query Properly configure a food stockpile to accept tallow and dye
    {tallow}{permitdye}

You can see a more complex example of this with the ``meltables`` stockpiles in
the `lower left corner <https://docs.google.com/spreadsheets/d/1gvTJxxRxZ5V4vXkqwhL-qlr_lXCNt8176TK14m4kSOU/edit#gid=787640554>`__
of the industry level.

The services_ level: handling multi-level dig blueprints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _services: https://docs.google.com/spreadsheets/d/1IBy6_pGEe6WSBCLukDz_5I-4vi_mpHuJJyOp2j6SJlY

.. image:: https://drive.google.com/uc?export=download&id=13vDIkTVOZGkM84tYf4O5nmRs4VZdE1gh
  :alt: Annotated screenshot of the dreamfort services level
  :target: https://drive.google.com/file/d/13vDIkTVOZGkM84tYf4O5nmRs4VZdE1gh
  :align: center

Services is a multi-level blueprint that includes a well cistern beneath the
main level. Unwanted ramps caused by channeling are an annoyance, but we can
avoid getting a ramp at the bottom of the cistern with careful use of `dig
priorities <quickfort-dig-priorities>`.

.. topic:: Tip

    Use dig priorities to control ramp creation.

We can `ensure <https://docs.google.com/spreadsheets/d/1IBy6_pGEe6WSBCLukDz_5I-4vi_mpHuJJyOp2j6SJlY/edit#gid=962076234>`__
the bottom level is carved out before the layer above is channelled by assigning
the channel designations lower priorities (the ``h5``\s in the third layer --
scroll down).

An alternative is to have a follow-up blueprint that removes any undesired
ramps. We did this on the
`surface <https://docs.google.com/spreadsheets/d/1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit#gid=1790750180>`__
and
`farming <https://docs.google.com/spreadsheets/d/1iuj807iGVk6vsfYY4j52v9_-wsszA1AnFqoxeoehByg/edit#gid=436537058>`__
levels with the miasma vents since it would be too complicated to synchronize
the digging between the two layers.

The guildhall_ level: avoiding smoothing issues
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _guildhall: https://docs.google.com/spreadsheets/d/1wwKcOpEW-v_kyEnFyXS0FTjvLwJsyWbCUmEGaXWxJyU

.. image:: https://drive.google.com/uc?export=download&id=17jHiCKeZm6FSS-CI4V0r0GJZh09nzcO_
  :alt: Annotated screenshot of the dreamfort guildhall level
  :target: https://drive.google.com/file/d/17jHiCKeZm6FSS-CI4V0r0GJZh09nzcO_
  :align: center

The goal of this level is to provide rooms for ``locations`` like guildhalls,
libraries, and temples. The value of these rooms is very important, so we are
likely to smooth and engrave everything. To smooth or engrave a wall tile, a
dwarf has to be adjacent to it, and since some furniture, like statues, block
dwarves from entering a tile, where you put them affects what you can access.

.. topic:: Tip

    Don't put statues in corners unless you want to smooth everything first.

In the guildhall level, the statues are placed so as not to block any wall
corners. This gives the player freedom for choosing when to smooth. If a statue
blocks a corner, or if a line of statues blocks a wall segment, it forces the
player to smooth before building the statues. Otherwise they have to bother with
temporarily removing statues to smooth the walls behind them.

The beds_ levels: multi level meta blueprints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _beds: https://docs.google.com/spreadsheets/d/1QNHORq6YmYfuVVMP5yGAFCQluary_JbgZ-UXACqKs9g

.. image:: https://drive.google.com/uc?export=download&id=1IBqCf6fF3lw7sHiBE_15Euubysl5AAiS
  :alt: Annotated screenshot of the dreamfort noble suites
  :target: https://drive.google.com/file/d/1IBqCf6fF3lw7sHiBE_15Euubysl5AAiS
  :align: center

.. image:: https://drive.google.com/uc?export=download&id=1mDQQXG8BnXqasRGFC9R5N6xNALiswEyr
  :alt: Annotated screenshot of the dreamfort apartments
  :target: https://drive.google.com/file/d/1mDQQXG8BnXqasRGFC9R5N6xNALiswEyr
  :align: center

The suites and apartments blueprints are straightforward. The only fancy bit
is the meta blueprint that digs the stack of apartment levels, which brings us
to our final tip:

.. topic:: Tip

    Use meta blueprints to lay out repeated adjacent levels.

We couldn't use this technique for the entire fortress since there is often an
aquifer between the farming and industry levels, and we can't know beforehand
how many z-levels we need to skip. We can, however, automate the digging of
everything from the industry level down, including designating all apartment
levels at once. See the
`#meta <https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=284974597>`__
blueprint in the `Dreamfort help spreadsheet
<https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=0>`__
for how it uses a ``repeat()`` marker for the ``/apartments1`` blueprint to
apply it to five z-levels at once.

That's it! I hope this guide was useful to you. Please leave feedback on the
forums if you have ideas on how this guide (or the dreamfort blueprints) can be
improved!

Links
-----

**Quickfort links:**

-  `Quickfort command reference <quickfort>`
-  `quickfort-alias-guide`
-  `blueprint-library-guide`
-  :forums:`Quickfort forum thread <176889>`
-  :issue:`DFHack issue tracker <>`
-  :source:`Blueprint library source <data/blueprints/library>`
-  :source-scripts:`Quickfort source code <internal/quickfort>`

**Related tools:**

-  DFHack's `blueprint plugin <blueprint>` can generate blueprints from actual
   DF maps.
-  DFHack's `buildingplan plugin <buildingplan>` sets material and quality
   constraints for quickfort-placed buildings.
-  `Python Quickfort <http://joelpt.net/quickfort>`__ is the previous,
   Python-based implementation that DFHack's quickfort script was inspired by.
