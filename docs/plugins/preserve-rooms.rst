preserve-rooms
==============

.. dfhack-tool::
    :summary: Manage room assignments for off-map units and noble roles.
    :tags: fort bugfix interface

When a citizen leaves the map for any reason, e.g. when going off on a raid,
they lose all their zone assignments. Any bedrooms, offices, dining rooms, and
tombs assigned to the citizen will become unassigned and may be claimed by
another unit while they are away.

A related issue occurs when a noble role changes hands, either because you have
reassigned them or because of an internal fort election. Rooms you have
assigned to the previous noble stay assigned to them, and the new noble
complains because their room requirements are not being met.

This tool solves both issues. It records when units leave the map and reserves
their assigned bedrooms, offices, etc. for them. The zones will be disabled in
their absence, and will be re-enabled and reassigned to them when they appear
back on the map. If they die away from the fort, the zone will become
unreserved and available for reuse.

When you click on an assignable zone, you will also now have the option to
associate the room with a noble role. The room will be automatically reassigned
to whoever currently holds that noble position. If multiple rooms of the same
type are assigned to a position that can be filled by multiple citizens (e.g.
you can have many barons), then only one room of that type will be assigned to
each holder of that position.

Usage
-----

::

    preserve-rooms [status]
    preserve-rooms now
    preserve-rooms enable|disable <feature>
    preserve-rooms reset <feature>

Examples
--------

``preserve-rooms``
    List the types of rooms that are assigned to each noble role and which of
    those rooms are being automatically assigned to new holders of the
    respective office.
``preserve-rooms now``
    Do an immediate update of room assignments. The plugin does this routinely
    in the background, but you can run it manually to update now.
``preserve-rooms disable track-raids``
    Disable the ``track-raids`` feature for this fort.
``preserve-rooms reset noble-roles``
    Clear all configuration related to the ``noble-roles`` feature.

Features
--------

``track-raids``
    Reserve the rooms assigned to units that leave the map and reassign them
    upon their return. This feature is automatically enabled for new forts
    unless disabled in `gui/control-panel` ("Bugfixes" tab).
``noble-roles``
    Allow rooms to be associated with noble roles. Associated rooms will be
    automatically assigned to the current holder of the specified role. This
    feature is enabled by default for all forts.

Overlay
-------

The ``preserve-rooms.reserved`` overlay indicates whether a zone is disabled
because it is being reserved for a unit that left the map and is expected to
return. It also provides widgets to mark the zone as associated with a role.
