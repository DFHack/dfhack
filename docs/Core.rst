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
the first argument.  This is very useful for the `lua` command.
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
#. :file:`data/installed_mods/...` (see below)

For example, if ``teleport`` is run, these folders are searched in order for
``teleport.lua``, and the first matching file is run.

Scripts in installed mods
.........................

Scripts in mods are automatically added to the script path. The following
directories are searched for mods::

    ../../workshop/content/975370/ (the DF Steam workshop directory)
    mods/
    data/installed_mods/

Each mod can have two directories that contain scripts:

- ``scripts_modactive/`` is added to the script path if and only if the mod is
    active in the loaded world.
- ``scripts_modinstalled/`` is added to the script path as long as the mod is
    installed in one of the searched mod directories.

Multiple versions of a mod may be installed at the same time. If a mod is
active in a loaded world, then the scripts for the version of the mod that is
active will be added to the script path. Otherwise, the latest version of each
mod is added to the script path.

Scripts for active mods take precedence according to their load order when you
generated the current world.

Scripts for non-active mods are ordered by their containing mod's ID.

For example, the search paths for mods might look like this::

    activemod_last_in_load_order/scripts_modactive
    activemod_last_in_load_order/scripts_modinstalled
    activemod_second_to_last_in_load_order/scripts_modactive
    activemod_second_to_last_in_load_order/scripts_modinstalled
    ...
    inactivemod1/scripts_modinstalled
    inactivemod2/scripts_modinstalled
    ...

Not all mods will have script directories, of course, and those mods will not be
added to the script search path. Mods are re-scanned whenever a world is loaded
or unloaded. For more information on scripts and mods, check out the
`modding-guide`.

Custom script paths
...................

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

Commandline options
===================

In addition to `Using an OS terminal`_ to execute commands on startup, DFHack
also recognizes a single commandline option that can be specified on the
commandline:

- ``--disable-dfhack``: If this option is passed on the Dwarf Fortress
  commandline, then DFHack will be disabled for the session. You will have to
  restart Dwarf Fortress without specifying this option in order to use DFHack.
  If you are launching Dwarf Fortress from Steam, you can enter the option in
  the "Launch Options" text box in the properties for the Dwarf Fortress app.
  Note that if you do this, DFHack will be disabled regardless of whether you
  run Dwarf Fortress from its own app or DFHack's. You will have to clear the
  DF Launch Options in order to use DFHack again. Note that even if DFHack is
  disabled, :file:`stdout.txt` and :file:`stderr.txt` will still be redirected
  to :file:`stdout.log` and :file:`stderr.log`, respectively.

.. _env-vars:

Environment variables
=====================

DFHack's behavior can be adjusted with some environment variables. For example,
on UNIX-like systems:

.. code-block:: shell

  DFHACK_SOME_VAR=1 ./dfhack

- ``DFHACK_DISABLE``: if set, DFHack will not initialize, not even to redirect
  standard output or standard error. This is provided as an alternative
  to the ``--disable-dfhack`` commandline parameter above for when environment
  variables are more convenient.

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

Core preferences
================

There are a few settings that can be changed dynamically via
`gui/control-panel` to affect runtime behavior. You can also toggle these from
the commandline using the `lua` command, e.g.
``lua dfhack.HIDE_ARMOK_TOOLS=true`` or by editing the generated
``dfhack-config/init/dfhack.control-panel-preferences.init`` file and
restarting DF.

- ``dfhack.HIDE_CONSOLE_ON_STARTUP``: Whether to hide the external DFHack
  terminal window on startup. This, of course, is not useful to change
  dynamically. You'll have to use `gui/control-panel` or edit the init file
  directly and restart DF for it to have an effect.

- ``dfhack.HIDE_ARMOK_TOOLS``: Whether to hide "armok" tools in command lists.

Performance monitoring
======================

Though DFHack tools are generally performant, they do take some amount of time
to run. DFHack tracks its impact on DF game speed so the DFHack team can be
aware of (and fix) tools that are taking more than their fair share of
processing time.

The target threshold for DFHack CPU utilization during unpaused gameplay with
all overlays and automation tools enabled is 10%. This is about the level where
players would notice the impact. In general, DFHack will have even less impact
for most players, since it is not common for every single DFHack tool to be
enabled at once.

DFHack will record a performance report with the savegame files named
``dfhack-perf-counters.dat``. The report contains measurements from when the
game was loaded to the time when it was saved. By default, only unpaused time is
measured (since processing done while the game is paused doesn't slow anything
down from the player's perspective). You can display a live report at any time
by running::

    :lua require('script-manager').print_timers()

You can reset the timers to start a new measurement session by running::

    :lua dfhack.internal.resetPerfCounters()

If you want to record performance over all elapsed time, not just unpaused
time, then instead run::

    :lua dfhack.internal.resetPerfCounters(true)
