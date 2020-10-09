.. _quickfort-user-guide:

Quickfort User Guide
====================

`Quickfort <quickfort>` is a DFHack script that helps you build fortresses from
"blueprint" .csv and .xlsx files. Many applications exist to edit these files,
such as MS Excel and `Google Sheets <https://sheets.new>`__. You can also build
your plan "for real" in Dwarf Fortress, and then export your map using the `blueprint`
plugin. Most layout and building-oriented DF commands are supported through the
use of multiple files or spreadsheets, each describing a different phase of DF
construction: designation, building, placing stockpiles/zones, and setting
configuration.

The original idea and 1.0 codebase came from
`Valdemar's <https://dwarffortresswiki.org/index.php/User:Valdemar>`__
auto-designation macro. Joel Thornton (joelpt) reimplemented the core logic in
Python and extended its functionality with `Quickfort
2.0 <https://github.com/joelpt/quickfort>`__. This DFHack-native implementation,
called "DFHack Quickfort" or just "quickfort", builds upon Quickfort 2.0's
formats and features. DFHack Quickfort is written in Lua and interacts with
Dwarf Fortress memory structures directly, allowing for instantaneous blueprint
application, error checking and recovery, and many other advanced features.

This document focuses on DFHack Quickfort's capabilities and teaches players how
to understand and build blueprint files. Some of the text was originally written
by Joel Thornton, reused here with his permission.

For those just looking to apply blueprints, check out the `quickfort command's
documentation <quickfort>` for syntax. There are also many ready-to-use blueprints
available in the ``blueprints/library`` subfolder in your DFHack installation.
Browse them on your computer or :source:`online <data/blueprints/library>`,
or run ``quickfort list -l`` at the ``[DFHack]#`` prompt to list them, and then
``quickfort run`` to apply them to your fort!

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
   -  Supports multiple blueprints per .csv file/spreadsheet sheet
   -  "meta" blueprints that automate the application of sequences of blueprints
   -  Undo functionality for dig, build, place, and zone blueprints
   -  Automatic cropping of blueprints so you don't get errors if the blueprint
      extends off the map
   -  Can generate manager orders for everything required by a build blueprint
   -  Library of ready-to-use blueprints included
   -  Verbose output mode for debugging

-  Dig mode

   -  Supports all types of designations, including dumping/forbidding items and
      setting traffic settings
   -  Supports setting dig priorities
   -  Supports applying dig blueprints in marker mode
   -  Handles carving arbitrarily complex minecart tracks, including tracks that
      cross other tracks

-  Build mode

   -  DFHack buildingplan integration
   -  Designate complete constructions at once, without having to wait for each
      tile to become supported before you can build it
   -  Automatic expansion of building footprints to their minimum dimensions, so
      only the center tile of a multi-tile building needs to be recorded in the
      blueprint
   -  Tile occupancy and validity checking so, for example, buildings that
      cannot be placed on a certain tile will simply be skipped instead of the
      blueprint failing to apply. Blueprints that are only partially applied for
      any reason (for example, you need to dig out some more tiles) can be
      safely reapplied to build the remaining buildings.
   -  Relaxed rules for farm plot and road placement: you can still place the
      building even if an invalid tile (e.g. stone tiles for farm plots) splits
      the designated area into two parts
   -  Intelligent boundary detection for adjacent buildings of the same type
      (e.g. a 6x6 block of ``wj`` cells will be correctly split into 4 jeweler's
      workshops)

-  Place and zone modes

   -  Define stockpiles and zones in any continguous shape, not just rectangular
      blocks
   -  Configurable maximums for bins, barrels and wheelbarrows assigned to
      created stockpiles
   -  Automatic splitting of stockpiles and zones that exceed maximum dimension
      limits

-  Query mode

   -  Send arbitrary keystroke sequences to the UI -- *anything* you can do
      through the UI is supported
   -  Supports aliases to automate frequent keystroke combos
   -  Includes a library of pre-made and tested aliases to automate most common
      tasks, such as configuring stockpiles for important item types or creating
      named hauling routes for quantum stockpiles.
   -  Supports including aliases in other aliases, and repeating key sequences a
      specified number of times
   -  Skips sending key sequences when the cursor is over a tile that does not
      have a stockpile or building, so missing buildings won't desynchronize
      your blueprint
   -  Instant halting of query blueprint application when keystroke errors are
      detected, such as when a key sequence leaves us stuck in a submenu, to
      make blueprint misconfigurations easier to debug

Editing Blueprints
------------------

We recommend using a spreadsheet editor such as Excel, `Google
Sheets <https://sheets.new>`__, or `LibreOffice <https://www.libreoffice.org>`__
to edit blueprint files, but any text editor will do.

The format of Quickfort-compatible blueprint files is straightforward. The first
line (or upper-left cell) of the spreadsheet should look like this:

::

   #dig This is a decription.

The keyword "dig" tells Quickfort we are going to be using the Designations menu
in DF. The following "mode" keywords are understood:

::

   dig     Designations menu (d)
   build   Build menu (b)
   place   Place stockpiles menu (p)
   zone    Activity zones menu (i)
   query   Set building tasks/prefs menu (q)

If no modeline appears in the first cell, the file or sheet is interpreted as a
``#dig`` blueprint.

