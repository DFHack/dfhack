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
buildings will be tagged with a clock graphic in graphics mode or a ``P``
marker in ASCII mode, as opposed to the ``x`` marker for "regular" suspended
buildings. If you have `suspendmanager` running, then buildings will be left
suspended when their items are all attached and ``suspendmanager`` will
unsuspend them for construction when it is safe to do so.

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
can then place as many of each building as you like. Items will be used to
build the planned buildings as they are produced, with minimal space dedicated
to stockpiles. The DFHack `orders` library can help with setting these manager
workorders up for you.

If you don't want to use the ``buildingplan`` interface for the building you're
currently trying to place, you can hit :kbd:`Alt`:kbd:`M` or click on the
minimize toggle in the upper right corner of the panel. If you do not wish to
ever use the ``buildingplan`` interface, you can turn off the
``buildingplan.planner`` overlay in `gui/control-panel` (on the "Overlays"
tab).

Usage
-----

::

    buildingplan [status]
    buildingplan set <setting> (true|false)
    buildingplan reset

Examples
--------

``buildingplan``
    Print a report of current settings, which kinds of buildings are planned,
    and what kinds of materials the buildings are waiting for.

``buildingplan set boulders false``
    When finding items to satisfy "building materials" requirements, don't
    select boulders. Use blocks or logs (if enabled) instead.

``buildingplan reset``
    Reset all settings and filters to their defaults. This command does not
    affect existing planned buildings.

.. _buildingplan-settings:

Global settings
---------------

The buildingplan plugin has several global settings that affect what materials
can be chosen when attaching items to planned buildings:

``blocks``, ``boulders``, ``logs``, ``bars`` (defaults: true, true, true, false)
    Allow blocks, boulders, logs, or bars to be matched for generic "building
    material" items.
``reconstruct`` (default: true)
    When you plan constructions, allow building on already-built constructions.
    For example, if you're planning a wall, you will be able to place it on an
    already-built floor. This matches vanilla behavior. However, it can be
    annoying that floors can be planned on top of other constructed floors,
    especially when you're trying to fill in "holes" in a large area of
    constructed flooring. Turn off to treat existing constructions as "invalid"
    locations for new constructions. This can help when extending existing
    constructions and will prevent you from wasting materials by constructing
    twice on a tile. Note that even if this option is disabled, you can still
    choose to place a construction on top of an existing construction if you
    just select a single 1x1 tile as your planning area.

These settings are saved with your fort, so you only have to set them once and
they will be persisted in your save.

If you normally embark with some blocks on hand for early workshops, you might
want to enable the following two commands on the ``Autostart`` subtab of the
``Automation`` tab to always configure `buildingplan` to just use blocks for
buildings and constructions::

    buildingplan set boulders false
    buildingplan set logs false

Building placement
------------------

Once you have selected a building type to build in the vanilla build menu, the
`buildingplan` placement UI appears as an `overlay` widget, covering the
vanilla building placement panel.

For basic usage, you don't need to change any settings. Just click to place
buildings of the selected type and right click to exit building mode. Any
buildings that require materials that you don't have on hand will be suspended
and built when the items are available. The closest available material will be
chosen for the building job.

When building constructions, you'll get a few extra options, like whether the
construction area should be hollow or what types of stairs you'd like at the
top and bottom of a stairwell. Also, unlike other buildings, it is ok if some
tiles selected in the construction area are not appropriate for building. For
example, if you want to fill an area with flooring, you can select the entire
area, and any tiles with existing buildings or walls will simply be skipped.

Some building types will have other options available as well, such as a
selector for how many weapons you want in weapon traps or whether you want to
only build engraved slabs.

Setting quality and material filters
++++++++++++++++++++++++++++++++++++

If you want to set restrictions on the items chosen to complete the planned
building, you can click on the "[any material]" link next to the item name or
select the item with the :kbd:`q` or :kbd:`Q` keys and hit :kbd:`f` to bring up
the filter dialog.

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
`buildingplan` will patiently wait for items made of materials you have
selected to become available.

Choosing specific items
+++++++++++++++++++++++

If you want to choose specific items instead of using the filters, click on the
"Choose items" selector or hit :kbd:`z` before placing the building. You can
choose to be prompted for every item ("Manually") or you can have it
automatically select the type of item that you last chose for this building
type. The list you are prompted with is sorted by most recently used materials
for that building type by default, but you can change to sort by name or by
available quantity by clicking on the "Sort by" selector or hitting :kbd:`R`.
The configuration for whether you would like to choose specific items is saved
per building type and will be restored when you plan more of that building type.

You can select the maximum quantity of a specified item by clicking on the item
name or selecting it with the arrow keys and hitting :kbd:`Enter`. You can
instead select items one at a time by Ctrl-clicking (:kbd:`Shift`:kbd:`Right`)
to increment or Ctrl-Shift-clicking (:kbd:`Shift`:kbd:`Left`) to decrement.

Once you are satisfied with your choices, click on the large green button or hit
:kbd:`C` to continue building. Note that you don't have to select all the items
that the building needs. Any remaining items will be automatically chosen from
other available items (or from items produced in the future if not all items
are available yet). If there are multiple item types to choose for the current
building, one dialog will appear per item type.

Building status
---------------

When viewing a planned building, a separate `overlay` widget appears on the
building info sheet, showing you which items have been attached and which items
are still pending. For a pending item, you can see its position in the
fulfillment queue. You need to manufacture these items for them to be attached
to the building. If there is a particular building that you need built ASAP,
you can click on the "make top priority" button (or hit :kbd:`Ctrl`:kbd:`T`) to
bump the items for this building to the front of their respective queues.

Note that each item type and filter configuration has its own queue, so even if
an item is in queue position 1, there may be other queues that snag the needed
item first.

Lever linking
-------------

When linking levers, `buildingplan` extends the vanilla panel by offering
control over which mechanisms are chosen for installation at the lever and at
the target. Heat safety filters are provided for convenience.

Mechanism unlinking
-------------------

When selecting a building linked with mechanisms, buttons to ``Unlink`` appear by
each linked building on the ``Show linked buildings`` tab. This will undo the
link without having to deconstruct and rebuild the target building. The unlinked
mechanisms will remain a part of their respective buildings (providing value as
usual) unless freed via the ``Free`` buttons on the ``Show items`` tab on both
buildings. This will remove the mechanism from the building and drop it onto the
ground, allowing it to be reused elsewhere. There is an option to auto-free
mechanisms when unlinking to perform this step automatically.
