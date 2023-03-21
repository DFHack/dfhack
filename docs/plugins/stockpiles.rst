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
``stockpiles import -m enable plants``
    Enables plants in the selected stockpile.
``stockpiles import -m disable category_food -f tallow``
    Disables all tallow in the selected food stockpile.
``stockpiles export mysettings``
    Export the settings for the currently selected stockpile to a file named
    ``dfhack-config/stockpiles/mysettings.dfstock``.
``stockpiles export mysettings -i categories,types``
    Export the stockpile category and item settings, but ignore the container
    and general settings. This allows you to import the configuration later
    without touching the container and general settings of the target
    stockpile.

Options
-------

``-s``, ``--stockpile <id>``
    Specify a specific stockpile ID instead of using the one currently selected
    in the UI.
``-i``, ``--include <comma separated list of elements to include>``
    When exporting, you can include this option to select only specific elements
    of the stockpile to record. If not specified, everything is included. When
    the file is later imported, only the included settings will be modified. The
    options are explained below in the next section.
``-m``, ``--mode (set|enable|disable)``
    When importing, choose the algorithm used to apply the settings. In ``set``
    mode (the default), the stockpile is cleared and the settings in the file
    are enabled. In ``enable`` mode, enabled settings in the file are *added*
    to the stockpile, but no other settings are changed. In ``disable`` mode,
    enabled settings in the file are *removed* from the current stockpile
    configuration, and nothing else is changed.
``-f``, ``--filter <search>[,<search>...]``
    When importing, only modify the settings that contain at least one of the
    given substrings.

Configuration elements
----------------------

The different configuration elements you can include in an exported settings
file are:

:containers: Max bins, max barrels, and num wheelbarrows.
:general: Whether the stockpile takes from links only and whether organic
    and/or inorganic materials are allowed.
:categories: The top-level categories of items that are enabled for the
    stockpile, like Ammo, Finished goods, or Stone.
:types: The elements below the categories, which include the sub-categories, the
    specific item types, and any toggles the category might have (like Prepared
    meals for the Food category).

.. _stockpiles-library:

The stockpiles settings library
-------------------------------

DFHack comes with a library of useful stockpile settings files that are ready
for import. If the stockpile configuration that you need isn't directly
represented, you can often use the ``enable`` and ``disable`` modes and/or
the ``filter`` option to transform an existing saved stockpile setting. Some
stockpile configurations can only be achieved with filters since the stockpile
lists are different for each world. For example, to disable all tallow in your
main food stockpile, you'd run this command::

    stockpiles import category_food -m disable -f tallow

Top-level categories
~~~~~~~~~~~~~~~~~~~~

Each stockpile category has a file that allows you to enable or disable the
entire category, or with a filter, any matchable subset thereof::

    category_ammo
    category_animals
    category_armor
    category_bars_blocks
    category_cloth
    category_coins
    category_corpses
    category_finished_goods
    category_food
    category_furniture
    category_gems
    category_leather
    category_refuse
    category_sheets
    category_stone
    category_weapons
    category_wood

For many of the categories, there are also subcategory prefixes that you can
match with filters and convenient pre-made settings files that manipulate
interesting category subsets.

Ammo stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    type/
    mats/
    other/
    core/
    total/

Convenience settings files::

    bolts
    metalammo
    boneammo
    woodammo

Example commands for a stockpile of metal bolts::

    stockpiles import category_ammo
    stockpiles import -m disable -f other/ category_ammo
    stockpiles import -m disable -f type/ category_ammo
    stockpiles import -m enable bolts

Animal stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

traps
cages

Armor stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

metalarmor
otherarmor
ironarmor
bronzearmor
copperarmor
steelarmor
masterworkarmor
artifactarmor
usablearmor
unusablearmor


Bar stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~

bars
metalbars
ironbars
pigironbars
steelbars
otherbars
coal
potash
ash
pearlash
soap
blocks

Cloth stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

thread
adamantinethread
cloth
adamantinecloth

Notes:

* ``thread`` and ``cloth`` refers to all materials that are not adamantine.

Finished goods stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

stonetools
woodtools
crafts
goblets
toys
masterworkfinishedgoods
artifactfinishedgoods

Food stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~

preparedmeals
unpreparedfish
plants
booze
seeds
dye
miscliquid
wax

Furniture stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

pots
bags
buckets
sand
masterworkfurniture
artifactfurniture

* Because of the limitations of Dwarf Fortress, ``bags`` cannot distinguish
  between empty bags and bags filled with gypsum powder.

Gem stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~

roughgems
roughglass
cutgems
cutglass
cutstone

Refuse stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

===========  ==================  ==================
Exclusive    Forbid              Permit
===========  ==================  ==================
corpses      forbidcorpses       permitcorpses
skulls       forbidskulls        permitskulls
bones        forbidbones         permitbones
shells       forbidshells        permitshells
teeth        forbidteeth         permitteeth
horns        forbidhorns         permithorns
hair         forbidhair          permithair
craftrefuse  forbidcraftrefuse   permitcraftrefuse
===========  ==================  ==================

Notes:

* ``usablehair`` Only hair and wool that can make usable clothing is included,
  i.e. from sheep, llamas, alpacas, and trolls.
* ``craftrefuse`` includes everything a craftsdwarf or tailor can use: skulls,
  bones, shells, teeth, horns, and "usable" hair/wool (defined above).

rawhides
tannedhides
usablehair

You can get a stockpile of usable refuse with the following set of commands::

    stockpiles import category_refuse -m enable -f skulls
    stockpiles import category_refuse -m enable -f bones
    stockpiles import category_refuse -m enable -f shells
    stockpiles import category_refuse -m enable -f teeth
    stockpiles import category_refuse -m enable -f horns
    stockpiles import usablehair -m enable

Stone stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

metalore
ironore
economic
flux
plasterproducing
coalproducing
otherstone
bauxite
clay

Weapon stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

weapons
metalweapons
stoneweapons
otherweapons
trapcomponents
ironweapons
silverweapons
bronzeweapons
copperweapons
steelweapons
platinumweapons
adamantineweapons
masterworkweapons
artifactweapons
usableweapons
unusableweapons