There are also "meta" and "notes" blueprints, but we'll talk about
`those <quickfort-meta>` `later <quickfort-notes>`.

Optionally following this keyword and a space, you may enter a comment. This
comment will appear in the output of ``quickfort list`` when run from the
``DFHack#`` prompt. You can use this space for explanations, attribution, etc.

Below this line begin entering the keys you want sent in each cell. For example,
we could dig out a 4x4 room like so (spaces are used as column separators here
for clarity, but a real .csv file would have commas):

::

   #dig
   d d d d #
   d d d d #
   d d d d #
   d d d d #
   # # # # #

Note the # symbols at the right end of each row and below the last row. These
are completely optional, but can be helpful to make the row and column positions
clear.

Once the dwarves have that dug out, let's build a walled-in bedroom within our
dug-out area:

::

   #build
   Cw Cw Cw Cw #
   Cw b  h  Cw #
   Cw       Cw #
   Cw Cw    Cw #
   #  #  #  #  #

Note my generosity - in addition to the bed (b) I've built a chest (h) here for
the dwarf as well. You must use the full series of keys needed to build
something in each cell, e.g. 'Cw' enters DF's constructions submenu (C) and
selects walls (w).

I'd also like to place a booze stockpile in the 2 unoccupied tiles in the room.

::

   #place Place a food stockpile
   ` ` ` ` #
   ` ` ` ` #
   ` f(2x1)#
   ` ` ` ` #
   # # # # #

This illustration may be a little hard to understand. The f(2x1) is in column 2,
row 3. All the other cells are empty. QF considers both "`" (backtick -- the
character under the tilde) and "~" (tilde) characters within cells to be empty
cells; this can help with multilayer or fortress-wide blueprint layouts as
'chalk lines'.

With f(2x1), we've asked QF to place a food stockpile 2 units wide by 1 high
unit. Note that the f(2x1) syntax isn't actually necessary here; we could have
just used:

::

   #place Place a food stockpile
   ` ` ` ` #
   ` ` ` ` #
   ` f f ` #
   ` ` ` ` #
   # # # # #

QF is smart enough to recognize this as a 2x1 food stockpile, and creates it as
such rather than as two 1x1 food stockpiles. Quickfort recognizes any connected
region of identical designations as a single stockpile. The tiles can be
connected orthogonally or diagonally, just as long as they are touching somehow.

Lastly, let's turn the bed into a bedroom and set the food stockpile to hold
only booze.

::

   #query
   ` ` ` ` #
   ` r&  ` #
   ` booze #
   ` ` ` ` #
   # # # # #

In column 2, row 2 we have "r&". This sends the "r" key to DF when the cursor is
over the bed, causing us to 'make room' and "&", which is a special symbol that
expands to "{Enter}", to indicate that we're done.

In column 2, row 3 we have "booze". This is one of many alias keywords defined
in the included :source:`baseline aliases file <data/quickfort/aliases-common.txt>`.
This particular alias sets a food stockpile to carry booze only. It sends the
keys needed to navigate DF's stockpile settings menu, and then sends an Escape
character ("^" or "{ESC}") to exit back to the map. It is important to exit out
of any menus that you enter while in query mode so that the cursor can move to
the next tile when it is done configuring the current tile.

Check out the included :source:`blueprint library <data/blueprints/library>`
to see many more examples. Read the baseline aliases file for helpful
pre-packaged aliases, or create your own in
:source:`dfhack-config/quickfort/aliases.txt` in your DFHack installation.

Area expansion syntax
~~~~~~~~~~~~~~~~~~~~~

In Quickfort, the following blueprints are equivalent:

::

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
form:

::

   keys(WxH)

In Quickfort the above two examples of specifying a contiguous 3x3 area produce
identical output: a single 3x3 designation will be performed, rather than nine
1x1 designations as the first example might suggest.

Area expansion syntax can only specify rectangular areas. If you want to create
extent-based structures (e.g. farm plots or stockpiles) in different shapes, use
the first format above. For example:

::

   #place L shaped food stockpile
   f f ` ` #
   f f ` ` #
   f f f f #
   f f f f #
   # # # # #

Area expansion syntax also sets boundaries, which can be useful if you want
adjacent, but separate, stockpiles of the same type:

::

   #place Two touching but separate food stockpiles
   f(4x2)  #
   ~ ~ ~ ~ #
   f(4x2)  #
   ~ ~ ~ ~ #
   # # # # #

As mentioned previously, "~" characters are ignored as comment characters and
can be used for visualizing the blueprint layout. The blueprint can be
equivalently written as:

::

   #place Two touching but separate food stockpiles
   f(4x2)  #
   ~ ~ ~ ~ #
   f f f f #
   f f f f #
   # # # # #

since the area expansion syntax of the upper stockpile prevents it from
combining with the lower, freeform syntax stockpile.

Area expansion syntax can also be used for buildings which have an adjustable
size, like bridges. The following blueprints are equivalent:

::

   #build a 4x2 bridge from row 1, col 1
   ga(4x2)  `  #
   `  `  `  `  #
   #  #  #  #  #

   #build a 4x2 bridge from row 1, col 1
   ga ga ga ga #
   ga ga ga ga #
   #  #  #  #  #

