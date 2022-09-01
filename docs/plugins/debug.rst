debug
=====

.. dfhack-tool::
    :summary: Provides commands for controlling debug log verbosity.
    :tags: dev
    :no-command:

.. dfhack-command:: debugfilter
    :summary: Configure verbosity of DFHack debug output.

Debug output is grouped by plugin name, category name, and verbosity level.

The verbosity levels are:

- ``Trace``
    Possibly very noisy messages which can be printed many times per second.
- ``Debug``
    Messages that happen often but they should happen only a couple of times per
    second.
- ``Info``
    Important state changes that happen rarely during normal execution.
- ``Warning``
    Enabled by default. Shows warnings about unexpected events which code
    managed to handle correctly.
- ``Error``
    Enabled by default. Shows errors which code can't handle without user
    intervention.

The runtime message printing is controlled using filters. Filters set the
visible messages of all matching categories. Matching uses regular expression
syntax, which allows listing multiple alternative matches or partial name
matches. This syntax is a C++ version of the ECMA-262 grammar (Javascript
regular expressions). Details of differences can be found at
https://en.cppreference.com/w/cpp/regex/ecmascript

Persistent filters are stored in ``dfhack-config/runtime-debug.json``. Oldest
filters are applied first. That means a newer filter can override the older
printing level selection.

Usage
-----

``debugfilter category [<plugin regex>] [<category regex>]``
    List available debug plugin and category names. If filters aren't givenm
    then all plugins/categories are matched. This command is a good way to test
    regex parameters before you pass them to ``set``.
``debugfilter filter [<id>]``
    List active and passive debug print level changes. The optional ``id``
    parameter is the id listed as first column in the filter list. If ``id`` is
    given, then the command shows extended information for the given filter
    only.
``debugfilter set [<level>] [<plugin regex>] [<category regex>]``
    Create a new debug filter to set category verbosity levels. This filter
    will not be saved when the DF process exists or the plugin is unloaded.
``debugfilter set persistent [<level>] [<plugin regex>] [<category regex>]``
    Store the filter in the configuration file to until ``unset`` is used to
    remove it.
``debugfilter unset <id> [<id> ...]``
    Delete a space separated list of filters.
``debugfilter disable <id> [<id> ...]``
    Disable a space separated list of filters but keep it in the filter list.
``debugfilter enable <id> [<id> ...]``
    Enable a space sperate list of filters.
``debugfilter header [enable] | [disable] [<element> ...]``
    Control which header metadata is shown along with each log message. Run it
    without parameters to see the list of configurable elements. Include an
    ``enable`` or ``disable``  keyword to change whether specific elements are
    shown.

Example
-------

``debugfilter set Warning core script``
    Hide script execution log messages (e.g. "Loading script:
    dfhack-config/dfhack.init"), which are normally output at Info verbosity
    in the "core" plugin with the "script" category.
