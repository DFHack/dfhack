load
====

.. dfhack-tool::
    :summary: Load and register a plugin library.
    :tags: dfhack

Also see `unload` and `reload` for related actions.

Usage
-----

::

    load <plugin> [<plugin> ...]
    load -a|--all

You can load individual named plugins or all plugins at once. Note that plugins
are disabled after loading/reloading until you explicitly `enable` them.
