suspendmanager
==============

.. dfhack-tool::
    :summary: Intelligently suspend and unsuspend jobs.
    :tags: fort auto jobs

This tool will watch your active jobs and:

- unsuspend jobs that have become suspended due to inaccessible materials,
  items temporarily in the way, or worker dwarves getting scared by wildlife
- suspend most construction jobs that would prevent a dwarf from reaching another
  construction job, such as when building a wall corner or high walls
- suspend construction jobs on top of a smoothing, engraving or track carving
  designation. This prevents the construction job from being completed first,
  which would erase the designation.
- suspend construction jobs that would cave in immediately on completion,
  such as when building walls or floors next to grates/bars.

Usage
-----

``enable suspendmanager``
    Start monitoring jobs.

``disable suspendmanager``
    Stop monitoring jobs.

``suspendmanager``
    Display the current status

``suspendmanager set preventblocking (true|false)``
    Prevent construction jobs from blocking each others (enabled by default). See `suspend`.


Overlay
-------

This plugin also provides an overlay that is managed by the `overlay` framework.
When the overlay is enabled, an icon or letter will appear over suspended
buildings:

- A clock icon (green ``P`` in ASCII mode) indicates that the building is still
  in planning mode and is waiting on materials. The `buildingplan` plugin will
  unsuspend it for you when those materials become available.
- A white ``x`` means that the building is maintained suspended by
  `suspendmanager`, selecting it will provide a reason for the suspension
- A yellow ``x`` means that the building is suspended. If you don't have
  `suspendmanager` managing suspensions for you, you can unsuspend it
  manually or with the `unsuspend` command.
- A red ``X`` means that the building has been re-suspended multiple times.
  You might need to look into whatever is preventing the building from being
  built (e.g. the building material for the building is inaccessible or there
  is an in-use item blocking the building site).

Note that in ASCII mode the letter will only appear when the game is paused
since it takes up the whole tile and makes the underlying building invisible.
In graphics mode, the icon only covers part of the building and so can always
be visible.