
[//]: # (The online version of this manual, which may be easier to read, is)
[//]: # (at https://github.com/DFHack/dfhack/tree/develop/data/blueprints)

[//]: # (Note to editors: don't word wrap -- it breaks the formatting on GitHub)

DFHack Quickfort User Manual
============================

DFHack Quickfort is a DFHack script that helps you build fortresses from "blueprint" .csv and .xlsx files. Many applications exist to edit these files, such as MS Excel and [Google Sheets](https://sheets.new). You can also build your plan "for real" in Dwarf Fortress, and then export your map using DFHack's [blueprint](https://docs.dfhack.org/en/stable/docs/Plugins.html#blueprint) plugin. Most layout and building-oriented DF commands are supported through the use of multiple files or spreadsheets, each describing a different phase of DF construction: designation, building, placing stockpiles, and setting configuration.

The original idea and 1.0 codebase came from [Valdemar's](https://dwarffortresswiki.org/index.php/User:Valdemar) auto-designation macro. Joel Thornton (joelpt) reimplemented the core logic in Python and extended its functionality with [Quickfort 2.0](https://github.com/joelpt/quickfort). This DFHack-native implementation, called "DFHack Quickfort" or just "quickfort", builds upon Quickfort 2.0's formats and features. DFHack Quickfort is written in Lua and interacts with Dwarf Fortress memory structures directly, allowing for instantaneous blueprint application, error checking and recovery, and many other advanced features.

This document focuses on DFHack Quickfort's capabilities and teaches players how to understand and build blueprint files.  Much of the text was originally written by Joel Thornton, reused here with his permission.

For those just looking to apply blueprints, check out the [quickfort command syntax](https://docs.dfhack.org/en/stable/docs/_auto/base.html#quickfort) in the DFHack Scripts documentation. There are many ready-to-use blueprints available in the `blueprints/library` subfolder in your DFHack installation. Browse them on your computer or [online](https://github.com/DFHack/dfhack/tree/develop/data/blueprints/library), or run `quickfort list -l` at the `DFHack#` prompt to list them, and then `quickfort run` to apply them to your fort!

See the [Links section](#links) for more information and online resources.


Table of Contents
-----------------

* [Features](#features)
* [Editing Blueprints](#editing-blueprints)
  * [Area expansion syntax](#area-expansion-syntax)
  * [Automatic area expansion](#automatic-area-expansion)
  * [Specifying a starting position](#specifying-a-starting-position)
  * [Multilevel blueprints](#multilevel-blueprints)
  * [Minecart tracks](#minecart-tracks)
* [Packaging a set of blueprints](#packaging-a-set-of-blueprints)
* [Troubleshooting](#troubleshooting)
* [Tips and tricks](#tips-and-tricks)
* [Links](#links)


Features
--------

* General
  * Manages complete blueprints to handle the four main phases of DF construction
  * Supports .csv and multi-worksheet .xlsx blueprint files
  * Near-instant application, even for very large and complex blueprints
  * Blueprints can span multiple z-levels
  * Undo functionality for dig, build, and place blueprints
  * Automatic cropping of blueprints so you don't get errors if the blueprint extends off the map
  * Can generate manager orders for everything required to apply build blueprints
  * Library of ready-to-use blueprints included
  * Verbose output mode for debugging blueprints
* Dig mode
  * Supports all types of designations, including dumping/forbidding items and setting traffic areas
  * Supports applying dig blueprints in marker mode
  * Handles carving arbitrarily complex minecart tracks, including tracks that cross other tracks
* Build and place modes
  * Supports stockpiles of all shapes, not just rectangular blocks
  * Configurable maximums for bins, barrels and wheelbarrows assigned to created stockpiles
  * Automatic expansion of building footprints to their minimum dimensions, so only the center tile of a multi-tile building needs to be recorded in the blueprint
  * Designates complete constructions at once, without having to wait for each tile to become supported before you can build it
  * Automatic splitting of stockpiles and buildings that exceed maximum dimension limits
  * Tile occupancy and validity checking so, for example, buildings that cannot be placed on a certain tile will simply be skipped instead of the blueprint failing to apply. Blueprints that are only partially applied for any reason (for example, you need to dig out some more tiles) can be safely reapplied to build the remaining buildings.
  * Relaxed rules for farm plot and road placement, allowing tiles that are separated by invalid tiles (e.g. stone tiles for farm plots) to be part of the same structure
* Query mode
  * Support sending arbitrary keystroke sequences to the UI -- configure *anything*
  * Supports aliases to automate frequent keystroke combos
  * Supports including aliases in other aliases, and repeating key sequences a specified number of times
  * Includes a library of pre-made and tested aliases to automate most common tasks, such as configuring stockpiles for important item types or creating hauling routes for quantum stockpiles.
  * Skips sending key sequences when the cursor is over a tile that does not have a stockpile or building
  * Instant halting of query blueprint application when keystroke errors are detected, such as when a key sequence leaves us stuck in a submenu


Editing Blueprints
------------------

The format of Quickfort-compatible blueprint files is straightforward.

It is recommended to use a spreadsheet editor such as Excel, [Google Sheets](https://sheets.new), or [LibreOffice](https://www.libreoffice.org) to edit these files, but any text editor will do.

The first line (or upper-left cell) of the spreadsheet should look like this:

    #dig This is a decription.

The keyword "dig" tells Quickfort we are going to be using the Designations menu in DF. The following "mode" keywords are understood:

    dig     Designations menu (d)
    build   Build menu (b)
    place   Place stockpiles menu (p)
    query   Set building tasks/prefs menu (q)

Optionally following this keyword and a space, you may enter a comment. This comment will appear in the output of `quickfort list` when run from the `DFHack#` prompt. You can use this space for explanations, attribution, etc.

Below this line begin entering the keys you want sent in each cell. For example, we could dig out a 4x4 room like so (spaces are used as column separators here for clarity, but a real .csv file would have commas):

    #dig
    d d d d #
    d d d d #
    d d d d #
    d d d d #
    # # # # #

Note the # symbols at the right end of each row and below the last row. These are completely optional, but can be helpful when visualizing the layout.

Once the dwarves have that dug out, let's build a walled-in bedroom within our dug-out area:

    #build
    Cw Cw Cw Cw #
    Cw b  h  Cw #
    Cw       Cw #
    Cw Cw    Cw #
    #  #  #  #  #

Note my generosity - in addition to the bed (b) I've built a chest (h) here for the dwarf as well. Note that you must use the full series of keys needed to build something in each cell, e.g. 'Cw' enters DF's constructions submenu (C) and selects walls (w).

I'd also like to place a booze stockpile in the 2 unoccupied tiles in the room.

    #place Place a food stockpile
    ` ` ` ` #
    ` ` ` ` #
    ` f(2x1)#
    ` ` ` ` #
    # # # # #

This illustration may be a little hard to understand. The f(2x1) is in column 2, row 3. All the other cells are empty. QF considers both "`" (backtick -- the character under the tilde) and "~" (tilde) characters within cells to be empty cells; this can help with multilayer or fortress-wide blueprint layouts as 'chalk lines'.

With f(2x1), we've asked QF to place a food stockpile 2 units wide by 1 high unit. Note that the f(2x1) syntax isn't actually necessary here; we could have just used:

    #place Place a food stockpile
    ` ` ` ` #
    ` ` ` ` #
    ` f f ` #
    ` ` ` ` #
    # # # # #

QF is smart enough to recognize this as a 2x1 food stockpile, and creates it as such rather than as two 1x1 food stockpiles. Quickfort recognizes any connected region of identical designations as a single stockpile. The tiles can be connected orthogonally or diagonally, just as long as they are touching somehow.

Lastly, let's turn the bed into a bedroom and set the food stockpile to hold only booze.

    #query
    ` ` ` ` #
    ` r&  ` #
    ` booze #
    ` ` ` ` #
    # # # # #

In column 2, row 2 we have "r&". This sends the "r" key to DF when the cursor is over the bed, causing us to 'make room' and "&", which is a special symbol that expands to "{Enter}", to indicate that we're done.

In column 2, row 3 we have "booze". This is one of many alias keywords defined in the included [baseline aliases file](https://github.com/DFHack/dfhack/tree/develop/data/quickfort/aliases-common.txt). This particular alias sets a food stockpile to carry booze only. It sends the keys needed to navigate DF's stockpile settings menu, and then sends an Escape character ("^" or "{ESC}") to exit back to the map. It is important to exit out of any menus that you enter while in query mode so that the cursor can move to the next tile when it is done configuring the current tile.

Check out the included [blueprint library](https://github.com/DFHack/dfhack/tree/develop/data/blueprints/library) to see many more examples. Read the baseline aliases file for helpful pre-packaged aliases, or create your own in [dfhack-config/quickfort/aliases.txt](https://github.com/DFHack/dfhack/tree/develop/dfhack-config/quickfort/aliases.txt) in your DFHack installation.


Area expansion syntax
---------------------

In Quickfort, the following blueprints are equivalent:

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

The second example uses Quickfort's "area expansion syntax", which takes the form:

    cmds(WxH)

In Quickfort the above two examples of specifying a contiguous 3x3 area produce identical output: a single 3x3 designation will be performed, rather than nine 1x1 designations as the first example might suggest.

Area expansion syntax can only specify rectangular areas. If you want to create extent-based structures (e.g. farm plots or stockpiles) in different shapes, use the first format above. For example:

    #place L shaped food stockpile
    f f ` ` #
    f f ` ` #
    f f f f #
    f f f f #
    # # # # #

Area expansion syntax also sets boundaries, which can be useful if you want adjacent, but separate, stockpiles of the same type:

    #place Two touching but separate food stockpiles
    f(4x2)  #
    ~ ~ ~ ~ #
    f(4x2)  #
    ~ ~ ~ ~ #
    # # # # #

As mentioned previously, "~" characters are ignored as comment characters and can be used for visualizing the blueprint layout. The blueprint can be equivalently written as:

    #place Two touching but separate food stockpiles
    f(4x2)  #
    ~ ~ ~ ~ #
    f f f f #
    f f f f #
    # # # # #

since the area expansion syntax of the upper stockpile prevents it from combining with the lower, freeform syntax stockpile.

Area expansion syntax can also be used for buildings which have an adjustable size, like bridges. The following blueprints are equivalent:

    #build a 4x2 bridge from row 1, col 1
    ga(4x2)  `  #
    `  `  `  `  #
    #  #  #  #  #

    #build a 4x2 bridge from row 1, col 1
    ga ga ga ga #
    ga ga ga ga #
    #  #  #  #  #


Automatic area expansion
------------------------

Buildings larger than 1x1, like workshops, can be represented in any of three ways. You can designate just their center tile with empty cells around it to leave room for the footprint, like this:

    #build a mason workshop in row 2, col 2 that will occupy the 3x3 area
    `  `  `  #
    `  wm `  #
    `  `  `  #
    #  #  #  #

Or you can fill out the entire footprint like this:

    #build a mason workshop
    wm wm wm #
    wm wm wm #
    wm wm wm #
    #  #  #  #

This format may be verbose for regular workshops, but it can be very helpful for laying out structures like screw pump towers and waterwheels, whose "center point" can be non-obvious.

Finally, you can use area expansion syntax to represent the workshop:

    #build a mason workshop
    wm(3x3)  #
    `  `  `  #
    `  `  `  #
    #  #  #  #

This style can be convenient for laying out multiple buildings of the same type. If you are building a large-scale block factory, for example, this will create 20 mason workshops all in a row:

    #build line of 20 mason workshops
    wm(60x3) #

Quickfort will intelligently break large areas of the same designation into appropriately-sized chunks.


Specifying a starting position
------------------------------

You can optionally specify a cursor starting position for a particular blueprint, simplifying the task of blueprint alignment. This can be helpful for blueprints that are based on a central staircase, for example.

To specify a cursor starting position, use the following modified format for the header line of your blueprint:

    #mode start(X;Y;STARTCOMMENT) comment

where X and Y specify the starting cursor position (1;1 is the top left cell) and STARTCOMMENT (optional) is information about where to position the cursor. This information appears in the `quickfort list` output.

A couple examples:

    #dig start(3; 3; Center tile of a 5-tile square) Regular blueprint comment
    #build start(10;15)


Multilevel blueprints
---------------------

Multilevel blueprints are accommodated by separating Z-levels of the blueprint with #> (go down one z-level) or #< (go up one z-level) at the end of each floor.

    #dig Stairs leading down to a small room below
    j  `  `  #
    `  `  `  #
    `  `  `  #
    #> #  #  #
    u  d  d  #
    d  d  d  #
    d  d  d  #
    #  #  #  #


Minecart tracks
---------------

There are two ways to produce minecart tracks, and they are handled very differently by the game. You can carve them into hard natural floors or you can construct them out of building materials. Constructed tracks are conceptually simpler, so we'll start with them.

### Constructed Tracks ###

Quickfort supports the designation of track stops and rollers through the normal mechanisms: a #build blueprint with `CS` and some number of 'd' and 'a' characters (for selecting dump direction and friction) in a cell designates a track stop and a #build blueprint with `Mr` and some number of 's' and 'q' characters (for direction and speed) designates a roller. This can get confusing very quickly and is very difficult to read in a blueprint. Constructed track segments don't even have keys associated with them at all!

To solve this problem, Quickfort provides the following keywords for use in build blueprints:

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

As an example, you can create an E-W track with stops at each end that dump to their outside directions with the following blueprint:

    #build Example track
    trackstopW trackEW trackEW trackEW trackstopE

Note that the **only** way to build track and track/ramp segments is with the keywords. The UI method of using "+" and "-" keys to select the track type from a list does not work since DFHack Quickfort doesn't actually send keys to the UI in order to build buildings. The text in your spreadsheet cells is mapped directly into DFHack API calls. Only query-mode blueprints still send actual keycodes to the UI.

### Carved Tracks ###

In the game, you carve a minecart track by specifying a beginning and ending tile and the game "adds" the designation to the tiles. You cannot designate single tiles. For example to carve two track segments that cross each other, you might use the cursor to designate a line of three vertical tiles like this:

    `  start here  `  #
    `  `           `  #
    `  end here    `  #

Then to carve the cross, you'd do a horizonal segment:

    `           `  `         #
    start here  `  end here  #
    `           `  `         #

This will result in a carved track that would be equivalent to a constructed track of the form:

    #build
    `       trackS     `       #
    trackE  trackNSEW  trackW  #
    `       trackN     `       #
    #       #          #       #

To carve this same track with a dig blueprint, you'd use area expansion syntax with a height or width of 1 to indicate the segments to designate:

    #dig
    `       T(1x3)  `  #
    T(3x1)  `       `  #
    `       `       `  #
    #       #       #  #

"But wait!", I can hear you say, "How do you designate a track corner that opens to the South and East? You can't put both T(1xH) and T(Wx1) in the same cell!" This is true, but you can specify both width and height, and for tracks, QF interprets it as an upper-left corner extending to the right W tiles and down H tiles. For example, to carve a track in a closed ring, you'd write:

    #dig
    T(3x3)  `  T(1x3)  #
    `       `  `       #
    T(3x1)  `  `       #
    #       #  #       #

Which would result in a carved track simliar to a constructed track of the form:

    #build
    trackSE  trackEW  trackSW  #
    trackNS  `        trackNS  #
    trackNE  trackEW  trackNW  #
    #        #        #        #


Packaging a set of blueprints
-----------------------------

A complete QF specification for a section of your fortress may contain 4 or more separate blueprints, one for each "phase" of construction (dig, build, place stockpiles, query building adjustments).

To manage all the separate blueprints, it is often convenient to keep related blueprints in a single .xlsx file, with each blueprint in a separate sheet within the file. Online spreadsheet applications like [Google Sheets](https://sheets.new) make it easy to work with multiple related blueprints, and, as a bonus, they retain any formatting you've set, like column sizes and coloring.

Of course, you could still choose to keep your blueprints in single-sheet .csv files and just give related blueprints similar names. The blueprint modes suggest a convenient naming scheme for .csv-based blueprint "stacks":

    bedroom.1.dig.csv
    bedroom.2.build.csv
    bedroom.3.place.csv
    bedroom.4.query.csv

But the naming is completely up to you, of course.


Tips and Tricks
---------------

* During blueprint application, especially query blueprints, don't click the mouse on the DF window or type any keys. They can change the state of the game while the blueprint is being applied, resulting in strange errors.

* After digging out an area, you may wish to smooth and/or engrave the area before starting the build phase, as dwarves may be unable to access walls or floors that are behind/under built objects.

* As of DF 0.34.x, it is no longer possible to build doors (d) at the same time that you build adjacent walls (Cw). Doors must now be built *after* walls are constructed for them to be next to. This does not affect the more common case where walls exist as a side-effect of having dug-out a room in a #dig blueprint.


Links
-----

### Quickfort links ###

* [Quickfort command syntax](https://docs.dfhack.org/en/stable/docs/_auto/base.html#quickfort)
* [Quickfort forum thread](http://www.bay12forums.com/smf/index.php?topic=176889.0)
* [Quickfort blueprints library](https://github.com/DFHack/dfhack/tree/develop/data/blueprints/library)
* [DFHack issue tracker](https://github.com/DFHack/dfhack/issues)
* [Quickfort source code](https://github.com/DFHack/scripts/tree/master/internal/quickfort)

### Related tools ###

* DFHack's [blueprint plugin](https://docs.dfhack.org/en/stable/docs/Plugins.html#blueprint) can generate blueprints from actual DF maps.
* [Python Quickfort](http://joelpt.net/quickfort) is the previous, Python-based implementation that DFHack's quickfort script was inspired by.
