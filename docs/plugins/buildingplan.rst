buildingplan
============

.. dfhack-tool::
    :summary: Plan building layouts with or without materials.
    :tags: fort design productivity buildings

Buildingplan allows you to place furniture, constructions, and other buildings,
regardless of whether the required materials are available. This allows you to
focus purely on design elements when you are laying out your fort, and defers
item production concerns to a more convenient time.

Buildingplan is an alternative to the vanilla building placement UI. It appears
after you have selected the type of building, furniture, or construction that
you want to place in the vanilla build menu. Buildingplan then takes over for
the actual placement step. If the placed building requires materials that
aren't available yet, it will be created in a suspended state. Buildingplan will
periodically scan for appropriate items and attach them to the planned
building. Once all items are attached, the construction job will be unsuspended
and a dwarf will come and build the building. If you have the `unsuspend`
overlay enabled (it is enabled by default), then buildingplan-suspended
buildings will appear with a ``P`` marker on the main map, as opposed to the
usual ``x`` marker for "regular" suspended buildings.

If you want to impose restrictions on which items are chosen for the buildings,
buildingplan has full support for quality and material filters (see `below
<Setting quality and material filters>`_). This lets you create layouts with a
consistent color, if that is part of your design.

If you just care about the heat sensitivity of the building, you can set the
building to be fire- or magma-proof in the placement UI screen. This makes it
very easy to ensure that your pump stacks and floodgates, for example, are
magma-safe.

Buildingplan works well in conjuction with other design tools like
`gui/quickfort`, which allow you to apply a building layout from a blueprint.
You can apply very large, complicated layouts, and the buildings will simply be
built when your dwarves get around to producing the needed materials. If you
set filters in the buildingplan UI before applying the blueprint, the filters
will be applied to the blueprint buildings, just as if you had planned them
from the buildingplan placement UI.

One way to integrate buildingplan into your gameplay is to create manager
workorders to ensure you always have a few blocks/doors/beds/etc. available. You
can then place as many of each building as you like. Produced items will be used
to build the planned buildings as they are produced, with minimal space
dedicated to stockpiles. The DFHack `orders` library can help with setting
these manager workorders up for you.

If you do not wish to use the ``buildingplan`` interface, you can turn off the
``buildingplan.planner`` overlay in `gui/control-panel` (on the "Overlays"
tab). You should not disable the ``buildingplan`` "System service" in
`gui/control-panel` since existing planned buildings in loaded forts will stop
functioning.

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

``buildingplan set boulders false``
    When finding items to satisfy "building materials" requirements, don't
    select boulders. Use blocks or logs (if enabled) instead.

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

Building placement
------------------

Once you have selected a building type to build in the vanilla build menu, the
`buildingplan` placement UI appears as an `overlay` widget, covering the
vanilla building placement panel.

For basic usage, you don't need to change any settings. Just click to place
buildings of the selected type and right click to exit building mode. Any
buildings that require materials that you don't have on hand will be suspended
and built when the items are available.

When building constructions, you'll get a few extra options, like whether the
construction area should be hollow or what types of stairs you'd like at the
top and bottom of a stairwell. Also, unlike other buildings, it is ok if some
tiles selected in the construction area are not appropriate for building. For
example, if you want to fill an area with flooring, you can select the entire
area, and any tiles with existing buildings or walls will simply be skipped.

Setting heat safety filters
+++++++++++++++++++++++++++

If you specifically need the building to be magma- or fire-safe, click on the
"Building safety" button or hit :kbd:`g` until the desired heat safety is
displayed. This filter applies to all items used to construct the building.

Setting quality and material filters
++++++++++++++++++++++++++++++++++++

If you want to set restrictions on the items chosen to complete the planned
building, you can click on the "filter" button next to the item name or select
the item with the :kbd:`*` and :kbd:`/` keys and hit :kbd:`f` to bring up the
filter dialog.

You can select whether the item must be decorated, and you can drag the ends of
the "Item quality" slider to set your desired quality range. Note that blocks,
boulders, logs, and bars don't have a quality, and the quality options are
disabled for those types. As you change the quality settings, the number of
currently available matched items of each material is adjusted in the materials
list.

You can click on specific materials to allow only items of those materials when
building the current type of building. You can also allow or disallow entire
categories of materials by clicking on the "Type" options on the left. Note
that it is perfectly fine to choose materials that currently show zero quantity.
`buildingplan` will patiently watch for items made of materials you have
selected.

Choosing specific items
+++++++++++++++++++++++

If you want to choose specific items, click on the "Choose from items" toggle
or hit :kbd:`i` before placing the building. When you click to place the
building, a dialog will come up that allows you choose which items to use. The
list is sorted by most recently used materials for that building type by
default, but you can change to sort by name or by available quantity by
clicking on the "Sort by" selector or hitting :kbd:`R`. The configuration for
whether you would like to choose specific items is saved per building type and
will be restored when you plan more of that building type.

You can select the maximum quantity of a specified item by clicking on the item
name or selecting it with the arrow keys and hitting :kbd:`Enter`. You can
instead select items one at a time by Ctrl-clicking (:kbd:`Shift`:kbd:`Right`)
to increment or Ctrl-Shift-clicking (:kbd:`Shift`:kbd:`Left`) to decrement.

Once you are satisfied with your choices, click on the "Build" button or hit
:kbd:`B` to continue building. Note that you don't have to select all the items
that the building needs. Any remaining items will be automatically chosen from
other available items (or future items if not all items are available yet). If
there are multiple item types to choose for the current building, one dialog
will appear per item type.

Building status
---------------

When viewing a planned building, a separate `overlay` widget appears on the
building info sheet, showing you which items have been attached and which items
are still pending. For a pending item, you can see its position in the
fulfillment queue. If there is a particular building that you need built ASAP,
you can click on the "make top priority" button (or hit :kbd:`Ctrl`:kbd:`T`) to
bump the items for this building to the front of their respective queues.

Note that each item type and filter configuration has its own queue, so even if
an item is in queue position 1, there may be other queues that snag the needed
item first.
