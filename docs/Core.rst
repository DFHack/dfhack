.. _dfhack-core:

###########
DFHack Core
###########

.. contents:: Contents
  :local:
  :depth: 2


Command implementation
======================
DFHack commands can be implemented in any of three ways:

:builtin:   commands are implemented by the core of DFHack. They manage
            other DFHack tools, interpret commands, and control basic
            aspects of DF (force pause or quit).

:plugins:   are stored in ``hack/plugins/`` and must be compiled with the
            same version of DFHack.  They are less flexible than scripts,
            but used for complex or ongoing tasks because they run faster.

:scripts:   are Lua scripts stored in ``hack/scripts/`` or other
            directories in the `script-paths`. Because they don't need to
            be compiled, scripts are more flexible about versions, and
            they are easier to distribute. Most third-party DFHack addons
            are scripts.

All tools distributed with DFHack are documented `here <tools>`.

Using DFHack commands
=====================
DFHack commands can be executed in a number of ways:

#. Typing the command into the DFHack console (see below)
#. From the OS terminal (see below)
#. Pressing a key combination set up with `keybinding`
#. From one of several `init-files`, automatically
#. Using `script` to run a batch of commands from a file
#. From an in-game command launcher interface like `gui/launcher`, the
   `hotkeys` overlay widget, or `gui/quickcmd`.

The DFHack console
------------------
The command line has some nice line editing capabilities, including history
that's preserved between different runs of DF - use :kbd:`↑` and :kbd:`↓`
to go through the history.

To include whitespace in the argument/s to some command, quote it in
double quotes.  To include a double quote character, use ``\"``.

If the first non-whitespace character is ``:``, the command is parsed in
an alternative mode.  The non-whitespace characters following the ``:`` are
the command name, and the remaining part of the line is used verbatim as
the first argument.  This is very useful for the `lua` and `rb` commands.
As an example, the following two command lines are exactly equivalent::

  :foo a b "c d" e f
  foo "a b \"c d\" e f"

Using an OS terminal
--------------------
DFHack commands can be run from an OS terminal at startup, using '+ args',
or at any other time using the ``dfhack-run`` executable.

If DF/DFHack is started with arguments beginning with ``+``, the remaining
text is treated as a command in the DFHack console.  It is possible to use
multiple such commands, which are split on ``+``.  For example:

.. code-block:: shell

    ./dfhack +load-save region1
    "Dwarf Fortress.exe" +devel/print-args Hello! +enable workflow

The first example (\*nix), `load-save`, skips the main menu and loads
``region1`` immediately.  The second (Windows) example prints
:guilabel:`Hello!` in the DFHack console, and `enables <enable>` `workflow`.
Note that the ``:foo`` syntax for whitespace in arguments is not compatible \
with '+ args'.


.. _dfhack-run:

dfhack-run
..........

If DF and DFHack are already running, calling ``dfhack-run my command``
in an external terminal is equivalent to calling ``my command`` in the
DFHack console.  Direct use of the DFHack console is generally easier,
but ``dfhack-run`` can be useful in a variety of circumstances:

- if the console is unavailable

  - with the init setting ``PRINT_MODE:TEXT``
  - while running an interactive command (e.g. `liquids` or `tiletypes`)

- from external programs or scripts
- if DF or DFHack are not responding

Examples:

.. code-block:: shell

    ./dfhack-run cursecheck
    dfhack-run kill-lua

The first (\*nix) example `checks for vampires <cursecheck>`; the
second (Windows) example uses `kill-lua` to stop a Lua script.

.. note::

  ``dfhack-run`` attempts to connect to a server on TCP port 5000. If DFHack
  was unable to start this server, ``dfhack-run`` will not be able to connect.
  This could happen if you have other software listening on port 5000, or if
  you have multiple copies of DF running simultaneously. To assign a different
  port, see `remote-server-config`.

.. _dfhack-config:

Configuration files
===================

Most DFHack settings can be changed by modifying files in the ``dfhack-config``
folder (which is in the DF folder). The default versions of these files, if they
exist, are in ``dfhack-config/default`` and are installed when DFHack starts if
necessary.

.. _init-files:

Init files
----------

.. contents::
   :local:

DFHack allows users to automatically run commonly-used DFHack commands
when DF is first loaded, when a world is loaded, when a map is loaded, when a
map is unloaded, and when a world is unloaded.

Init scripts function the same way they would if the user manually typed
in their contents, but are much more convenient.  In order to facilitate
savegave portability, mod merging, and general organization of init files,
DFHack supports multiple init files both in the main DF directory and
save-specific init files in the save folders.

DFHack looks for init files in two places each time they could be run:

#. The :file:`dfhack-config/init` subdirectory in the main DF directory and
#. :file:`save/{world}/init`, where ``{world}`` is the current save

For each of those directories, all matching init files will be executed in
alphabetical order.

Before running matched init scripts in any of those locations, the
:file:`dfhack-config/init/default.*` file that matches the event will be run to
load DFHack defaults. Only the :file:`dfhack-config/init` directory is checked
for this file, not any :file:`save` directories. If you want DFHack to load
without running any of its default configuration commands, edit the
:file:`dfhack-config/init/default.*` files and comment out the commands you see
there.

When reading commands from the init files or with the `script` command, if the
final character on a line is a backslash then the next uncommented line is
considered a continuation of that line, with the backslash deleted.  Commented
lines are skipped, so it is possible to comment out parts of a command with the
``#`` character.

.. _dfhack.init:

