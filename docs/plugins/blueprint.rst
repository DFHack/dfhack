blueprint
=========
The ``blueprint`` command exports the structure of a portion of your fortress in
a blueprint file that you (or anyone else) can later play back with `quickfort`.

Blueprints are ``.csv`` or ``.xlsx`` files created in the ``blueprints``
subdirectory of your DF folder. The map area to turn into a blueprint is either
selected interactively with the ``blueprint gui`` command or, if the GUI is not
used, starts at the active cursor location and extends right and down for the
requested width and height.

**Usage:**

    ``blueprint <width> <height> [<depth>] [<name> [<phases>]] [<options>]``

    ``blueprint gui [<name> [<phases>]] [<options>]``

**Examples:**

``blueprint gui``
    Runs `gui/blueprint`, the interactive frontend, where all configuration for
    a ``blueprint`` command can be set visually and interactively.

``blueprint 30 40 bedrooms``
    Generates blueprints for an area 30 tiles wide by 40 tiles tall, starting
    from the active cursor on the current z-level. Blueprints are written
    sequentially to ``bedrooms.csv`` in the ``blueprints`` directory.

``blueprint 30 40 bedrooms dig --cursor 108,100,150``
    Generates only the ``#dig`` blueprint in the ``bedrooms.csv`` file, and
    the start of the blueprint area is set to a specific value instead of using
    the in-game cursor position.

**Positional Parameters:**

:``width``:   Width of the area (in tiles) to translate.
:``height``:  Height of the area (in tiles) to translate.
:``depth``:   Number of z-levels to translate. Positive numbers go *up* from the
    cursor and negative numbers go *down*. Defaults to 1 if not specified,
    indicating that the blueprint should only include the current z-level.
:``name``:    Base name for blueprint files created in the ``blueprints``
    directory. If no name is specified, "blueprint" is used by default. The
    string must contain some characters other than numbers so the name won't be
    confused with the optional ``depth`` parameter.

**Phases:**

If you want to generate blueprints only for specific phases, add their names to
the commandline, anywhere after the blueprint base name. You can list multiple
phases; just separate them with a space.

:``dig``:    Generate quickfort ``#dig`` blueprints for digging natural stone.
:``carve``:  Generate quickfort ``#dig`` blueprints for smoothing and carving.
:``build``:  Generate quickfort ``#build`` blueprints for constructions and
    buildings.
:``place``:  Generate quickfort ``#place`` blueprints for placing stockpiles.
:``zone``:   Generate quickfort ``#zone`` blueprints for designating zones.
:``query``:  Generate quickfort ``#query`` blueprints for configuring rooms.

If no phases are specified, phases are autodetected. For example, a ``#place``
blueprint will be created only if there are stockpiles in the blueprint area.

**Options:**

``-c``, ``--cursor <x>,<y>,<z>``:
    Use the specified map coordinates instead of the current cursor position for
    the upper left corner of the blueprint range. If this option is specified,
    then an active game map cursor is not necessary.
``-e``, ``--engrave``:
    Record engravings in the ``carve`` phase. If this option is not specified,
    engravings are ignored.
``-f``, ``--format <format>``:
    Select the output format of the generated files. See the ``Output formats``
    section below for options. If not specified, the output format defaults to
    "minimal", which will produce a small, fast ``.csv`` file.
``-h``, ``--help``:
    Show command help text.
``-s``, ``--playback-start <x>,<y>,<comment>``:
    Specify the column and row offsets (relative to the upper-left corner of the
    blueprint, which is ``1,1``) where the player should put the cursor when the
    blueprint is played back with `quickfort`, in
    `quickfort start marker <quickfort-start>` format, for example:
    ``10,10,central stairs``. If there is a space in the comment, you will need
    to surround the parameter string in double quotes: ``"-s10,10,central stairs"`` or
    ``--playback-start "10,10,central stairs"`` or
    ``"--playback-start=10,10,central stairs"``.
``-t``, ``--splitby <strategy>``:
    Split blueprints into multiple files. See the ``Splitting output into
    multiple files`` section below for details. If not specified, defaults to
    "none", which will create a standard quickfort
    `multi-blueprint <quickfort-packaging>` file.

**Output formats:**

Here are the values that can be passed to the ``--format`` flag:

:``minimal``:
    Creates ``.csv`` files with minimal file size that are fast to read and
    write. This is the default.
:``pretty``:
    Makes the blueprints in the ``.csv`` files easier to read and edit with a text
    editor by adding extra spacing and alignment markers.

**Splitting output into multiple files:**

The ``--splitby`` flag can take any of the following values:

:``none``:
    Writes all blueprints into a single file. This is the standard format for
    quickfort fortress blueprint bundles and is the default.
:``phase``:
    Creates a separate file for each phase.