Automatic area expansion
~~~~~~~~~~~~~~~~~~~~~~~~

Buildings larger than 1x1, like workshops, can be represented in any of three
ways. You can designate just their center tile with empty cells around it to
leave room for the footprint, like this:

::

   #build a mason workshop in row 2, col 2 that will occupy the 3x3 area
   `  `  `  #
   `  wm `  #
   `  `  `  #
   #  #  #  #

Or you can fill out the entire footprint like this:

::

   #build a mason workshop
   wm wm wm #
   wm wm wm #
   wm wm wm #
   #  #  #  #

This format may be verbose for regular workshops, but it can be very helpful for
laying out structures like screw pump towers and waterwheels, whose "center
point" can be non-obvious.

Finally, you can use area expansion syntax to represent the workshop:

::

   #build a mason workshop
   wm(3x3)  #
   `  `  `  #
   `  `  `  #
   #  #  #  #

This style can be convenient for laying out multiple buildings of the same type.
If you are building a large-scale block factory, for example, this will create
20 mason workshops all in a row:

::

   #build line of 20 mason workshops
   wm(60x3) #

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

.. _quickfort-dig-priorities:

Dig priorities
~~~~~~~~~~~~~~

DF designation priorities are supported for ``#dig`` blueprints. The full syntax
is ``[letter][number][expansion]``, where if the ``letter`` is not specified,
``d`` is assumed, and if ``number`` is not specified, ``4`` is assumed (the
default priority). So each of these blueprints is equivalent:

::

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
want to dig everything just yet. In ``#dig`` mode, you can add a ``m`` before
any other designation letter to indicate that the tile should be designated in
marker mode. For example, to dig out the perimeter of a room, but leave the
center of the room marked for digging later:

::

   #dig
   d  d  d  d d #
   d md md md d #
   d md md md d #
   d md md md d #
   d  d  d  d d #
   #  #  #  # # #

Then you can use "Toggle Standard/Marking" (``d-M``) to convert the center tiles
to regular designations at your leisure.

To apply an entire dig blueprint in marker mode, regardless of what the
blueprint itself says, you can set the global quickfort setting
``force_marker_mode`` to ``true`` before you apply the blueprint.

Note that the in-game UI setting "Standard/Marker Only" (``d-m``) does not have
any effect on quickfort.

Stockpiles and zones
~~~~~~~~~~~~~~~~~~~~

It is very common to have stockpiles that accept multiple categories of items or
zones that permit more than one activity. Although it is perfectly valid to
declare a single-purpose stockpile or zone and then modify it with a ``#query``
blueprint, quickfort also supports directly declaring all the types on the
``#place`` and ``#zone`` blueprints. For example, to declare a 10x10 area that
is a pasture, a fruit picking area, and a meeting area all at once, you could
write:

::

   #zone main pasture and picnic area
   nmg(10x10)

And similarly, to declare a stockpile that accepts both corpses and refuse, you
could write:

::

   #place refuse heap
   yr(20x10)

The order of the individual letters doesn't matter.

To toggle the ``active`` flag for zones, add an ``a`` character to the string.
For example, to create a *disabled* pit zone (that you later intend to turn into
a pond and carefully fill to 3-depth water):

::

   #zone disabled future pond zone
   pa(1x3)

Note that while this notation covers most use cases, tweaking low-level zone
parameters, like hospital supply levels or converting between pits and ponds,
must still be done manually or with a ``#query`` blueprint.

Minecart tracks
~~~~~~~~~~~~~~~

There are two ways to produce minecart tracks, and they are handled very
differently by the game. You can carve them into hard natural floors or you can
construct them out of building materials. Constructed tracks are conceptually
simpler, so we'll start with them.

Constructed tracks
``````````````````

Quickfort supports the designation of track stops and rollers through the normal
mechanisms: a ``#build`` blueprint with ``CS`` and some number of ``d`` and
``a`` characters (for selecting dump direction and friction) in a cell
designates a track stop and a ``#build`` blueprint with ``Mr`` and some number
of ``s`` and ``q`` characters (for direction and speed) designates a roller.
This can get confusing very quickly and is very difficult to read in a
blueprint. Constructed track segments don't even have keys associated with them
at all!

To solve this problem, Quickfort provides the following keywords for use in
build blueprints:

::

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
their outside directions with the following blueprint:

::

   #build Example track
   trackstopW trackEW trackEW trackEW trackstopE

Note that the **only** way to build track and track/ramp segments is with the
keywords. The UI method of using "+" and "-" keys to select the track type from
a list does not work since DFHack Quickfort doesn't actually send keys to the UI
to build buildings. The text in your spreadsheet cells is mapped directly into
DFHack API calls. Only ``#query`` blueprints still send actual keycodes to the
UI.

Carved tracks
`````````````

In the game, you carve a minecart track by specifying a beginning and ending
tile and the game "adds" the designation to the tiles. You cannot designate
single tiles. For example to carve two track segments that cross each other, you
might use the cursor to designate a line of three vertical tiles like this:

::

   `  start here  `  #
   `  `           `  #
   `  end here    `  #
   #  #           #  #

Then to carve the cross, you'd do a horizonal segment:

