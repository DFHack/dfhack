suspendmanager
==============

.. dfhack-tool::
    :summary: Intelligently suspend and unsuspend jobs.
    :tags: fort auto jobs

.. dfhack-command:: unsuspend
    :summary: Resume suspended building construction jobs.

When enabled, ``suspendmanager`` will watch your active jobs and:

- unsuspend jobs that have become suspended due to inaccessible materials,
  items temporarily in the way, or worker dwarves getting scared by wildlife
- suspend most construction jobs that would prevent a dwarf from reaching
  another construction job, such as when building a wall corner or high walls
- suspend construction jobs on top of smoothing, engraving, or track carving
  designations. This prevents the construction job from being completed first,
  which would cause the designation to be lost.
- suspend construction jobs that would cave in immediately on completion,
  such as when building walls or floors next to grates/bars.

Usage
-----

``enable suspendmanager``
    Start monitoring jobs.

``suspendmanager``
    Display the current status

``suspendmanager set preventblocking (true|false)``
    Prevent construction jobs from blocking each others (enabled by default). See `suspend`.

``unsuspend [-s|--skipblocking] [-q|--quiet]``
    Perform a single cycle, suspending and unsuspending jobs as described above,
    regardless of whether `suspendmanager` is enabled.

    This allows you to quickly recover if a bunch of jobs were suspended due to
    the workers getting scared off by wildlife or items temporarily blocking
    building sites.

Options
-------

``-q``, ``--quiet``
    Suppress summary output.

``-s``, ``--skipblocking``
    Also resume jobs that may block other jobs.

Overlay
-------

This plugin also provides an overlay that is managed by the `overlay` framework.
When the overlay is enabled, an icon or letter will appear over suspended
buildings:

- A clock icon (green ``P`` in ASCII mode) indicates that the building is still
  in `planning mode <buildingplan>` and is waiting on materials. The building
  will be unsuspended for you when those materials become available.
- A white ``x`` means that the building has been suspended by
  `suspendmanager`. If you select the building in the UI, you will see the
  reason for the suspension.
- A yellow ``x`` means that the building is suspended and no DFHack tool is
  managing the suspension. You can unsuspend it manually or with the
  `unsuspend` command.
- A red ``X`` means that the building has been re-suspended multiple times.
  You might need to look into whatever is preventing the building from being
  built (e.g. the building material for the building is inaccessible or there
  is an in-use item blocking the building site).

Note that in ASCII mode the letter will only appear when the game is paused
since it takes up the whole tile and hides the underlying building. In graphics
mode, the icon only covers part of the building and so can always be visible.
