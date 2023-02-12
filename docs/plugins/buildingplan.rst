buildingplan
============

.. dfhack-tool::
    :summary: Plan building construction before you have materials.
    :tags: fort design buildings

This plugin adds a planning mode for building placement. You can then place
furniture, constructions, and other buildings before the required materials are
available, and they will be created in a suspended state. Buildingplan will
periodically scan for appropriate items, and the jobs will be unsuspended when
the items are available.

This is very powerful when used with tools like `quickfort`, which allow you to
set a building plan according to a blueprint, and the buildings will simply be
built when you can build them.

You can use manager work orders or `workflow` to ensure you always have one or
two doors/beds/tables/chairs/etc. available, and place as many as you like.
Materials are used to build the planned buildings as they are produced, with
minimal space dedicated to stockpiles.

Usage
-----

::

    enable buildingplan
    buildingplan [status]
    buildingplan set <setting> true|false

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

    on-new-fortress buildingplan set boulders false; buildingplan set logs false

.. _buildingplan-filters:

Item filtering
--------------

While placing a building, you can set filters for what materials you want the
building made out of, what quality you want the component items to be, and
whether you want the items to be decorated.

If a building type takes more than one item to construct, use
:kbd:`Ctrl`:kbd:`Left` and :kbd:`Ctrl`:kbd:`Right` to select the item that you
want to set filters for. Any filters that you set will be used for all buildings
of the selected type placed from that point onward (until you set a new filter
or clear the current one). Buildings placed before the filters were changed will
keep the filter values that were set when the building was placed.

For example, you can be sure that all your constructed walls are the same color
by setting a filter to accept only certain types of stone.