::

   `           `  `         #
   start here  `  end here  #
   `           `  `         #
   #           #  #         #

This will result in a carved track that would be equivalent to a constructed
track of the form:

::

   #build
   `       trackS     `       #
   trackE  trackNSEW  trackW  #
   `       trackN     `       #
   #       #          #       #

To carve this same track with a ``#dig`` blueprint, you'd use area expansion
syntax with a height or width of 1 to indicate the segments to designate:

::

   #dig
   `       T(1x3)  `  #
   T(3x1)  `       `  #
   `       `       `  #
   #       #       #  #

"But wait!", I can hear you say, "How do you designate a track corner that opens
to the South and East? You can't put both T(1xH) and T(Wx1) in the same cell!"
This is true, but you can specify both width and height, and for tracks, QF
interprets it as an upper-left corner extending to the right W tiles and down H
tiles. For example, to carve a track in a closed ring, you'd write:

::

   #dig
   T(3x3)  `  T(1x3)  #
   `       `  `       #
   T(3x1)  `  `       #
   #       #  #       #

Which would result in a carved track simliar to a constructed track of the form:

::

   #build
   trackSE  trackEW  trackSW  #
   trackNS  `        trackNS  #
   trackNE  trackEW  trackNW  #
   #        #        #        #

.. _quickfort-modeline:

Modeline markers
~~~~~~~~~~~~~~~~

The modeline has some additional optional components that we haven't talked
about yet. You can:

-  give a blueprint a label by adding a ``label()`` marker
-  set a cursor offset and/or start hint by adding a ``start()`` marker
-  hide a blueprint from being listed with a ``hidden()`` marker
-  register a message to be displayed after the blueprint is successfully
   applied

The full modeline syntax, when everything is specified, is:

::

   #mode label(mylabel) start(X;Y;STARTCOMMENT) hidden() message(mymessage) comment

Note that all elements are optional except for the initial ``#mode`` (though, as
mentioned in the first section, if a modeline doesn't appear at all in the first
cell of a spreadsheet, the blueprint is interpreted as a ``#dig`` blueprint with
no optional markers). Here are a few examples of modelines with optional
elements before we discuss them in more detail:

::

   #dig start(3; 3; Center tile of a 5-tile square) Regular blueprint comment
   #build label(noblebedroom) start(10;15)
   #query label(configstockpiles) No explicit start() means cursor is at upper left corner
   #meta label(digwholefort) start(center of stairs on surface)
   #dig label(digdining) hidden() managed by the digwholefort meta blueprint
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

Start positions
```````````````

Start positions specify a cursor offset for a particular blueprint, simplifying
the task of blueprint alignment. This is very helpful for blueprints that are
based on a central staircase, but it helps whenever a blueprint has an obvious
"center". For example:

::

   #build start(2;2;center of workshop) label(masonw) a mason workshop
   wm wm wm #
   wm wm wm #
   wm wm wm #
   #  #  #  #

will build the workshop *centered* on the cursor, not down and to the right of
the cursor.

The two numbers specify the column and row (or X and Y offset) where the cursor
is expected to be when you apply the blueprint. Position 1;1 is the top left
cell. The optional comment will show up in the ``quickfort list`` output and
should contain information about where to position the cursor. If the start
position is 1;1, you can omit the numbers and just add a comment describing
where to put the cursor. This is also useful for meta blueprints that don't
actually care where the cursor is, but that refer to other blueprints that have
fully-specified ``start()`` markers. For example, a meta blueprint that refers
to the ``masonw`` blueprint above could look like this:

::

   #meta start(center of workshop) a mason workshop
   /masonw

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
players to take manual steps that cannot be automated, like assigning animals to
a pasture or assigning minecarts to a route, or listing the next step in a
series of blueprints. For long or multi-part messages, you can embed newlines:

::

   "#meta label(surface1) message(This would be a good time to start digging the industry level.
   Once the area is clear, continue with /surface2.) clear the embark site and set up pastures"

.. _quickfort-packaging:

Packaging a set of blueprints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A complete specification for a section of your fortress may contain 5 or more
separate blueprints, one for each "phase" of construction (dig, build, place
stockpiles, designate zones, query building adjustments).

To manage all the separate blueprints, it is often convenient to keep related
blueprints in a single file. For .xlsx spreadsheets, you can keep each blueprint
in a separate sheet. Online spreadsheet applications like `Google
Sheets <https://sheets.new>`__ make it easy to work with multiple related
blueprints, and, as a bonus, they retain any formatting you've set, like column
sizes and coloring.

For both .csv files and .xlsx spreadsheets you can also add as many blueprints
as you want in a single file or sheet. Just add a modeline in the first column
to indicate the start of a new blueprint. Instead of multiple .csv files, you
can concatenate them into one single file.

For example, you can store multiple blueprints together like this:

::

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

Of course, you could still choose to keep your blueprints in single-sheet .csv
files and just give related blueprints similar names:

::

   bedroom.1.dig.csv
   bedroom.2.build.csv
   bedroom.3.place.csv
   bedroom.4.query.csv
   bedroom.5.query2.csv

But the naming and organization is completely up to you.

.. _quickfort-meta:

Meta blueprints
~~~~~~~~~~~~~~~

Meta blueprints are blueprints that script a series of other blueprints. Many
blueprint packages follow this pattern:

