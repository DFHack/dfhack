#############
DFHack Readme
#############

.. contents::


==============
Getting DFHack
==============
The project is currently hosted at http://www.github.com/

Recent releases are available in source and binary formats `on the releases
page`_, while the binaries for releases 0.40.15-r1 to 0.34.11-r4 are on DFFD_.
Even older versions are available here_.

.. _`on the releases page`: http://github.com/DFHack/dfhack/releases
.. _DFFD: http://dffd.bay12games.com/search.php?string=DFHack&id=15
.. _here: http://dethware.org/dfhack/download

All new releases are announced in `the bay12 forums thread`_, which is also a
good place for discussion and questions.

.. _`the bay12 forums thread`: http://www.bay12forums.com/smf/index.php?topic=139553

If this sounds too complicated, you might prefer to `get a Starter Pack`_.
These are packages for new players or those who don't want to set up everything
themselves, and are available - with DFHack configured - for all OSes.

.. _`get a Starter Pack`: http://dwarffortresswiki.org/index.php/Utility:Lazy_Newb_Pack

Compatibility
=============
DFHack is available for Windows (XP or later), Linux (any modern distribution),
or OS X (10.6.8 to 10.9).

Most releases only support the version of DF mentioned in their title - for
example, DFHack 0.40.24-r2 only supports DF 0.40.24 - but some releases
support earlier DF versions as well.  Wherever possible, use the latest version
built for the target version of DF.

On Windows, DFHack is compatible with the SDL version of DF, but not the legacy version.

It is also possible to use the Windows DFHack with Wine under Linux and OS X.

Installation/Removal
====================
Installing DFhack involves copying files into your DF folder.
Copy the files from a release archive so that:

* On Windows, SDL.dll is replaced
* On Linux, the 'dfhack' script is placed in the same folder as the 'df' script

Uninstalling is basically the same, in reverse:

* On Windows, first delete SDL.dll and rename SDLreal.dll to SDL.dll. Then
  remove the other DFHack files
* On Linux, Remove the DFHack files.

The stonesense plugin might require some additional libraries on Linux.

If any of the plugins or dfhack itself refuses to load, check the stderr.log
file created in your DF folder.


===============
Getting started
===============
If DFHack is installed correctly, it will automatically pop up a console
window once DF is started as usual on windows. Linux and Mac OS X require
running the dfhack script from the terminal, and will use that terminal for
the console.

**NOTE**: The dfhack-run executable is there for calling DFHack commands in
an already running DF+DFHack instance from external OS scripts and programs,
and is *not* the way how you use DFHack normally.

DFHack has a lot of features, which can be accessed by typing commands in the
console, or by mapping them to keyboard shortcuts. Most of the newer and more
user-friendly tools are designed to be at least partially used via the latter
way.

In order to set keybindings, you have to create a text configuration file
called ``dfhack.init``; the installation comes with an example version called
``dfhack.init-example``, which is fully functional, covers all of the recent
features and can be simply renamed to ``dfhack.init``. You are encouraged to look
through it to learn which features it makes available under which key combinations.

For more information, refer to the rest of this document.

Using DFHack
============
DFHack basically extends what DF can do with something similar to the drop-down
console found in Quake engine games. On Windows, this is a separate command line
window. On linux, the terminal used to launch the dfhack script is taken over
(so, make sure you start from a terminal). Basic interaction with dfhack
involves entering commands into the console. For some basic instructions,
use the 'help' command. To list all possible commands, use the 'ls' command.
Many commands have their own help or detailed description. You can use
'command help' or 'command ?' to show that.

The command line has some nice line editing capabilities, including history
that's preserved between different runs of DF (use up/down keys to go through
the history).

The second way to interact with DFHack is to bind the available commands
to in-game hotkeys. The old way to do this is via the hotkey/zoom menu (normally
opened with the 'h' key). Binding the commands is done by assigning a command as
a hotkey name (with 'n').

