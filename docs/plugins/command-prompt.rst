command-prompt
==============
**Tags:** `tag/system`
:dfhack-keybind:`command-prompt`

:index:`An in-game DFHack terminal where you can enter other commands.
<command-prompt; An in-game DFHack terminal where you can enter other commands.>`

Usage::

    command-prompt [entry]

If called with parameters, it starts with that text in the command edit area.
This is most useful for developers, who can set a keybinding to open a laungage
interpreter for lua or Ruby by starting with the `:lua <lua>` or `:rb <rb>`
portions of the command already filled in.

Also see `gui/quickcmd` for a different take on running commands from the UI.

.. image:: ../images/command-prompt.png