-  Apply dig blueprint to designate dig areas
-  Wait for miners to dig
-  **Apply build buildprint** to designate buildings
-  **Apply place buildprint** to designate stockpiles
-  **Apply query blueprint** to configure stockpiles
-  Wait for buildings to get built
-  Apply a different query blueprint to configure rooms

Those three "apply"s in the middle might as well get done in one command instead
of three. A meta blueprint can encode that sequence. A meta blueprint refers to
other blueprints by their label (see the `Modeline markers`_ section above) in
the same format used by the `quickfort` command: ``<sheet name>/<label>``, or
just ``/<label>`` for blueprints in .csv files or blueprints in the same
spreadsheet sheet as the ``#meta`` blueprint that references them.

A few examples might make this clearer. Say you have a .csv file with the "bed"
blueprints in the previous section:

::

   #dig label(bed1)
   ...
   #build label(bed2)
   ...
   #place label(bed3)
   ...
   #query label(bed4)
   ...
   #query label(bed5)
   ...

Note how I've given them all labels so we can address them safely. If I hadn't
given them labels, they would receive default labels of "1", "2", "3", etc, but
those labels would change if I ever add more blueprints at the top. This is not
a problem if we're just running the blueprints individually from the
``quickfort list`` command, but meta blueprints need a label name that isn't
going to change over time.

So let's add a meta blueprint to this file that will combine the middle three
blueprints into one:

::

   "#meta plan bedroom: combines build, place, and stockpile config blueprints"
   /bed2
   /bed3
   /bed4

Now your sequence is shortened to:

-  Apply dig blueprint to designate dig areas
-  Wait for miners to dig
-  **Apply meta buildprint** to build buildings and designate/configure
   stockpiles
-  Wait for buildings to get built
-  Apply the final query blueprint to configure the room

You can use meta blueprints to lay out your fortress at a larger scale as well.
The ``#<`` and ``#>`` notation is valid in meta blueprints, so you can, for
example, store the dig blueprints for all the levels of your fortress in
different sheets in a spreadsheet, and then use a meta blueprint to designate
your entire fortress for digging at once. For example, say you have a
spreadsheet with the following layout:

+-------------------------------------------+----------------------------------+
| Sheet name                                | contents                         |
+===========================================+==================================+
| dig_farming                               | one #dig blueprint, no label     |
+-------------------------------------------+----------------------------------+
| dig_industry                              | one #dig blueprint, no label     |
+-------------------------------------------+----------------------------------+
| dig_dining                                | four #dig blueprints, with       |
|                                           | labels "main", "basement",       |
|                                           | "waterway", and "cistern"        |
+-------------------------------------------+----------------------------------+
| dig_guildhall                             | one #dig blueprint, no label     |
+-------------------------------------------+----------------------------------+
| dig_suites                                | one #dig blueprint, no label     |
+-------------------------------------------+----------------------------------+
| dig_bedrooms                              | one #dig blueprint, no label     |
+-------------------------------------------+----------------------------------+

We can add a sheet named "dig_all" with the following contents (we're expecting
a big fort, so we're planning for a lot of bedrooms):

::

   #meta dig the whole fortress (remember to set force_marker_mode to true)
   dig_farming/1
   #>
   dig_industry/1
   #>
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
   dig_bedrooms/1
   #>
   dig_bedrooms/1
   #>
   dig_bedrooms/1
   #>
   dig_bedrooms/1
   #>
   dig_bedrooms/1

Note that for blueprints without an explicit label, we still need to address
them by their auto-generated numerical label.

You can then hide the blueprints that you now manage with the ``#meta``-mode
blueprint from ``quickfort list`` by adding a ``hidden()`` marker to their
modelines. That way the output of ``quickfort list`` won't be cluttered by
blueprints that you don't need to run directly. If you ever *do* need to access
the managed blueprints individually, you can still see them with
``quickfort list --hidden``.

.. _quickfort-notes:

Notes blueprints
~~~~~~~~~~~~~~~~

Sometimes you just want to record some information about your blueprints, such
as when to apply them, what preparations you need to make, or what the
blueprints contain. The `message() <quickfort-message>` modeline marker is
useful for small, single-line messages, but a ``#notes`` blueprint is more
convenient for long messages or messages that span many lines. The lines in a
``#notes`` blueprint are output as if they were contained within a ``message()``
marker. For example, the following two blueprints result in the same output:

::

   "#meta label(help) message(This is the help text for the blueprint set
   contained in this file.

   More info here...) blueprint set walkthough"

   #notes label(help) blueprint set walkthrough
   This is the help text for the blueprint set
   contained in this file

   More info here...

The quotes around the ``#meta`` modeline allow newlines in a single cell's text.
Each line of the ``#notes`` "blueprint", however, is in a separate cell,
allowing for much easier viewing and editing.

Buildingplan integration
------------------------

Buildingplan is a DFHack plugin that keeps jobs in a suspended state until the
materials required for the job are available. This prevents a building
designation from being canceled when a dwarf picks up the job but can't find the
materials.

For all types that buildingplan supports, quickfort using buildingplan to manage
construction. Buildings are still constructed immediately if you have the
materials, but you now have the freedom to apply build blueprints before you
manufacture all required materials, and the jobs will be fulfilled as the
materials become available.

