.. _config-examples-guide:
.. _dfhack-examples-guide:

DFHack Example Configuration File Index
=======================================

The :source:`hack/examples <data/examples>` folder contains ready-to-use
examples of various DFHack configuration files. You can use them by copying them
to appropriate folders where DFHack and its plugins can find them (details
below). You can use them unmodified, or you can customize them to better suit
your preferences.

The ``init/`` subfolder
-----------------------

The :source:`init/ <data/examples/init>` subfolder contains useful DFHack
`init-files` that you can copy into your main Dwarf Fortress folder -- the same
directory as ``dfhack.init``.

.. _onMapLoad-dreamfort-init:

:source:`onMapLoad_dreamfort.init <data/examples/init/onMapLoad_dreamfort.init>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the config file that comes with the `dreamfort` set of blueprints, but
it is useful (and customizable) for any fort. It includes the following config:

- Calls `ban-cooking` for items that have important alternate uses and should
  not be cooked. This configuration is only set when a fortress is first
  started, so later manual changes will not be overridden.
- Automates calling of various fort maintenance and `scripts-fix`, like
  `cleanowned` and `fix/stuckdoors`.
- Keeps your manager orders intelligently ordered with `orders` ``sort`` so no
  orders block other orders from ever getting completed.
- Periodically enqueues orders to shear and milk shearable and milkable pets.
- Sets up `autofarm` to grow 30 units of every crop, except for pig tails, which
  is set to 150 units to support the textile industry.
- Sets up `seedwatch` to keep 30 of every type of seed.
- Configures `prioritize` to automatically boost the priority of important and
  time-sensitive tasks that could otherwise get ignored in busy forts, like
  hauling food, tanning hides, storing items in vehicles, pulling levers, and
  removing constructions.
- Optimizes `autobutcher` settings for raising geese, alpacas, sheep, llamas,
  and pigs. Adds sensible defaults for all other animals, including dogs and
  cats. There are instructions in the file for customizing the settings for
  other combinations of animals. These settings are also only set when a
  fortress is first started, so any later changes you make to autobutcher
  settings won't be overridden.
- Enables `automelt`, `tailor`, `zone`, `nestboxes`, and `autonestbox`.

The ``orders/`` subfolder
-------------------------

The :source:`orders/ <data/examples/orders>` subfolder contains manager orders
that, along with the ``onMapLoad_dreamfort.init`` file above, allow a fort to be
self-sustaining. Copy them to your ``dfhack-config/orders/`` folder and import
as required with the `orders` DFHack plugin.

:source:`basic.json <data/examples/orders/basic.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

:source:`furnace.json <data/examples/orders/furnace.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

:source:`military.json <data/examples/orders/military.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

:source:`smelting.json <data/examples/orders/smelting.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection adds smelting jobs for all ores. It includes handling the ores
already managed by ``military.json``, but has lower limits. This ensures all
ores will be covered if a player imports ``smelting`` but not ``military``, but
the higher-volume ``military`` orders will take priority if both are imported.

:source:`rockstock.json <data/examples/orders/rockstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This collection of orders keeps a small stock of all types of rock furniture.
This allows you to do ad-hoc furnishings of guildhalls, libraries, temples, or
other rooms with `buildingplan` and your masons will make sure there is always
stock on hand to fulfill the plans.

:source:`glassstock.json <data/examples/orders/glassstock.json>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
