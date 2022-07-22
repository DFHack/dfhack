sc-script
=========

Tags: `tag/system`
:dfhack-keybind:`sc-script`

:index:`Run commands when game state changes occur.
<sc-script; Run commands when game state changes occur.>` This is similar to
the static `init-files` but is slightly more flexible since it can be set
dynamically.

Usage:

- ``sc-script [help]``
    Show the list of valid event names.
- ``sc-script list [<event>]``
    List the currently registered files for all events or the specified event.
- ``sc-script add|remove <event> <file> [<file> ...]``
    Register or unregister a file to be run for the specified event.

Examples
--------

- ``sc-script add SC_MAP_LOADED spawn_extra_monsters.init``
    Registers the ``spawn_extra_monsters.init`` file to be run whenever a new
    map is loaded.