If a ``#build`` blueprint only refers to supported types, the buildingplan
integration pairs well with the `workflow` plugin, which can build items a few
at a time continuously as long as they are needed. For building types that are
not yet supported by buildingplan, a good pattern to follow is to first run
``quickfort orders`` on the ``#build`` blueprint to manufacture all the required
items, then apply the blueprint itself.

See the `buildingplan documentation <buildingplan>` for more information. As of
this writing, buildingplan only supports basic furniture.

Generating manager orders
-------------------------

Quickfort can generate manager orders to make sure you have the proper items in
stock to apply a ``#build`` blueprint.

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
hitting ``d``. You might want to set the rock type for other non-block orders to
something different if you fear running out of the type of rock that you want to
use for blocks.

There are a few building types that will generate extra manager orders for
related materials:

-  Track stops will generate an order for a minecart
-  Traction benches will generate orders for a table, mechanism, and rope
-  Levers will generate an order for an extra two mechanisms for connecting the
   lever to a target
-  Cage traps will generate an order for a cage

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
   new level when your miners are done with their current one.

-  As of DF 0.34.x, it is no longer possible to build doors (d) at the same time
   that you build adjacent walls (Cw). Doors must now be built *after* walls are
   constructed for them to be next to. This does not affect the more common case
   where walls exist as a side-effect of having dug-out a room in a #dig
   blueprint.

Caveats and limitations
-----------------------

-  Buildings will be designated regardless of whether you have the required
   materials, but if materials are not available when the construction job is
   picked up by a dwarf, the buildings will be canceled and the designations
   will disappear. Until the buildingplan plugin can be extended to support all
   building types, you should use ``quickfort orders`` to pre-manufacture all
   the materials you need for a ``#build`` blueprint before you apply it.

-  If you use the ``jugs`` alias in your ``#query``-mode blueprints, be aware
   that there is no way to differentiate jugs from other types of tools in the
   game. Therefore, ``jugs`` stockpiles will also take nest boxes and other
   tools. The only workaround is not to have other tools lying around in your
   fort.

-  Likewise for bags. The game does not differentiate between empty and full
   bags, so you'll get bags of gypsum power and sand in your bags stockpile
   unless you avoid collecting sand and are careful to assign all your gypsum to
   your hospital.

-  Weapon traps and upright spear/spike traps can currently only be built with a
   single weapon.

-  Pressure plates can be built, but they cannot be usefully configured yet.

-  Building instruments, bookcases, display furniture, and offering places are
   not yet supported by DFHack.

-  This script is relatively new, and there are bound to be bugs! Please report
   them at the :issue:`DFHack issue tracker <>` so they can be addressed.

Dreamfort case study: a practical guide to advanced blueprint design
--------------------------------------------------------------------

While syntax definitions and toy examples will certainly get you started with
your blueprints, it may not be clear how all the quickfort features fit together
or what the best practices are, especially for large and complex blueprint sets.
This section walks through the "Dreamfort" blueprints found in the DFHack
blueprint library, highlighting design choices and showcasing practical
techniques that can help you create better blueprints. Note that this is not a
guide for how to design the best forts (there is plenty about that `on the wiki
<http://dwarffortresswiki.org/index.php?title=Design_strategies>`__). This is
essentially an extended tips and tricks section focused on how to make usable
and useful quickfort blueprints that will save you time and energy.

The Dreamfort blueprints we'll be discussing are available in the library as
:source:`one large .csv file <data/blueprints/library/dreamfort.csv>`
or `online
<https://drive.google.com/drive/folders/1iS90EEVqUkxTeZiiukVj1pLloZqabKuP>`__ as
individual spreadsheets. Either can be read and applied by quickfort, but for us
humans, the online spreadsheets are much easier to work with. Each spreadsheet
has a "Notes" sheet with some useful details. Flip through some of the
spreadsheets and read the `walkthrough
<https://docs.google.com/spreadsheets/d/
13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=0>`__ to get oriented.
Also, if you haven't built Dreamfort before, try an embark in a flat area and
take it for a spin!

Almost every quickfort feature is used somewhere in Dreamfort, so the blueprints
as a whole are useful as practical examples. You can copy the blueprints and use
them as starting points for your own, or just refer to them when you create
something similar.

In this case study, we'll start by discussing the high level organization of the
Dreamfort blueprint set, using the "surface" blueprints as an example. Then
we'll walk through the blueprints for each of the remaining fort levels in turn,
calling out feature usage examples and explaining the parts that might not be
obvious just from looking at them.

The surface_ level: how to manage complexity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _surface: https://docs.google.com/spreadsheets/d/1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit?usp=sharing

For smaller blueprints, packaging and usability are not really that important -
just write it, run it, and you're done. However, as your blueprints become
larger and more detailed, there are some best practices that can help you deal
with the added complexity. Dreamfort's surface level is many steps long since
there are trees to be cleared, holes to be dug, flooring to be laid, and
furniture to be built, and each step requires the previous step to be completely
finished before it can begin. Therefore, a lot of thought went into minimizing
the toil associated with applying so many blueprints.

.. topic:: Tip

    Use meta blueprints to script blueprint sequences and reduce the number of
    quickfort commands you have to run.

