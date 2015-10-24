##############
DFHack Scripts
##############

Lua or ruby scripts placed in the ``hack/scripts/`` directory are considered for
execution as if they were native DFHack commands. They are listed at the end
of the ``ls`` command output.

Note: scripts in subdirectories of hack/scripts/ can still be called, but will
only be listed by ls if called as ``ls -a``. This is intended as a way to hide
scripts that are obscure, developer-oriented, or should be used as keybindings
or from the init file.

``kill-lua`` stops any currently-running Lua scripts. By default, scripts can
only be interrupted every 256 instructions. Use ``kill-lua force`` to interrupt
the next instruction.


The following pages document all the standard DFHack scripts.

* Basic scripts are not stored in any subdirectory, and can be invoked directly.
  They are generally useful tools for any player.
* ``devel/*`` scripts are intended for developers, or still substantially unfinished.
  If you don't already know what they do, best to leave them alone.
* ``fix/*`` scripts fix various bugs and issues, some of them obscure.
* ``gui/*`` scripts implement dialogs in the main game window.

  In order to avoid user confusion, as a matter of policy all these tools
  display the word "DFHack" on the screen somewhere while active.
  When that is not appropriate because they merely add keybinding hints to
  existing DF screens, they deliberately use red instead of green for the key.

* ``modtools/*`` scripts provide tools for modders, often with changes
  to the raw files, and are not intended to be called manually by end-users.

  These scripts are mostly useful for raw modders and scripters. They all have
  standard arguments: arguments are of the form ``tool -argName1 argVal1
  -argName2 argVal2``. This is equivalent to ``tool -argName2 argVal2 -argName1
  argVal1``. It is not necessary to provide a value to an argument name: ``tool
  -argName3`` is fine. Supplying the same argument name multiple times will
  result in an error. Argument names are preceded with a dash. The ``-help``
  argument will print a descriptive usage string describing the nature of the
  arguments. For multiple word argument values, brackets must be used: ``tool
  -argName4 [ sadf1 sadf2 sadf3 ]``. In order to allow passing literal braces as
  part of the argument, backslashes are used: ``tool -argName4 [ \] asdf \foo ]``
  sets ``argName4`` to ``\] asdf foo``. The ``*-trigger`` scripts have a similar
  policy with backslashes.


.. toctree::
   :maxdepth: 2

   /docs/_auto/base
   /docs/_auto/devel
   /docs/_auto/fix
   /docs/_auto/gui
   /docs/_auto/modtools


.. warning::

    The following documentation is pulled in from external ``README`` files.
    
    - it may not match the DFHack docs style
    - some scripts have incorrect names, or are documented but unavailable
    - some scripts have entries here and are also included above

.. toctree::
   :glob:
   :maxdepth: 2

   /scripts/3rdparty/*/README

