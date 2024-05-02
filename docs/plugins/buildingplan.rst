buildingplan
============

.. dfhack-tool::
    :summary: Plan building construction before you have materials.
    :tags: untested fort design buildings

This plugin adds a planning mode for building placement. You can then place
furniture, constructions, and other buildings before the required materials are
available, and they will be created in a suspended state. Buildingplan will
periodically scan for appropriate items, and the jobs will be unsuspended when
the items are available.

This is very useful when combined with manager work orders or `workflow` -- you
can set a constraint to always have one or two doors/beds/tables/chairs/etc.
available, and place as many as you like. Materials are used to build the
planned buildings as they are produced, with minimal space dedicated to
stockpiles.

Usage
-----

::

    enable buildingplan
    buildingplan set
    buildingplan set <setting> true|false

Running ``buildingplan set`` without parameters displays the current settings.

.. _buildingplan-settings:

Global settings
---------------

The buildingplan plugin has global settings that can be set from the UI
(:kbd:`G` from any building placement screen, for example:
:kbd:`b`:kbd:`a`:kbd:`G`). These settings can also be set via the
``buildingplan set`` command. The available settings are:

``all_enabled`` (default: false)
    Enable planning mode for all building types.
``blocks``, ``boulders``, ``logs``, ``bars`` (defaults: true, true, true, false)
    Allow blocks, boulders, logs, or bars to be matched for generic "building
    material" items.
``quickfort_mode`` (default: false)
    Enable compatibility mode for the legacy Python Quickfort (this setting is
    not required for DFHack `quickfort`)

The settings for ``blocks``, ``boulders``, ``logs``, and ``bars`` are saved with
your fort, so you only have to set them once and they will be persisted in your
save.

If you normally embark with some blocks on hand for early workshops, you might
want to add this line to your ``dfhack-config/init/onMapLoad.init`` file to
always configure buildingplan to just use blocks for buildings and
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

Quickfort mode
--------------

If you use the external Python Quickfort to apply building blueprints instead of
the native DFHack `quickfort` script, you must enable Quickfort mode. This
temporarily enables buildingplan for all building types and adds an extra blank
screen after every building placement. This "dummy" screen is needed for Python
Quickfort to interact successfully with Dwarf Fortress.

Note that Quickfort mode is only for compatibility with the legacy Python
Quickfort. The DFHack `quickfort` script does not need this Quickfort mode to be
enabled. The `quickfort` script will successfully integrate with buildingplan as
long as the buildingplan plugin itself is enabled.
