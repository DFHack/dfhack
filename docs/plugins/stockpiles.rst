stockpiles
==========

.. dfhack-tool::
    :summary: Import, export, or modify stockpile settings.
    :tags: fort design productivity stockpiles

Commands act upon the stockpile selected in the UI unless another stockpile
identifier is specified on the commandline.

You can also specify hauling route stops to import and export "desired items"
configuration.

Usage
-----

::

    stockpiles [status]
    stockpiles list [<search>]
    stockpiles import <name> [<options>]
    stockpiles export <name> [<options>]

Exported settings are saved in the ``dfhack-config/stockpiles`` folder, where
you can view and delete them, if desired. Names can only contain numbers,
letters, periods, and underscores.

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
``stockpiles import -m disable cat_food -f tallow``
    Disables all tallow in the selected food stockpile.
``stockpiles export mysettings``
    Export the settings for the currently selected stockpile to a file named
    ``dfhack-config/stockpiles/mysettings.dfstock``.
``stockpiles export mysettings -i categories,types``
    Export the stockpile category and item settings, but ignore the container
    and general settings. This allows you to import the configuration later
    without touching the container and general settings of the target
    stockpile.
``stockpiles export mydumpersettings -r "Stone quantum"``
    Exports the "desired items" for the first stop of the "Stone quantum"
    hauling route.
``stockpiles export myroutesettings -r "Train porter,2"``
    Exports the "desired items" for the stop with id 2 (the second stop, unless
    you've previously deleted the original first stop and now this is the
    first) of the "Train porter" hauling route.

Options
-------

``-s``, ``--stockpile <name or id>``
    Specify a specific stockpile by name or internal ID instead of using the
    stockpile currently selected in the UI.
``-r``, ``--route <route name or id>[,<stop name or id>]``
    Specify a hauling route and route stop as the target for import/export
    instead of a stockpile. If not specified, the first route stop is targeted.
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

Overlay
-------

This plugin provides a panel that appears when you select a stockpile via an
`overlay` widget. You can use it to easily toggle `logistics` plugin features
like autotrade, automelt, or autotrain. There are also buttons along the top frame for:

- minimizing the panel (if it is in the way of the vanilla stockpile
  configuration widgets)
- showing help for the overlay widget in `gui/launcher` (this page)
- configuring advanced settings for the stockpile, such as whether automelt
  will melt masterworks

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

    stockpiles import cat_food -m disable -f tallow

Top-level categories
~~~~~~~~~~~~~~~~~~~~

Each stockpile category has a file that allows you to enable or disable the
entire category, or with a filter, any matchable subset thereof::

    cat_ammo
    cat_animals
    cat_armor
    cat_bars_blocks
    cat_cloth
    cat_coins
    cat_corpses
    cat_finished_goods
    cat_food
    cat_furniture
    cat_gems
    cat_leather
    cat_refuse
    cat_sheets
    cat_stone
    cat_weapons
    cat_wood

In addition, there are files for ``all``, which includes all categories except
refuse and corpses (mirroring the "all" configuration in-game), and
``everything``, which really includes all categories.

For many of the categories, there are also flags, subcategory prefixes, and
item properties that you can match with filters. In addition, there are
normally at least a few convenient pre-made settings files that manipulate
interesting category subsets.

Cross-category stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Settings files::

    artifacts
    masterworks

Example command for a meltable weapons stockpile::

    stockpiles import cat_weapons
    stockpiles import -m disable cat_weapons -f other/
    stockpiles import -m disable artifacts
    stockpiles import -m disable masterworks

Ammo stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    type/
    mats/
    other/
    core/
    total/

Settings files::

    bolts
    metalammo
    boneammo
    woodammo

Example commands for a stockpile of metal bolts::

    stockpiles import cat_ammo -f mats/,core/,total/
    stockpiles import -m enable bolts

Animal stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Flags::

    cages
    traps

Properties::

    tameable

Settings files::

    cages
    traps

Example commands for a stockpile of empty cages::

    stockpiles import cages

Or, using the flag for the same effect::

    stockpiles import cat_animals -f cages

