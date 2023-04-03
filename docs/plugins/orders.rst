orders
======

.. dfhack-tool::
    :summary: Manage manager orders.
    :tags: fort productivity workorders

Usage
-----

``orders list``
    Shows the list of previously exported orders, including the orders library.
``orders export <name>``
    Saves all the current manager orders in a file.
``orders import <name>``
    Imports the specified manager orders. Note this adds to your current set of
    manager orders. It will not clear the orders that already exist.
``orders clear``
    Deletes all manager orders in the current embark.
``orders sort``
    Sorts current manager orders by repeat frequency so repeating orders don't
    prevent one-time orders from ever being completed. The sorting order is:
    one-time orders first, then yearly, seasonally, monthly, and finally, daily.

You can keep your orders automatically sorted by adding the following command to
your ``dfhack-config/init/onMapLoad.init`` file::

    repeat -name orders-sort -time 1 -timeUnits days -command [ orders sort ]

Exported orders are saved in the ``dfhack-config/orders`` directory, where you
can view, edit, and delete them, if desired.

Examples
--------

``orders export myorders``
    Export the current manager orders to a file named
    ``dfhack-config/orders/myorders.json``.
``orders import library/basic``
    Import manager orders from the library that keep your fort stocked with
    basic essentials.

Overlay
-------

Orders plugin functionality is directly available when the manager orders screen
is open via an `overlay` widget. There are hotkeys assigned to export, import,
sort, and clear. You can also click on the hotkey hints as if they were buttons.
Clearing will ask for confirmation before acting.

If you want to change where the overlay panel appears, you can move it via
`gui/overlay`. If you just need to get the overlay out of the way temporarily,
for example to read a long description of a historical figure when choosing a
subject for a statue, click on the small arrow in the upper right corner of the
overlay panel. Click on the arrow again to restore the panel.

The orders library
------------------

DFHack comes with a library of useful manager orders that are ready for import:

:source:`library/basic <data/orders/basic.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders handles basic fort necessities:

- prepared meals and food products (and by-products like oil)
- booze/mead
- thread/cloth/dye
- pots/jugs/buckets/mugs
- bags of leather, cloth, silk, and yarn
- crafts, totems, and shleggings from otherwise unusable by-products
- mechanisms/cages
- splints/crutches
- lye/soap
- ash/potash
- beds/wheelbarrows/minecarts
- scrolls

You should import it as soon as you have enough dwarves to perform the tasks.
Right after the first migration wave is usually a good time.

Armok's note: shleggings? Yes, `shleggings <https://youtu.be/bLN8cOcTjdo>`__.

:source:`library/furnace <data/orders/furnace.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection creates basic items that require heat. It is separated out from
``library/basic`` to give players the opportunity to set up magma furnaces first
in order to save resources. It handles:

- charcoal (including smelting of bituminous coal and lignite)
- pearlash
- sand
- green/clear/crystal glass
- adamantine processing
- item melting

Orders are missing for plaster powder until DF :bug:`11803` is fixed.

:source:`library/military <data/orders/military.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection adds high-volume smelting jobs for military-grade metal ores and
produces weapons and armor:

- leather backpacks/waterskins/cloaks/quivers/armor
- bone/wooden bolts
- smelting for platinum, silver, steel, bronze, bismuth bronze, and copper (and
  their dependencies)
- bronze/bismuth bronze/copper bolts
- silver/steel/iron/bismuth bronze/bronze/copper weapons and armor,
  with checks to ensure only the best available materials are being used

If you set a stockpile to take weapons and armor of less than masterwork quality
and turn on `automelt` (like what `dreamfort` provides on its industry level),
these orders will automatically upgrade your military equipment to masterwork.
Make sure you have a lot of fuel (or magma forges and furnaces) before you turn
``automelt`` on, though!

This file should only be imported, of course, if you need to equip a military.

:source:`library/military_include_artifact_materials <data/orders/military_include_artifact_materials.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As above, but this collection will also allow creation of platinum blunt weapons.
Normally these are only created by artifact moods, work orders can't be created
manually for them.

- platinum/silver/steel/iron/bismuth bronze/bronze/copper weapons and armor,
  with checks to ensure only the best available materials are being used

:source:`library/smelting <data/orders/smelting.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection adds smelting jobs for all ores. It includes handling the ores
already managed by ``library/military``, but has lower limits. This ensures all
ores will be covered if a player imports ``library/smelting`` but not
``library/military``, but the higher-volume ``library/military`` orders will
take priority if both are imported.

:source:`library/rockstock <data/orders/rockstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders keeps a small stock of all types of rock furniture.
This allows you to do ad-hoc furnishings of guildhalls, libraries, temples, or
other rooms with `buildingplan` and your masons will make sure there is always
stock on hand to fulfill the plans.

:source:`library/glassstock <data/orders/glassstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Similar to ``library/rockstock`` above, this collection keeps a small stock of
all types of glass furniture. If you have a functioning glass industry, this is
more sustainable than ``library/rockstock`` since you can never run out of sand.
If you have plenty of rock and just want the variety, you can import both
``library/rockstock`` and ``library/glassstock`` to get a mixture of rock and
glass furnishings in your fort.

There are a few items that ``library/glassstock`` produces that
``library/rockstock`` does not, since there are some items that can not be made
out of rock, for example:

- tubes and corkscrews for building magma-safe screw pumps
- windows
- terrariums (as an alternative to wooden cages)
