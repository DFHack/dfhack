labormanager
============
Automatically manage dwarf labors to efficiently complete jobs.
Labormanager is derived from autolabor (above) but uses a completely
different approach to assigning jobs to dwarves. While autolabor tries
to keep as many dwarves busy as possible, labormanager instead strives
to get jobs done as quickly as possible.

Labormanager frequently scans the current job list, current list of
dwarfs, and the map to determine how many dwarves need to be assigned to
what labors in order to meet all current labor needs without starving
any particular type of job.

.. warning::

    *As with autolabor, labormanager will override any manual changes you
    make to labors while it is enabled, including through other tools such
    as Dwarf Therapist*

Simple usage:

:enable labormanager: Enables the plugin with default settings.
    (Persistent per fortress)

:disable labormanager: Disables the plugin.

Anything beyond this is optional - labormanager works fairly well on the
default settings.

The default priorities for each labor vary (some labors are higher
priority by default than others). The way the plugin works is that, once
it determines how many of each labor is needed, it then sorts them by
adjusted priority. (Labors other than hauling have a bias added to them
based on how long it's been since they were last used, to prevent job
starvation.) The labor with the highest priority is selected, the "best
fit" dwarf for that labor is assigned to that labor, and then its
priority is *halved*. This process is repeated until either dwarfs or
labors run out.

Because there is no easy way to detect how many haulers are actually
needed at any moment, the plugin always ensures that at least one dwarf
is assigned to each of the hauling labors, even if no hauling jobs are
detected. At least one dwarf is always assigned to construction removing
and cleaning because these jobs also cannot be easily detected. Lever
pulling is always assigned to everyone. Any dwarfs for which there are
no jobs will be assigned hauling, lever pulling, and cleaning labors. If
you use animal trainers, note that labormanager will misbehave if you
assign specific trainers to specific animals; results are only guaranteed
if you use "any trainer", and animal trainers will probably be
overallocated in any case.

Labormanager also sometimes assigns extra labors to currently busy
dwarfs so that when they finish their current job, they will go off and
do something useful instead of standing around waiting for a job.

There is special handling to ensure that at least one dwarf is assigned
to haul food whenever food is detected left in a place where it will rot
if not stored. This will cause a dwarf to go idle if you have no
storepiles to haul food to.

Dwarfs who are unable to work (child, in the military, wounded,
handless, asleep, in a meeting) are entirely excluded from labor
assignment. Any dwarf explicitly assigned to a burrow will also be
completely ignored by labormanager.

The fitness algorithm for assigning jobs to dwarfs generally attempts to
favor dwarfs who are more skilled over those who are less skilled. It
also tries to avoid assigning female dwarfs with children to jobs that
are "outside", favors assigning "outside" jobs to dwarfs who are
carrying a tool that could be used as a weapon, and tries to minimize
how often dwarfs have to reequip.

Labormanager automatically determines medical needs and reserves health
care providers as needed. Note that this may cause idling if you have
injured dwarfs but no or inadequate hospital facilities.

Hunting is never assigned without a butchery, and fishing is never
assigned without a fishery, and neither of these labors is assigned
unless specifically enabled.

The method by which labormanager determines what labor is needed for a
particular job is complicated and, in places, incomplete. In some
situations, labormanager will detect that it cannot determine what labor
is required. It will, by default, pause and print an error message on
the dfhack console, followed by the message "LABORMANAGER: Game paused
so you can investigate the above message.". If this happens, please open
an issue on github, reporting the lines that immediately preceded this
message. You can tell labormanager to ignore this error and carry on by
typing ``labormanager pause-on-error no``, but be warned that some job may go
undone in this situation.

Advanced usage:

:labormanager enable:                      Turn plugin on.
:labormanager disable:                     Turn plugin off.
:labormanager priority <labor> <value>:    Set the priority value (see above) for labor <labor> to <value>.
:labormanager reset <labor>:               Reset the priority value of labor <labor> to its default.
:labormanager reset-all:                   Reset all priority values to their defaults.
:labormanager allow-fishing:               Allow dwarfs to fish. *Warning* This tends to result in most of the fort going fishing.
:labormanager forbid-fishing:              Forbid dwarfs from fishing. Default behavior.
:labormanager allow-hunting:               Allow dwarfs to hunt. *Warning* This tends to result in as many dwarfs going hunting as you have crossbows.
:labormanager forbid-hunting:              Forbid dwarfs from hunting. Default behavior.
:labormanager list:                        Show current priorities and current allocation stats.
:labormanager pause-on-error yes:          Make labormanager pause if the labor inference engine fails. See above.
:labormanager pause-on-error no:           Allow labormanager to continue past a labor inference engine failure.