Armor stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Flags and subcategory prefixes::

    nouse
    canuse
    body/
    head/
    feet/
    hands/
    legs/
    shield/
    mats/
    other/
    core/
    total/

Settings files::

    metalarmor
    otherarmor
    ironarmor
    bronzearmor
    copperarmor
    steelarmor
    usablearmor
    unusablearmor

Example commands for a stockpile of sub-masterwork meltable armor::

    stockpiles import cat_armor
    stockpiles import -m disable -f other/,core/mas,core/art cat_armor

Bar stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    mats/bars/
    other/bars/
    mats/blocks/
    other/blocks/

Settings files::

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

Example commands for a stockpile of blocks::

    stockpiles import blocks

Cloth stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    thread/silk/
    thread/plant/
    thread/yarn/
    thread/metal/
    cloth/silk/
    cloth/plant/
    cloth/yarn/
    cloth/metal/

Settings files::

    thread
    adamantinethread
    cloth
    adamantinecloth

Notes:

* ``thread`` and ``cloth`` settings files set all materials that are not
    adamantine.

Corpse stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Properties::

    tameable

Finished goods stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    type/
    mats/
    other/
    core/
    total/

Settings files::

    stonetools
    woodtools
    crafts
    goblets
    toys

Example commands for a toy stockpile::

    stockpiles import cat_finished_goods -f mats/,other/,core/,total/
    stockpiles import -m enable toys

Food stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~

Flags and subcategory prefixes::

    preparedmeals
    meat/
    fish/prepared/
    fish/unprepared/
    egg/
    plants/
    drink/plant/
    drink/animal/
    cheese/plant/
    cheese/animal/
    seeds/
    leaves/
    powder/plant/
    powder/animal/
    glob/
    liquid/plant/
    liquid/animal/
    liquid/misc/
    paste/
    pressed/

Settings files::

    preparedmeals
    unpreparedfish
    plants
    booze
    seeds
    dye
    miscliquid
    wax

Example commands for a kitchen ingredients stockpile::

    stockpiles import cat_food -f meat/,fish/prepared/,egg/,cheese/,leaves/,powder/,glob/,liquid/plant/,paste/,pressed/
    stockpiles import cat_food -m enable -f milk,royal_jelly
    stockpiles import dye -m disable
    stockpiles import cat_food -m disable -f tallow,thread,liquid/misc/

Furniture stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    type/
    mats/
    other/
    core/
    total/

Settings files::

    pots
    barrels
    bags
    buckets
    sand

* Because of the limitations of Dwarf Fortress, ``bags`` cannot distinguish
  between empty bags and bags filled with gypsum powder.

Example commands for a sand bag stockpile::

    stockpiles import cat_furniture
    stockpiles import cat_furniture -m disable -f type/
    stockpiles import sand -m enable

Gem stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    mats/rough/
    mats/cut/
    other/rough/
    other/cut/

Settings files::

    roughgems
    roughglass
    cutgems
    cutglass
    cutstone

Refuse stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Flags and subcategory prefixes::

    rawhide/fresh
    rawhide/rotten
    type/
    corpses/
    bodyparts/
    skulls/
    bones/
    hair/
    shells/
    teeth/
    horns/

Properties::

    tameable

Settings files::

    rawhides
    tannedhides
    usablehair

Notes:

* ``usablehair`` Only hair and wool that can make usable clothing is included,
  i.e. from sheep, llamas, alpacas, and trolls.

Example commands for a craftable refuse stockpile::

    stockpiles import cat_refuse -f skulls/,bones/,shells',teeth/,horns/
    stockpiles import usablehair -m enable

Sheet stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subcategory prefixes::

    paper/
    parchment/

Stone stockpile adjustments
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Settings files::

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

Flags and subcategory prefixes::

    nouse
    canuse
    type/weapon/
    type/trapcomp/
    mats/
    other/
    core/
    total/

Settings files::

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
    usableweapons
    unusableweapons

Example commands for a non-metallic trap components stockpile::

    stockpiles import cat_weapons
    stockpiles import cat_weapons -m disable -f type/weapon/
    stockpiles metalweapons -m disable
