ls
==

.. dfhack-tool::
    :summary: List available DFHack commands.
    :tags: dfhack

In order to group related commands, each command is associated with a list of
tags. You can filter the listed commands by a tag or a substring of the
command name. Can also be invoked as ``dir``.

Usage:

``ls [<options>]``
    Lists all available commands and the tags associated with them.
``ls <tag> [<options>]``
    Shows only commands that have the given tag. Use the `tags` command to see
    the list of available tags.
``ls <string> [<options>]``
    Shows commands that include the given string. E.g. ``ls quick`` will show
    all the commands with "quick" in their names. If the string is also the
    name of a tag, then it will be interpreted as a tag name.

Examples
--------

- ``ls adventure``
    Lists all commands with the ``adventure`` tag.
- ``ls --dev trigger``
    Lists all commands, including developer and modding commands, that match the
    substring "trigger"

Options
-------

``--notags``
    Don't print out the tags associated with each command.
``--dev``
    Include commands intended for developers and modders.