A new and more flexible way is the keybinding command in the dfhack console.
However, bindings created this way are not automatically remembered between runs
of the game, so it becomes necessary to use the dfhack.init file to ensure that
they are re-created every time it is loaded.

Interactive commands like 'liquids' cannot be used as hotkeys.

Most of the commands come from plugins. Those reside in 'hack/plugins/'.

Something doesn't work, help!
=============================
First, don't panic :)

Second, dfhack keeps a few log files in DF's folder (``stderr.log`` and
``stdout.log``). Looking at these might help you solve the problem.
If it doesn't, you can ask for help in the forum thread or on IRC.

If you found a bug, you can report it in the Bay12 DFHack thread, the issues
tracker on github, or visit the #dfhack IRC channel on freenode.

Commands
========
DFHack command syntax consists of a command name, followed by arguments separated
by whitespace. To include whitespace in an argument, quote it in double quotes.
To include a double quote character, use ``\"`` inside double quotes.

If the first non-whitespace character of a line is ``#``, the line is treated
as a comment, i.e. a silent no-op command.

When reading commands from dfhack.init or with the ``script`` command, if the final character on a line is a backslash then the next uncommented line is considered a continuation of that line, with the backslash deleted.
Commented lines are skipped, so it is possible to comment out parts of a command with the ``#`` character.

If the first non-whitespace character is ``:``, the command is parsed in a special
alternative mode: first, non-whitespace characters immediately following the ``:``
are used as the command name; the remaining part of the line, starting with the first
non-whitespace character *after* the command name, is used verbatim as the first argument.
The following two command lines are exactly equivalent::

    :foo a b "c d" e f
    foo "a b \"c d\" e f"

This is intended for commands like ``rb_eval`` that evaluate script language statements.

Almost all the commands support using the 'help <command-name>' built-in command
to retrieve further help without having to look at this document. Alternatively,
some accept a 'help'/'?' option on their command line.

The init file
=============
DFHack allows users to automatically run commonly-used DFHack commands when DF is first loaded, when a game is loaded, and when a game is unloaded.
Init scripts function the same way they would if the user manually typed in their contents, but are much more convenient.
If your DF folder contains at least one file with a name following the format ``dfhack*.init`` where ``*`` is a placeholder for any string (including the empty string), then all such files are executed in alphabetical order as init scripts when DF is first loaded.
If your DF folder does not contain any such files, then DFHack will execute ``dfhack.init-example`` as an example of useful commands to be run automatically.
If you want DFHack to do nothing on its own, then create an empty ``dfhack.init`` file in the main DF directory, or delete ``dfhack.init-example``.
The file ``dfhack.init-example`` is included as an example for users to follow if they need DFHack command executed automatically.
We highly recommend modifying or deleting ``dfhack.init-example`` as its settings will not be optimal for all players.

In order to facilitate savegave portability, mod merging, and general organization of init files, DFHack supports multiple init files both in the main DF directory and save-specific init files in the save folders.
DFHack looks for init files in three places.
It will look for them in the main DF directory, and in ``data/save/_____/raw`` and ``data/save/_____/raw/objects`` where ``_____`` is the name of the current savegame.
When a game is loaded, DFHack looks for files of the form ``onLoad*.init``, where ``*`` can be any string, including the empty string.
When a game is unloaded, DFHack looks for files of the form ``onUnload*.init``.
Again, these files may be in any of the above three places.
All matching init files will be sorted and executed in alphebetical order.

Setting keybindings
===================
To set keybindings, use the built-in ``keybinding`` command. Like any other
command it can be used at any time from the console, but it is most useful
in the DFHack init file.

Currently, any combinations of Ctrl/Alt/Shift with A-Z, 0-9, or F1-F12 are supported.

Possible ways to call the command:

``keybinding list <key>``
  List bindings active for the key combination.
``keybinding clear <key> <key>...``
  Remove bindings for the specified keys.
``keybinding add <key> "cmdline" "cmdline"...``
  Add bindings for the specified key.