The single most effective way to make your blueprint sets easier to use is to
group them with `meta blueprints <quickfort-meta>`. For the Dreamfort set of
blueprints, each logical "step" generally takes more than one blueprint. For
example, setting up pastures with a ``#zone`` blueprint, placing starting
stockpiles with a #place blueprint, building starting workshops with a
``#build`` blueprint, and configuring the stockpiles with a ``#query`` blueprint
can all be done at once. Bundling blueprints like this reduced the number of
steps in Dreamfort from 47 to 24, and it also made it much clearer to see which
blueprints can be applied at once without unpausing the game. Check out
dreamfort_surface's "`meta
<https://docs.google.com/spreadsheets/d/
1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit#gid=972927200>`__" sheet to
see how much meta blueprints can simplify your life.

Note that one of the ``#meta`` blueprints just has one line. In this case, the
``#meta`` blueprint isn't strictly necessary. The referenced blueprint could
just be applied directly. However, quickfort lists blueprints in the order that
it reads them, and we chose to make a one-blueprint meta blueprint to ensure all
the steps appear in order in the quickfort list output.

By the way, you can define `as many blueprints as you want
<quickfort-packaging>` on one sheet, but multi-blueprint sheets are
especially useful when writing meta blueprints. It's like having a bird's eye
view of your entire plan in one sheet.

.. topic:: Tip

    Keep the blueprint list uncluttered with hidden() markers.

If a blueprint is bundled into a meta blueprint, it does not need to appear in
the quickfort list output, since you won't be running it directly. Add a
`hidden() marker <quickfort-hidden>` to those blueprints to keep the list
output tidy. You can still access hidden blueprints with ``quickfort list
--hidden`` if you need to -- for example to reapply a partially completed #build
blueprint -- but now they won’t clutter up the normal blueprint list.

.. topic:: Tip

    Name your blueprints with a common prefix so you can find them easily.

This goes for both the file name and the `modeline label() <quickfort-label>`.
Searching and filtering is implemented for both the
``quickfort list`` command and the quickfort interactive dialog. If you give
related blueprints a common prefix, it makes it easy to set the filters to
display just the blueprints that you're interested in. If you have a lot of
blueprints, this can save you a lot of time. Dreamfort, of course, uses the
"dreamfort" prefix for the files and sequence names for the labels, like
"surface1", "surface2", "farming1", etc. So if I’m in the middle of applying the
surface blueprints, I’d set the filter to ``dreamfort surface`` to just display
the relevant blueprints.

.. topic:: Tip

    Add descriptive comments that remind you what the blueprint contains.

If you've been away from Dwarf Fortress for a while, it's easy to forget what
your blueprints actually do. Make use of `modeline comments
<quickfort-modeline>` so your descriptions are visible in the blueprint list.
If you use meta blueprints, all your comments can be conveniently edited on one
sheet, like in surface's meta sheet.

.. topic:: Tip

    Use message() markers to remind yourself what to do next.

`Messages <quickfort-message>` are displayed after a blueprint is applied. Good
things to include in messages are:

* The name of the next blueprint to apply and when to run it
* Whether quickfort orders should be run for an upcoming step
* Any manual actions that have to happen, like assigning minecarts to hauling
  routes or pasturing animals after creating zones

These things are just too easy to forget. Adding a message() can save you from
time-wasting mistakes. Note that message() markers can still appear on the
hidden() blueprints, and they'll still get shown when the blueprint is run via
the ``#meta`` blueprint. For an example of this, check out the `zones sheet
<https://docs.google.com/spreadsheets/d/
1vlxOuDOTsjsZ5W45Ri1kJKgp3waFo8r505LfZVg5wkU/edit#gid=1226136256>`__ where the
pastures are defined.

The farming_ level: fun with stockpiles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _farming: https://docs.google.com/spreadsheets/d/1iuj807iGVk6vsfYY4j52v9_-wsszA1AnFqoxeoehByg/edit?usp=sharing

It is usually convenient to store closely associated blueprints in the same
spreadsheet. The farming level is very closely tied to the surface because the
miasma vents have to perfectly line up. However, surface is a separate z-level
and, more importantly, already has many many blueprints, so farming is split
into a separate file.

.. topic:: Tip

    Automate stockpile chains when you can, and write message() reminders when
    you can't.

The farming level starts doing interesting things with query blueprints and
stockpiles. Note the `careful customization
<https://docs.google.com/spreadsheets/d/1iuj807iGVk6vsfYY4j52v9_-
wsszA1AnFqoxeoehByg/edit#gid=486506218>`__ of the food stockpiles and the
stockpile chains set up with the ``give*`` aliases. This is so when multiple
stockpiles can hold the same item, the largest can keep the smaller ones filled.
If you have multiple stockpiles holding the same type on different z-levels,
though, this can be tricky to set up with a blueprint. Here, the jugs and pots
stockpiles must be manually linked to the quantum stockpile on the industry
level, since we can't know beforehand how many z-levels away that is. Note how
we call that out in the query blueprint's message().

.. topic:: Tip

    Use aliases to set up hauling routes and quantum stockpiles.

Hauling routes are notoriously fiddly to set up, but they can be automated with
blueprints. Check out the Southern area of the ``#place`` and ``#query``
blueprints for how the quantum garbage dump is configured.