dfhack\*.init
.............
On startup, DFHack looks for files of the form ``dfhack*.init`` (where ``*`` is
a placeholder for any string, including the empty string).

These files are best used for keybindings and enabling persistent tools
which do not require a world to be loaded.


.. _onLoad.init:

onLoad\*.init
.............
When a world is loaded, DFHack looks for files of the form ``onLoad*.init``,
where ``*`` can be any string, including the empty string.

A world being loaded can mean a fortress, an adventurer, or legends mode.

These files are best used for non-persistent commands, such as setting
a `bugfix-tag-index` script to run on `repeat`.


.. _onMapLoad.init:

onMapLoad\*.init
................
When a map is loaded, either in adventure or fort mode, DFHack looks for files
of the form ``onMapLoad*.init``, where ``*`` can be any string, including the
empty string.

These files are best used for commands that are only relevant once there is a
game map loaded.


.. _onMapUnload.init:
.. _onUnload.init:

onMapUnload\*.init and onUnload\*.init
......................................
When a map or world is unloaded, DFHack looks for files of the form
``onMapUnload*.init`` or ``onUnload*.init``, respectively.

Modders often use unload init scripts to disable tools which should not run
after a modded save is unloaded.


.. _other_init_files:

init.d/\*.lua
.............

Any lua script named ``init.d/*.lua``, in the save or main DF directory,
will be run when any world or that save is loaded.


.. _script-paths:

Script paths
------------

Script paths are folders that DFHack searches to find a script when a command is
run. By default, the following folders are searched, in order (relative to the
root DF folder):

#. :file:`dfhack-config/scripts`
#. :file:`save/{world}/scripts` (only if a save is loaded)
#. :file:`hack/scripts`

For example, if ``teleport`` is run, these folders are searched in order for
``teleport.lua``, and the first matching file is run.

Script paths can be added by modifying :file:`dfhack-config/script-paths.txt`.
Each line should start with one of these characters:

- ``+``: adds a script path that is searched *before* the default paths (above)
- ``-``: adds a script path that is searched *after* the default paths
- ``#``: a comment (the line is ignored)

Paths can be absolute or relative - relative paths are interpreted relative to
the root DF folder.

.. admonition:: Tip

    When developing scripts in the :source-scripts:`dfhack/scripts repo <>`,
    it may be useful to add the path to your local copy of the repo with ``+``.
    This will allow you to make changes in the repo and have them take effect
    immediately, without needing to re-install or copy scripts over manually.

Note that ``script-paths.txt`` is only read at startup, but the paths can also be
modified programmatically at any time through the `Lua API <lua-api-internal>`.


.. _env-vars:

Environment variables
=====================

DFHack's behavior can be adjusted with some environment variables. For example,
on UNIX-like systems:

.. code-block:: shell

  DFHACK_SOME_VAR=1 ./dfhack

- ``DFHACK_PORT``: the port to use for the RPC server (used by ``dfhack-run``
  and `remotefortressreader` among others) instead of the default ``5000``. As
  with the default, if this port cannot be used, the server is not started.
  See `remote` for more details.

- ``DFHACK_DISABLE_CONSOLE``: if set, the DFHack console is not set up. This is
  the default behavior if ``PRINT_MODE:TEXT`` is set in ``data/init/init.txt``.
  Intended for situations where DFHack cannot run in a terminal window.

- ``DFHACK_HEADLESS``: if set, and ``PRINT_MODE:TEXT`` is set, DF's display will
  be hidden, and the console will be started unless ``DFHACK_DISABLE_CONSOLE``
  is also set. Intended for non-interactive gameplay only.

- ``DFHACK_NO_GLOBALS``, ``DFHACK_NO_VTABLES``: ignores all global or vtable
  addresses in ``symbols.xml``, respectively. Intended for development use -
  e.g. to make sure tools do not crash when these addresses are missing.

- ``DFHACK_NO_DEV_PLUGINS``: if set, any plugins from the plugins/devel folder
  that are built and installed will not be loaded on startup.

- ``DFHACK_LOG_MEM_RANGES`` (macOS only): if set, logs memory ranges to
  ``stderr.log``. Note that `devel/lsmem` can also do this.

- ``DFHACK_ENABLE_LUACOV``: if set, enables coverage analysis of Lua scripts.
  Use the `devel/luacov` script to generate coverage reports from the collected
  metrics.

Other (non-DFHack-specific) variables that affect DFHack:

- ``TERM``: if this is set to ``dumb`` or ``cons25`` on \*nix, the console will
  not support any escape sequences (arrow keys, etc.).

- ``LANG``, ``LC_CTYPE``: if either of these contain "UTF8" or "UTF-8" (not case
  sensitive), ``DF2CONSOLE()`` will produce UTF-8-encoded text. Note that this
  should be the case in most UTF-8-capable \*nix terminal emulators already.

Miscellaneous notes
===================
This section is for odd but important notes that don't fit anywhere else.

* If a DF :kbd:`H` hotkey is named with a DFHack command, pressing
  the corresponding :kbd:`Fx` button will run that command, instead of
  zooming to the set location.
  *This feature will be removed in a future version.*  (see :issue:`731`)

* The binaries for 0.40.15-r1 to 0.34.11-r4 are on DFFD_.
  Older versions are available here_.
  *These files will eventually be migrated to GitHub.*  (see :issue:`473`)

  .. _DFFD: https://dffd.bay12games.com/search.php?string=DFHack&id=15&limit=1000
  .. _here: https://dethware.org/dfhack/download
