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
``orders recheck [this]``
    Sets the status to ``Checking`` (from ``Active``) for all work orders that
    have conditions that can be re-checked. If the "this" option is passed,
    only sets the status for the workorder whose condition details page is
    open. This makes the manager reevaluate its conditions. This is especially
    useful for an order that had its conditions met when it was started, but
    the requisite items have since disappeared and the workorder is now
    generating job cancellation spam.
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

Fort-wide work orders screen
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Orders plugin functionality is directly available via an `overlay` widget when
the fort-wide work orders screen is open. There are hotkeys assigned to export,
import, sort, clear, and recheck conditions. You can also click on the hotkey
hints as if they were buttons. Clearing will ask for confirmation before acting.

When you open the conditions screen for a manager order, there is also a small
overlay that allows you to recheck conditions for just that order. This is
useful for when the conditions were true when the order started, but they have
become false and now you're just getting repeated cancellation spam as the
order cannot be fulfilled.

Workshop Workers tab
~~~~~~~~~~~~~~~~~~~~

For workshops that do *not* have a workshop master assigned, there is a slider
you can use to restrict the units that perform jobs at that workshop by their
skill level.

Due to space constraints, some skill levels are combined with the adjacent
higher rank on the slider:

- "Competent" includes "Adequate" workers
- "Proficient" includes "Skilled" workers
- "Expert" includes "Adept" workers
- "Accomplished" includes "Professional" workers
- "Master" includes "Great" workers
- "Grand Master" includes "High Master" workers

Finally, a list is shown for workshops that service manager orders of multiple
labor types. You can toggle the listed labors so the workshop only accepts
general work orders that match the enabled labors (the list of allowed labors
is different for every workshop).

For example, by default, all weapon, armor, and blacksmithing general manager
orders get sent to all forges that can take general work orders. With labor
restrictions, you can designate specific forges to handle just weapons, just
armor, or just metalsmithing. Then, you can assign appropriate legendary
masters to each forge, and they will only receive orders for appropriate
products.

Simiarly, you can set up Craftsdwarf's workshops to specialize in stone, wood,
or bone.

Regardless of the labor restriction settings, you can manually assign any task
to the workshop and it will still be completed. The labor restrictions only
apply to general manager work orders scheduled from the fort-wide work orders
screen.

Veteran players may remember these overlays as vanilla features in pre-v50 Dwarf
Fortress. This is actually still the case. The DFHack overlay simply provides a
UI for the vanilla feature hiding beneath the surface.

If you want to change where the overlay panels appear, you can move them with
`gui/overlay`.

The orders library
------------------

DFHack comes with a library of useful manager orders that are ready for import:

:source:`library/basic <data/orders/basic.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders handles basic fort necessities:

- prepared meals and food products (and by-products like oil)
- booze/mead
- thread/cloth/dye
- pots/bins/jugs/buckets/mugs
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

Note that the jugs are specifically made out of wood. This is so, as long as you don't may any other "Tools" out of wood, you can have a stockpile just for jugs by restricting a finished goods stockpile to only take wooden tools.

Armok's additional note: "shleggings? Yes,
`shleggings <https://youtu.be/bLN8cOcTjdo&t=3458>`__."

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

- leather backpacks/waterskins/quivers/armor
- silk cloaks
- bone/wooden bolts
- smelting for platinum, silver, steel, bronze, bismuth bronze, and copper (and
  their dependencies)
- bronze/bismuth bronze/copper bolts
- steel/silver/iron/bismuth bronze/bronze/copper weapons and armor,
  with checks to ensure only the best available materials are being used
- wooden shields (if metal isn't available)

If you set a stockpile to take weapons and armor of less than masterwork quality
and turn on `automelt` (like what `dreamfort` provides on its industry level),
these orders will automatically upgrade your military equipment to masterwork.
Make sure you have a lot of fuel (or magma forges and furnaces) before you turn
``automelt`` on, though!

This file should only be imported, of course, if you need to equip a military.

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
