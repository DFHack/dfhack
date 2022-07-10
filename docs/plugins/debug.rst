debug
=====
Manager for DFHack runtime debug prints. Debug prints are grouped by plugin name,
category name and print level. Levels are ``trace``, ``debug``, ``info``,
``warning`` and ``error``.

The runtime message printing is controlled using filters. Filters set the
visible messages of all matching categories. Matching uses regular expression syntax,
which allows listing multiple alternative matches or partial name matches.
This syntax is a C++ version of the ECMA-262 grammar (Javascript regular expressions).
Details of differences can be found at
https://en.cppreference.com/w/cpp/regex/ecmascript

Persistent filters are stored in ``dfhack-config/runtime-debug.json``.
Oldest filters are applied first. That means a newer filter can override the
older printing level selection.

Usage: ``debugfilter [subcommand] [parameters...]``

The following subcommands are supported:

help
----
Give overall help or a detailed help for a subcommand.

Usage: ``debugfilter help [subcommand]``

category
--------
List available debug plugin and category names.

Usage: ``debugfilter category [plugin regex] [category regex]``

The list can be filtered using optional regex parameters. If filters aren't
given then the it uses ``"."`` regex which matches any character. The regex
parameters are good way to test regex before passing them to ``set``.

filter
------
List active and passive debug print level changes.

Usage: ``debugfilter filter [id]``

Optional ``id`` parameter is the id listed as first column in the filter list.
If id is given then the command shows information for the given filter only in
multi line format that is better format if filter has long regex.

set
---
Creates a new debug filter to set category printing levels.

Usage: ``debugfilter set [level] [plugin regex] [category regex]``

Adds a filter that will be deleted when DF process exists or plugin is unloaded.

Usage: ``debugfilter set persistent [level] [plugin regex] [category regex]``

Stores the filter in the configuration file to until ``unset`` is used to remove
it.

Level is the minimum debug printing level to show in log.

* ``trace``: Possibly very noisy messages which can be printed many times per second

* ``debug``: Messages that happen often but they should happen only a couple of times per second

* ``info``: Important state changes that happen rarely during normal execution

* ``warning``: Enabled by default. Shows warnings about unexpected events which code managed to handle correctly.

* ``error``: Enabled by default. Shows errors which code can't handle without user intervention.

unset
-----
Delete a space separated list of filters

Usage: ``debugfilter unset [id...]``

disable
-------
Disable a space separated list of filters but keep it in the filter list

Usage: ``debugfilter disable [id...]``

enable
------
Enable a space sperate list of filters

Usage: ``debugfilter enable [id...]``
