channel-safely
==============

.. dfhack-tool::
    :summary: Auto-manage channel designations to keep dwarves safe.
    :tags: unavailable

Multi-level channel projects can be dangerous, and managing the safety of your
dwarves throughout the completion of such projects can be difficult and time
consuming. This plugin keeps your dwarves safe (at least while channeling) so you don't
have to. Now you can focus on designing your dwarven cities with the deep chasms
they were meant to have.

Usage
-----

::

    enable channel-safely
    channel-safely set <setting> <value>
    channel-safely enable|disable <feature>
    channel-safely <command>

When enabled the map will be scanned for channel designations which will be grouped
together based on adjacency and z-level. These groups will then be analyzed for safety
and designations deemed unsafe will be put into :wiki:`Marker Mode <Designations_menu#Marker_Mode>`.
Each time a channel designation is completed its group status is checked, and if the group
is complete pending groups below are made active again.

Features and settings once set will persist until you change them, even if you save and reload your game.

Examples
--------

``channel-safely``
    The plugin reports its configured status.

``channel-safely runonce``
    Runs the safety procedures once. You can use this if you prefer initiating scans manually.

``channel-safely disable require-vision``
    Allows the plugin to read all tiles, including the ones your dwarves know nothing about.

``channel-safely enable monitor``
    Enables monitoring active channel digging jobs. Meaning that if another unit it present
    or the tile below becomes open space the job will be paused or canceled (respectively).

``channel-safely set ignore-threshold 3``
    Configures the plugin to ignore designations equal to or above priority 3 designations.

Commands
--------

:runonce:           Run the safety procedures once to set the marker mode of designations.
:rebuild:           Rebuild the designation group data. You should also read Troubleshooting.

Features
--------

:require-vision:    Toggle whether the dwarves need vision of a tile before channeling to it can be deemed unsafe. (default: enabled)
:risk-averse:       Toggles whether to use cave-in prevention. Designations are activated in stages
                    and their priorities along edges are modified. (default: enabled)
:monitoring:        Toggle whether to monitor the conditions of active digs. (default: disabled)
:resurrect:         Toggle whether to resurrect units involved in cave-ins, and if monitor is enabled
                    units who die while digging. (default: disabled)

Settings
--------

:refresh-freq:      The rate at which full refreshes are performed.
                    This can be expensive if you're undertaking many mega projects. (default:600, twice a day)
:monitor-freq:      The rate at which active jobs are monitored. (default:1)
:ignore-threshold:  Sets the priority threshold below which designations are processed. You can set to 1 or 0 to
                    effectively disable the scanning. (default: 5)
:fall-threshold:    Sets the fall threshold beyond which is considered unsafe. (default: 1)

Troubleshooting
---------------

If designations aren't switching correctly, try putting the designations into marker mode.
Then press . (next) or resume. If you're debugging code you'll want these::

    debugfilter set Info channel manager
    debugfilter set Debug channel plugin
    debugfilter set Trace channel group