Note that aliases that must be applied in a particular order must appear in the
same cell. Otherwise there are no guarantees for which cell will be processed
first. For example, look at the track stop cells in the ``#query`` blueprint for
how the hauling routes are given names.

The industry_ level: when not to use aliases
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _industry: https://docs.google.com/spreadsheets/d/1gvTJxxRxZ5V4vXkqwhL-qlr_lXCNt8176TK14m4kSOU/edit?usp=sharing

The industry level is densely packed and has more complicated examples of
stockpile configurations and quantum dumps. However, what I'd like to call out
are the key sequences that are *not* in aliases.

.. topic:: Tip

     Don't use aliases for ad-hoc cursor movements.

It may be tempting to put all query blueprint key sequences into aliases to make
them easier to edit, keep them all in one place, and make them reusable, but
some key sequences just aren't very valuable as aliases.

`Check out <https://docs.google.com/spreadsheets/d/1gvTJxxRxZ5V4vXkqwhL-
qlr_lXCNt8176TK14m4kSOU/edit#gid=787640554>`__ the Eastern (goods) and Northern
(stone and gems) quantum stockpiles -- cells I19 and R10. They give to the
jeweler's workshop to prevent the jeweler from using the gems held in reserve
for strange moods. The keys are not aliased since they're dependent on the
relative positions of the tiles where they are interpreted, which is easiest to
see in the blueprint itself. Also, if you move the workshop, it's easier to fix
the stockpile link right there in the blueprint instead of editing the separate
aliases.txt file.

The services_ level: handling multi-level dig blueprints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _services: https://docs.google.com/spreadsheets/d/1IBy6_pGEe6WSBCLukDz_5I-4vi_mpHuJJyOp2j6SJlY/edit?usp=sharing

Services is a multi-level blueprint that includes a well cistern beneath the
main level. Unwanted ramps caused by channeling are an annoyance, but we can
avoid getting a ramp at the bottom of the cistern with careful use of `dig
priorities <quickfort-dig-priorities>`.

.. topic:: Tip

    Use dig priorities to control ramp creation.

We can `ensure <https://docs.google.com/spreadsheets/d/1IBy6_pGEe6WSBCLukDz_5I-
4vi_mpHuJJyOp2j6SJlY/edit#gid=962076234>`__ the bottom level is carved out
before the layer above is channelled by assigning the channel designations lower
priorities (row 76). This is easy to do here because it's just one tile and
there is no chance of cave-in. We could have used this technique on the farming
level for the miasma vents instead of requiring that the channels be dug before
the farming level is dug, but that would have been much more fiddly for the
larger areas.

The alternative is just to have a follow-up blueprint that removes any undesired
ramps. Using dig priorities to avoid the issue in the first place can be
cleaner, though.

The guildhall_ level: avoiding smoothing issues
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _guildhall: https://docs.google.com/spreadsheets/d/1wwKcOpEW-v_kyEnFyXS0FTjvLwJsyWbCUmEGaXWxJyU/edit?usp=sharing

The goal of this level is to provide rooms for locations like guildhalls,
libraries, and temples. The value of these rooms is very important, so we are
likely to smooth and engrave everything. To smooth or engrave a wall tile, a
dwarf has to be adjacent to it, and since some furniture, like statues, block
dwarves from entering a tile, where you put them affects what you can access.

.. topic:: Tip

    Don't put statues in corners unless you want to smooth everything first.

In the guildhall level, the statues are placed so as not to block any wall
corners. This gives the player freedom for choosing when to smooth. If statues
block wall segments, it forces the player to smooth before building the statues,
or else they have to mess with temporarily removing statues to smooth the walls
behind them.

The beds_ levels: multi level meta blueprints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _beds: https://docs.google.com/spreadsheets/d/1QNHORq6YmYfuVVMP5yGAFCQluary_JbgZ-UXACqKs9g/edit?usp=sharing

The suites and apartments blueprints are straightforward. The only fancy bit
here is the meta blueprint, which brings us to our final tip:

.. topic:: Tip

    Use meta blueprints to lay out multiple adjacent levels.

We couldn't use this technique for the entire fortress since there is often an
aquifer between the farming and industry levels, and we can't know beforehand
how many z-levels we need to skip. Here, though, we can at least provide the
useful shortcut of designating all apartment levels at once. See the meta
blueprint for how it applies the apartments on six z-levels using ``#>`` between
apartment blueprint references.

That's it! I hope this guide was useful to you. Please leave feedback on the
forums if you have ideas on how this guide (or the dreamfort blueprints) can be
improved!

Links
-----

**Quickfort links:**

-  `Quickfort command reference <quickfort>`
-  :forums:`Quickfort forum thread <176889>`
-  :source:`Quickfort blueprints library <data/blueprints/library>`
-  :issue:`DFHack issue tracker <>`
-  :source:scripts:`Quickfort source code <internal/quickfort>`

**Related tools:**

-  DFHack's `blueprint plugin <blueprint>` can generate blueprints from actual
   DF maps.
-  `Python Quickfort <http://joelpt.net/quickfort>`__ is the previous,
   Python-based implementation that DFHack's quickfort script was inspired by.
