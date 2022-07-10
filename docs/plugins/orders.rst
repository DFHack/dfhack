orders
======

A plugin for manipulating manager orders.

Subcommands:

:list: Shows the list of previously exported orders, including the orders library.
:export NAME: Exports the current list of manager orders to a file named ``dfhack-config/orders/NAME.json``.
:import NAME: Imports manager orders from a file named ``dfhack-config/orders/NAME.json``.
:clear: Deletes all manager orders in the current embark.
:sort: Sorts current manager orders by repeat frequency so daily orders don't
    prevent other orders from ever being completed: one-time orders first, then
    yearly, seasonally, monthly, then finally daily.

You can keep your orders automatically sorted by adding the following command to
your ``onMapLoad.init`` file::

    repeat -name orders-sort -time 1 -timeUnits days -command [ orders sort ]


The orders library
------------------

DFHack comes with a library of useful manager orders that are ready for import:

:source:`basic.json <data/orders/basic.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders handles basic fort necessities:

- prepared meals and food products (and by-products like oil)
- booze/mead
- thread/cloth/dye
- pots/jugs/buckets/mugs
- bags of leather, cloth, silk, and yarn
- crafts and totems from otherwise unusable by-products
- mechanisms/cages
- splints/crutches
- lye/soap
- ash/potash
- beds/wheelbarrows/minecarts
- scrolls

You should import it as soon as you have enough dwarves to perform the tasks.
Right after the first migration wave is usually a good time.

:source:`furnace.json <data/orders/furnace.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection creates basic items that require heat. It is separated out from
``basic.json`` to give players the opportunity to set up magma furnaces first in
order to save resources. It handles:

- charcoal (including smelting of bituminous coal and lignite)
- pearlash
- sand
- green/clear/crystal glass
- adamantine processing
- item melting

Orders are missing for plaster powder until DF :bug:`11803` is fixed.

:source:`military.json <data/orders/military.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection adds high-volume smelting jobs for military-grade metal ores and
produces weapons and armor:

- leather backpacks/waterskins/cloaks/quivers/armor
- bone/wooden bolts
- smelting for platinum, silver, steel, bronze, bismuth bronze, and copper (and
  their dependencies)
- bronze/bismuth bronze/copper bolts
- platinum/silver/steel/iron/bismuth bronze/bronze/copper weapons and armor,
  with checks to ensure only the best available materials are being used

If you set a stockpile to take weapons and armor of less than masterwork quality
and turn on `automelt` (like what `dreamfort` provides on its industry level),
these orders will automatically upgrade your military equipment to masterwork.
Make sure you have a lot of fuel (or magma forges and furnaces) before you turn
``automelt`` on, though!

This file should only be imported, of course, if you need to equip a military.

:source:`smelting.json <data/orders/smelting.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection adds smelting jobs for all ores. It includes handling the ores
already managed by ``military.json``, but has lower limits. This ensures all
ores will be covered if a player imports ``smelting`` but not ``military``, but
the higher-volume ``military`` orders will take priority if both are imported.

:source:`rockstock.json <data/orders/rockstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders keeps a small stock of all types of rock furniture.
This allows you to do ad-hoc furnishings of guildhalls, libraries, temples, or
other rooms with `buildingplan` and your masons will make sure there is always
stock on hand to fulfill the plans.

:source:`glassstock.json <data/orders/glassstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Similar to ``rockstock`` above, this collection keeps a small stock of all types
of glass furniture. If you have a functioning glass industry, this is more
sustainable than ``rockstock`` since you can never run out of sand. If you have
plenty of rock and just want the variety, you can import both ``rockstock`` and
``glassstock`` to get a mixture of rock and glass furnishings in your fort.

There are a few items that ``glassstock`` produces that ``rockstock`` does not,
since there are some items that can not be made out of rock, for example:

- tubes and corkscrews for building magma-safe screw pumps
- windows
- terrariums (as an alternative to wooden cages)
