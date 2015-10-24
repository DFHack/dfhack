##############
DFHack Scripts
##############

Lua or ruby scripts placed in the ``hack/scripts/`` directory are considered for
execution as if they were native DFHack commands. They are listed at the end
of the ``ls`` command output.

Note: scripts in subdirectories of hack/scripts/ can still be called, but will
only be listed by ls if called as ``ls -a``. This is intended as a way to hide
scripts that are obscure, developer-oriented, or should be used as keybindings
or from the init file.  See the page for each type for details.

``kill-lua`` stops any currently-running Lua scripts. By default, scripts can
only be interrupted every 256 instructions. Use ``kill-lua force`` to interrupt
the next instruction.

The following pages document all the standard DFHack scripts.

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