``keybinding set <key> "cmdline" "cmdline"...``
  Clear, and then add bindings for the specified key.

The *<key>* parameter above has the following *case-sensitive* syntax::

    [Ctrl-][Alt-][Shift-]KEY[@context[|context...]]

where the *KEY* part can be any recognized key and [] denote optional parts.

When multiple commands are bound to the same key combination, DFHack selects
the first applicable one. Later 'add' commands, and earlier entries within one
'add' command have priority. Commands that are not specifically intended for use
as a hotkey are always considered applicable.

The *context* part in the key specifier above can be used to explicitly restrict
the UI state where the binding would be applicable. If called without parameters,
the ``keybinding`` command among other things prints the current context string.
Only bindings with a *context* tag that either matches the current context fully,
or is a prefix ending at a '/' boundary would be considered for execution, i.e.
for context ``foo/bar/baz``, possible matches are any of ``@foo/bar/baz``, ``@foo/bar``,
``@foo`` or none. Multiple contexts can be specified by separating them with a
pipe (``|``) - for example, ``@foo|bar|baz/foo``.

Hotkeys
=======
Opens an in-game screen showing DFHack keybindings that are active in the current context.

.. image:: images/hotkeys.png

Type ``hotkeys`` into the DFHack console to open the screen, or bind the command to a
globally active hotkey.  The default keybinding is ``Ctrl-F1``.

Enabling plugins
================
Many plugins can be in a distinct enabled or disabled state. Some of
them activate and deactivate automatically depending on the contents
of the world raws. Others store their state in world data. However a
number of them have to be enabled globally, and the init file is the
right place to do it.

Most such plugins or scripts support the built-in ``enable`` and ``disable``
commands. Calling them at any time without arguments prints a list
of enabled and disabled plugins, and shows whether that can be changed
through the same commands.

To enable or disable plugins that support this, use their names as
arguments for the command::

  enable manipulator search


In-game interface tools
=======================
Some tools work by displaying dialogs or overlays in the game window.

In order to avoid user confusion, as a matter of policy all these tools
must display the word "DFHack" on the screen somewhere while active.
When that is not appropriate because they merely add keybinding hints to
existing DF screens, they deliberately use red instead of green for the key.

Game progress
=============

die
---
Instantly kills DF without saving.

quicksave
---------
Save immediately, without exiting.  Only available in fortress mode.

forcepause
----------
Forces DF to pause. This is useful when your FPS drops below 1 and you lose
control of the game.  Activate with ``forcepause 1``; deactivate with ``forcepause 0``.

hide / show
-----------
Hides or shows the DFHack terminal window, respectively.  To use ``show``, use
the in-game console (default keybinding ``Ctrl-Shift-P``).  Only available on Windows.

nopause
-------
Disables pausing (both manual and automatic) with the exception of pause forced
by 'reveal hell'. This is nice for digging under rivers.

Patched binaries
================
As an alternative to permanently modifying the binary, you can use the
``binpatch`` dfhack command to apply patches live in memory during a DF session.

In this case, updating symbols.xml is not necessary - otherwise Linux and OSX
users of patched binaries may have to find the relevant section in ``symbols.xml``,
and add a new line with the checksum of their executable::

    <md5-hash value='????????????????????????????????'/>

In order to find the correct value of the hash, look into stderr.log;
DFHack prints an error there if it does not recognize the hash.

DFHack includes a small stand-alone utility for applying and removing
binary patches from the game executable. Use it from the regular operating
system console:

``binpatch check "Dwarf Fortress.exe" patch.dif``
   Checks and prints if the patch is currently applied.

``binpatch apply "Dwarf Fortress.exe" patch.dif``
   Applies the patch, unless it is already applied or in conflict.

``binpatch remove "Dwarf Fortress.exe" patch.dif``
   Removes the patch, unless it is already removed.

The patches are expected to be encoded in text format used by IDA.
