buildingplan
============

.. dfhack-tool::
    :summary: Plan building layouts with or without materials.
    :tags: fort design buildings

Buildingplan allows you to place furniture, constructions, and other buildings,
regardless of whether the required materials are available. This allows you to
focus purely on design elements when you are laying out your fort, and defers
item production concerns to a more convenient time.

Buildingplan is as an alternative to the vanilla building placement UI. It
appears after you have selected the type of building, furniture, or construction
that you want to place in the vanilla build menu. Buildingplan then takes over
for the actual placement step. If any building materials are not available yet
for the placed building, it will be created in a suspended state. Buildingplan
will periodically scan for appropriate items and attach them. Once all items are
attached, the construction job will be unsuspended and a dwarf will come and
build the building. If you have the `unsuspend` overlay enabled (it is enabled
by default), then buildingplan-suspended buildings will appear with a ``P`` marker
on the main map, as opposed to the usual ``x`` marker for "regular" suspended
buildings.

If you want to impose restrictions on which items are chosen for the buildings,
buildingplan has full support for quality and material filters. Before you place
a building, you can select a component item in the list and hit ``f`` or click on
the ``filter`` button next to the item description. This will let you choose your
desired item quality range, whether the item must be decorated, and even which
specific materials the item must be made out of. This lets you create layouts
with a consistent color, if that is part of your design.

If you just care about the heat sensitivity of the building, you can set the
building to be fire- or magma-proof in the placement UI screen or in any item
filter screen, and the restriction will apply to all building items. This makes it
very easy to create magma-safe pump stacks, for example.

Buildingplan works very well in conjuction with other design tools like
`gui/quickfort`, which allow you to apply a building layout from a blueprint. You
can apply very large, complicated layouts, and the buildings will simply be built
when your dwarves get around to producing the needed materials. If you set filters
in the buildingplan UI before applying the blueprint, the filters will be applied
to the blueprint buildings, just as if you had planned them from the buildingplan
placement UI.

One way to integrate buildingplan into your gameplay is to create manager
workorders to ensure you always have a few blocks/doors/beds/etc. available. You
can then place as many of each building as you like. Produced items will be used
to build the planned buildings as they are produced, with minimal space dedicated
to stockpiles. The DFHack `orders` library can help with setting up these manager
workorders for you.

If you do not wish to use the ``buildingplan`` interface, you can turn off the
``buildingplan.planner`` overlay in `gui/overlay`. You should not disable the
``buildingplan`` service entirely in `gui/control-panel` since then existing
planned buildings in loaded forts will stop functioning.

Usage
-----

::

    buildingplan [status]
    buildingplan set <setting> (true|false)

Examples
--------

``buildingplan``
    Print a report of current settings, which kinds of buildings are planned,
    and what kinds of materials the buildings are waiting for.

.. _buildingplan-settings:

Global settings
---------------

The buildingplan plugin has several global settings that affect what materials
can be chosen when attaching items to planned buildings:

``blocks``, ``boulders``, ``logs``, ``bars`` (defaults: true, true, true, false)
    Allow blocks, boulders, logs, or bars to be matched for generic "building
    material" items.

These settings are saved with your fort, so you only have to set them once and
they will be persisted in your save.

If you normally embark with some blocks on hand for early workshops, you might
want to add this line to your ``dfhack-config/init/onMapLoad.init`` file to
always configure `buildingplan` to just use blocks for buildings and
constructions::

    on-new-fortress buildingplan set boulders false
    on-new-fortress buildingplan set logs false
