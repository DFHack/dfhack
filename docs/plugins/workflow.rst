workflow
========
Manage control of repeat jobs.  `gui/workflow` provides a simple
front-end integrated in the game UI.

Usage:

``workflow enable [option...], workflow disable [option...]``
   If no options are specified, enables or disables the plugin.
   Otherwise, enables or disables any of the following options:

   - drybuckets: Automatically empty abandoned water buckets.
   - auto-melt: Resume melt jobs when there are objects to melt.
``workflow jobs``
   List workflow-controlled jobs (if in a workshop, filtered by it).
``workflow list``
   List active constraints, and their job counts.
``workflow list-commands``
   List active constraints as workflow commands that re-create them;
   this list can be copied to a file, and then reloaded using the
   ``script`` built-in command.
``workflow count <constraint-spec> <cnt-limit> [cnt-gap]``
   Set a constraint, counting every stack as 1 item.
``workflow amount <constraint-spec> <cnt-limit> [cnt-gap]``
   Set a constraint, counting all items within stacks.
``workflow unlimit <constraint-spec>``
   Delete a constraint.
``workflow unlimit-all``
   Delete all constraints.

Function
--------
When the plugin is enabled, it protects all repeat jobs from removal.
If they do disappear due to any cause, they are immediately re-added to their
workshop and suspended.

In addition, when any constraints on item amounts are set, repeat jobs that
produce that kind of item are automatically suspended and resumed as the item
amount goes above or below the limit. The gap specifies how much below the limit
the amount has to drop before jobs are resumed; this is intended to reduce
the frequency of jobs being toggled.

Constraint format
-----------------
The constraint spec consists of 4 parts, separated with ``/`` characters::

    ITEM[:SUBTYPE]/[GENERIC_MAT,...]/[SPECIFIC_MAT:...]/[LOCAL,<quality>]

The first part is mandatory and specifies the item type and subtype,
using the raw tokens for items (the same syntax used custom reaction inputs).
For more information, see :wiki:`this wiki page <Material_token>`.

The subsequent parts are optional:

- A generic material spec constrains the item material to one of
  the hard-coded generic classes, which currently include::

    PLANT WOOD CLOTH SILK LEATHER BONE SHELL SOAP TOOTH HORN PEARL YARN
    METAL STONE SAND GLASS CLAY MILK

- A specific material spec chooses the material exactly, using the
  raw syntax for reaction input materials, e.g. ``INORGANIC:IRON``,
  although for convenience it also allows just ``IRON``, or ``ACACIA:WOOD`` etc.
  See the link above for more details on the unabbreviated raw syntax.

- A comma-separated list of miscellaneous flags, which currently can
  be used to ignore imported items or items below a certain quality.

Constraint examples
-------------------
Keep metal bolts within 900-1000, and wood/bone within 150-200::

    workflow amount AMMO:ITEM_AMMO_BOLTS/METAL 1000 100
    workflow amount AMMO:ITEM_AMMO_BOLTS/WOOD,BONE 200 50

Keep the number of prepared food & drink stacks between 90 and 120::

    workflow count FOOD 120 30
    workflow count DRINK 120 30

Make sure there are always 25-30 empty bins/barrels/bags::

    workflow count BIN 30
    workflow count BARREL 30
    workflow count BOX/CLOTH,SILK,YARN 30

Make sure there are always 15-20 coal and 25-30 copper bars::

    workflow count BAR//COAL 20
    workflow count BAR//COPPER 30

Produce 15-20 gold crafts::

    workflow count CRAFTS//GOLD 20

Collect 15-20 sand bags and clay boulders::

    workflow count POWDER_MISC/SAND 20
    workflow count BOULDER/CLAY 20

Make sure there are always 80-100 units of dimple dye::

    workflow amount POWDER_MISC//MUSHROOM_CUP_DIMPLE:MILL 100 20

.. note::

  In order for this to work, you have to set the material of the PLANT input
  on the Mill Plants job to MUSHROOM_CUP_DIMPLE using the `job item-material <job>`
  command. Otherwise the plugin won't be able to deduce the output material.

Maintain 10-100 locally-made crafts of exceptional quality::

    workflow count CRAFTS///LOCAL,EXCEPTIONAL 100 90
