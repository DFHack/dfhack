preserve-rooms
==============

.. dfhack-tool::
    :summary: Manage room assignments for off-map units and noble roles.
    :tags: fort bugfix interface

When a citizen leaves the map for any reason, e.g. when going off on a raid,
they lose all their zone assignments. Any bedrooms, offices, dining rooms, or
tombs assigned to the citizen will become unassigned and may be claimed by
another unit while they are away.

A related issue occurs when a noble or administrative role changes hands,
either because you have reassigned them or because of an internal fort
election. Rooms you have assigned to the previous noble stay assigned to them,
and the new noble complains because their room requirements are not being met.

This tool mitigates both issues. It records when units leave the map and
reserves their assigned bedrooms, offices, etc. for them. The zones will be
disabled in their absence (so other units don't steal them), and will be
re-enabled and reassigned to them when they appear back on the map. If they die
away from the fort, the zone will become unreserved and available for reuse.

When you click on an assignable zone, you will also now have the option to
associate the room with a noble or administrative role. The room will be
automatically reassigned to whoever currently holds that position. If multiple
rooms of the same type are assigned to a position, then only one room of that
type will be assigned to each holder of that position (e.g. one room per baron
or militia captain).

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
``preserve-rooms disable track-missions``
    Disable the ``track-missions`` feature for this fort.
``preserve-rooms reset track-roles``
    Clear all configuration related to the ``track-roles`` feature (currently
    assigned rooms are not unassigned).

Features
--------

``track-missions``
    Reserve the rooms assigned to units that leave the map and reassign them
    upon their return. This feature is enabled by default.
``track-roles``
    Allow rooms to be associated with noble or adminstrative roles. Associated
    rooms will be automatically assigned to the current holder of the specified
    role. This feature is enabled by default.

Overlay
-------

The ``preserve-rooms.reserved`` overlay indicates whether a zone is disabled
because it is being reserved for a unit that left the map and is expected to
return. For unreserved rooms, it provides widgets to mark the zone as
associated with a specific noble or administrative role.

Notes
-----

This tool fixes rooms being unassigned when a unit leaves the map. This is a
different bug from the one fixed by `preserve-tombs`, which handles the case
where a tomb is unassigned upon a unit's death, preventing them from ever being
buried in their own tomb.
