hotkeys
=======

.. dfhack-tool::
    :summary: Show all DFHack keybindings for the current context.
    :tags: dfhack

The command opens an in-game screen showing which DFHack keybindings are active
in the current context. See also `hotkey-notes`.

Usage
-----

``hotkeys``
    Show the list of keybindings for the current context in an in-game menu.
``hotkeys list``
    List the keybindings to the console.

Menu overlay widget
-------------------

The in-game hotkeys menu is registered with the `overlay` framework and appears
as a DFHack logo in the upper-left corner of the screen. You can bring up the
menu by clicking on the logo or by hitting the global :kbd:`Ctrl`:kbd:`Shift`:kbd:`c` hotkey. You can select a command to run from
the list by clicking on it with the mouse or by using the keyboard to select a
command with the arrow keys and hitting :kbd:`Enter`.

The menu closes automatically when an action is taken or when you click or
right click anywhere else on the screen.

A short description of the command will appear in a nearby textbox. If you'd
like to see the full help text for the command or edit the command before
running, you can open it for editing in `gui/launcher` by shift clicking on the
command, left clicking on the arrow to the left of the command, or by pressing
the right arrow key while the command is selected.
