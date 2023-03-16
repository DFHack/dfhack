stockpiles
==========

.. dfhack-tool::
    :summary: Import, export, or modify stockpile settings and features.
    :tags: fort design productivity stockpiles

If you are importing or exporting setting and don't want to specify a building
ID, select a stockpile in the UI before running the command.

Usage
-----

::

    stockpiles [status]
    stockpiles list [<search>]
    stockpiles export <name> [<options>]
    stockpiles import <name> [<options>]

Exported stockpile settings are saved in the ``dfhack-config/stockpiles``
folder, where you can view and delete them, if desired. Names can only
contain numbers, letters, periods, underscores, dashes, and spaces. If
the name has spaces, be sure to surround it with double quotes (:kbd:`"`).

The names of library settings files are all prefixed by the string ``library/``.
You can specify library files explicitly by including the prefix, or you can
just write the short name to use a player-exported file by that name if it
exists, and the library file if it doesn't.

Examples
--------

``stockpiles``
    Shows the list of all your stockpiles and some relevant statistics.
``stockpiles list``
    Shows the list of previously exported stockpile settings files, including
    the stockpile configuration library.
``stockpiles list plants``
    Shows the list of exported stockpile settings files that include the
    substring ``plants``.
``stockpiles import library/plants``
    Imports the library ``plants`` settings file into the currently selected
    stockpile.
``stockpiles import plants``
    Imports a player-exported settings file named ``plants``, or the library
    ``plants`` settings file if a player-exported file by that name doesn't
    exist.
``stockpiles export mysettings``
    Export the settings for the currently selected stockpile to a file named
    ``dfhack-config/stockpiles/mysettings.dfstock``.

Options
-------

``-i``, ``--include <comma separated list of elements to include>``
    When exporting, you can include this option to select only specific elements
    of the stockpile to record. If not specified, everything is included. When
    the file is later imported, only the included settings will be modified. The
    options are explained below in the next section.
``-d``, ``--disable``
    When importing, treat the settings in the file as elements to *remove** from
    the current stockpile configuration. Elements that are enabled in the file
    will be *disabled* on the stockpile. No other stockpile configuration will
    be changed.
``-e``, ``--enable``
    When importing, treat the settings in the file as elements to *add* to the
    current stockpile configuration. Elements that are enabled in the file will
    be enabled on the stockpile, but nothing currently enabled on the stockpile
    will be disabled.
``-s``, ``--stockpile <id>``
    Specify a specific stockpile ID instead of using the one currently selected
    in the UI.

Configuration elements
----------------------

The different configuration elements you can include in an exported settings file
are:

:general: Max bins, barrels, and wheelbarrows; whether the stockpile takes from
    links only; whether organic and/or inorganic materials are allowed.
:categories: The top-level categories of items that are enabled for the stockpile,
    like Ammo, Finished goods, or Stone.
:types: The elements below the categories, which include the sub-categories, the
    specific item types, and any toggles the category might have (like Prepared
    meals for the Food category).

.. _stockpiles-library:

The stockpiles settings library
-------------------------------

DFHack comes with a library of useful stockpile settings files that are ready
for import:

TODO: port alias library here
