labormanager
============

.. dfhack-tool::
    :summary: Automatically manage dwarf labors.
    :tags: unavailable

Labormanager is derived from `autolabor` but uses a completely different
approach to assigning jobs to dwarves. While autolabor tries to keep as many
dwarves busy as possible, labormanager instead strives to get jobs done as
quickly as possible.

Labormanager frequently scans the current job list, current list of dwarves, and
the map to determine how many dwarves need to be assigned to what labors in
order to meet all current labor needs without starving any particular type of
job.

Dwarves on active military duty or dwarves assigned to burrows are left
untouched.

.. warning::

    As with autolabor, labormanager will override any manual changes you make to
    labors while it is enabled, including through other tools such as Dwarf
    Therapist. Do not run both autolabor and labormanager at the same time!

Usage
-----

::

    enable labormanager

Anything beyond this is optional - labormanager works well with the default
settings. Once you have enabled it in a fortress, it stays enabled until you
explicitly disable it, even if you save and reload your game.

The default priorities for each labor vary (some labors are higher priority by
default than others). The way the plugin works is that, once it determines how
many jobs of each labor are needed, it then sorts them by adjusted priority.
(Labors other than hauling have a bias added to them based on how long it's been
since they were last used to prevent job starvation.) The labor with the highest
priority is selected, the "best fit" dwarf for that labor is assigned to that
labor, and then its priority is *halved*. This process is repeated until either
dwarves or labors run out.

Because there is no easy way to detect how many haulers are actually needed at
any moment, the plugin always ensures that at least one dwarf is assigned to
each of the hauling labors, even if no hauling jobs are detected. At least one
dwarf is always assigned to construction removing and cleaning because these
jobs also cannot be easily detected. Lever pulling is always assigned to
everyone. Any dwarves for which there are no jobs will be assigned hauling,
lever pulling, and cleaning labors. If you use animal trainers, note that
labormanager will misbehave if you assign specific trainers to specific animals;
results are only guaranteed if you use "any trainer".

Labormanager also sometimes assigns extra labors to currently busy dwarfs so
that when they finish their current job, they will go off and do something
useful instead of standing around waiting for a job.

There is special handling to ensure that at least one dwarf is assigned to haul
food whenever food is detected left in a place where it will rot if not stored.
This will cause a dwarf to go idle if you have no stockpiles to haul food to.

Dwarves who are unable to work (child, in the military, wounded, handless,
asleep, in a meeting) are entirely excluded from labor assignment. Any dwarf
explicitly assigned to a burrow will also be completely ignored by labormanager.

The fitness algorithm for assigning jobs to dwarves generally attempts to favor
dwarves who are more skilled over those who are less skilled. It also tries to
avoid assigning female dwarfs with children to jobs that are "outside", favors
assigning "outside" jobs to dwarfs who are carrying a tool that could be used as
a weapon, and tries to minimize how often dwarves have to reequip.

Labormanager automatically determines medical needs and reserves health care
providers as needed. Note that this may cause idling if you have injured dwarves
but no or inadequate hospital facilities.

Hunting is never assigned without a butchery, and fishing is never assigned
without a fishery, and neither of these labors is assigned unless specifically
enabled (see below).

The method by which labormanager determines what labor is needed for a
particular job is complicated and, in places, incomplete. In some situations,
labormanager will detect that it cannot determine what labor is required. It
will, by default, pause and print an error message on the dfhack console,
followed by the message "LABORMANAGER: Game paused so you can investigate the
above message.". If this happens, please open an :issue:`<issue>` on GitHub,
reporting the lines that immediately preceded this message. You can tell
labormanager to ignore this error and carry on by running
``labormanager pause-on-error no``, but be warned that some job may go undone in
this situation.

Examples
--------

``labormanager priority BREWER 500``
    Boost the priority of brewing jobs.
``labormanager max FISH 1``
    Only assign fishing to one dwarf at a time. Note that you also have to run
    ``labormanager allow-fishing`` for any dwarves to be assigned fishing at
    all.

Advanced usage
--------------

``labormanager list``
    Show current priorities and current allocation stats. Use this command to
    see the IDs for all labors.
``labormanager status``
    Show basic status information.
``labormanager priority <labor> <value>``
    Set the priority value for labor <labor> to <value>.
``labormanager max <labor> <value>``
    Set the maximum number of dwarves that can be assigned to a labor.
``labormanager max <labor> none``
    Unrestrict the number of dwarves that can be assigned to a labor.
``labormanager max <labor> disable``
    Don't manage the specified labor. Dwarves who you have manually enabled this
    labor on will be less likely to have managed labors assigned to them.
``labormanager reset-all|reset <labor>``
    Return a labor (or all labors) to the default priority.
``labormanager allow-fishing|forbid-fishing``
    Allow/disallow fisherdwarves. *Warning* Allowing fishing tends to result in
    most of the fort going fishing. Fishing is forbidden by default.
``labormanager allow-hunting|forbid-hunting``
    Allow/disallow hunterdwarves. *Warning* Allowing hunting tends to result in
    as many dwarves going hunting as you have crossbows. Hunting is forbidden by
    default.
``labormanager pause-on-error yes|no``
    Make labormanager pause/continue if the labor inference engine fails. See
    the above section for details.
