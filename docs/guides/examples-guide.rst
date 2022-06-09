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

The ``professions/`` subfolder
------------------------------

The :source:`professions/ <data/examples/professions>` subfolder contains
professions, or sets of related labors, that you can assign to your dwarves with
the DFHack `manipulator` plugin. Copy them into the ``professions/``
subdirectory under the main Dwarf Fortress folder (you may have to create this
subdirectory) and assign them to your dwarves in the manipulator UI, accessible
from the ``units`` screen via the :kbd:`l` hotkey. Make sure that the
``manipulator`` plugin is enabled in your ``dfhack.init`` file! You can assign a
profession to a dwarf by selecting the dwarf in the ``manipulator`` UI and
hitting :kbd:`p`. The list of professions that you copied into the
``professions/`` folder will show up for you to choose from. This is very useful
for assigning roles to new migrants to ensure that all the tasks in your fort
have adequate numbers of dwarves attending to them.

If you'd rather use Dwarf Therapist to manage your labors, it is easy to import
these professions to DT and use them there. Simply assign the professions you
want to import to a dwarf. Once you have assigned a profession to at least one
dwarf, you can select "Import Professions from DF" in the DT "File" menu. The
professions will then be available for use in DT.

In the charts below, the "At Start" and "Max" columns indicate the approximate
number of dwarves of each profession that you are likely to need at the start of
the game and how many you are likely to need in a mature fort.

=============  ========  ===== =================================================
Profession     At Start  Max   Description
=============  ========  ===== =================================================
Chef           0         3     Buchery, Tanning, and Cooking. It is important to
                               focus just a few dwarves on cooking since
                               well-crafted meals make dwarves very happy. They
                               are also an excellent trade good.
Craftsdwarf    0         4-6   All labors used at Craftsdwarf's workshops,
                               Glassmaker's workshops, and kilns.
Doctor         0         2-4   The full suite of medical labors, plus Animal
                               Caretaking for those using the dwarfvet plugin.
Farmer         1         4     Food- and animal product-related labors. This
                               profession also has the ``Alchemist`` labor
                               enabled since they need to focus on food-related
                               jobs, though you might want to disable
                               ``Alchemist`` for your first farmer until there
                               are actual farming duties to perform.
Fisherdwarf    0         0-1   Fishing and fish cleaning. If you assign this
                               profession to any dwarf, be prepared to be
                               inundated with fish. Fisherdwarves *never stop
                               fishing*. Be sure to also run ``prioritize -a
                               PrepareRawFish ExtractFromRawFish`` (or use the
                               ``onMapLoad_dreamfort.init`` file above) or else
                               caught fish will just be left to rot.
Hauler         0         >20   All hauling labors plus Siege Operating, Mechanic
                               (so haulers can assist in reloading traps) and
                               Architecture (so haulers can help build massive
                               windmill farms and pump stacks). As you
                               accumulate enough Haulers, you can turn off
                               hauling labors for other dwarves so they can
                               focus on their skilled tasks. You may also want
                               to restrict your Mechanic's workshops to only
                               skilled mechanics so your haulers don't make
                               low-quality mechanisms.
Laborer        0         10-12 All labors that don't improve quality with skill,
                               such as Soapmaking and furnace labors.
Marksdwarf     0         10-30 Similar to Hauler. See the description for
                               Meleedwarf below for more details.
Mason          2         2-4   Masonry, Gem Cutting/Encrusting, and
                               Architecture. In the early game, you may need to
                               run "`prioritize` ConstructBuilding" to get your
                               masons to build wells and bridges if they are too
                               busy crafting stone furniture.
Meleedwarf     0         20-50 Similar to Hauler, but without most civilian
                               labors. This profession is separate from Hauler
                               so you can find your military dwarves easily.
                               Meleedwarves and Marksdwarves have Mechanics and
                               hauling labors enabled so you can temporarily
                               deactivate your military after sieges and allow
                               your military dwarves to help clean up.
Migrant        0         0     You can assign this profession to new migrants
                               temporarily while you sort them into professions.
                               Like Marksdwarf and Meleedwarf, the purpose of
                               this profession is so you can find your new
                               dwarves more easily.
Miner          2         2-10  Mining and Engraving. This profession also has
                               the ``Alchemist`` labor enabled, which disables
                               hauling for those using the `autohauler` plugin.
                               Once the need for Miners tapers off in the late
                               game, dwarves with this profession make good
                               military dwarves, wielding their picks as
                               weapons.
Outdoorsdwarf  1         2-4   Carpentry, Bowyery, Woodcutting, Animal Training,
                               Trapping, Plant Gathering, Beekeeping, and Siege
                               Engineering.
Smith          0         2-4   Smithing labors. You may want to specialize your
                               Smiths to focus on a single smithing skill to
                               maximize equipment quality.
StartManager   1         0     All skills not covered by the other starting
                               professions (Miner, Mason, Outdoorsdwarf, and
                               Farmer), plus a few overlapping skills to
                               assist in critical tasks at the beginning of the
                               game. Individual labors should be turned off as
                               migrants are assigned more specialized
                               professions that cover them, and the StartManager
                               dwarf can eventually convert to some other
                               profession.
Tailor         0         2     Textile industry labors: Dying, Leatherworking,
                               Weaving, and Clothesmaking.
=============  ========  ===== =================================================

A note on autohauler
~~~~~~~~~~~~~~~~~~~~

These profession definitions are designed to work well with or without the
`autohauler` plugin (which helps to keep your dwarves focused on skilled labors
instead of constantly being distracted by hauling). If you do want to use
autohauler, adding the following lines to your ``onMapLoad.init`` file will
configure it to let the professions manage the "Feed water to civilians" and
"Recover wounded" labors instead of enabling those labors for all hauling
dwarves::

    on-new-fortress enable autohauler
    on-new-fortress autohauler FEED_WATER_CIVILIANS allow
    on-new-fortress autohauler RECOVER_WOUNDED allow
