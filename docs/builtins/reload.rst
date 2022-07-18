reload
------

Reload a loaded plugin. Developer use this command to reload a plugin that they
are actively modifying. Also see `load` and `unload` for related actions.

Usage::

    reload <plugin> [<plugin> ...]
    reload -a|--all

You can reload individual named plugins or all plugins at once. Note that
plugins are disabled after loading/reloading until you explicitly `enable`
them.
