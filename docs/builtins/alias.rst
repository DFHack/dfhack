alias
-----

Configure helper aliases for other DFHack commands. Aliases are resolved
immediately after built-in commands, which means that an alias cannot override
a built-in command, but can override a command implemented by a plugin or
script.

Usage:

- ``alias list``
  lists all configured aliases
- ``alias add <name> <command> [arguments...]``
  adds an alias
- ``alias replace <name> <command> [arguments...]``
  replaces an existing alias with a new command, or adds the alias if it does
  not already exist
- ``alias delete <name>``
  removes the specified alias

Aliases can be given additional arguments when created and invoked, which will
be passed to the underlying command in order.

Example::

    [DFHack]# alias add pargs devel/print-args example
    [DFHack]# pargs text
    example
    text
