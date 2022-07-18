script
------

Executes a batch file of DFHack commands. It reads a text file and runs each
line as a DFHack command as if it had been typed in by the user - treating the
input like `an init file <init-files>`.

Some other tools, such as `autobutcher` and `workflow`, export their settings as
the commands to create them - which can later be reloaded with ``script``.

Usage::

    script <filename>

Examples:

- ``script startup.txt``
    Executes the commands in ``startup.txt``, which exists in your DF game
    directory.
