enable
------

Activate a DFHack tool that has some persistent effect. Many plugins and scripts
can be in a distinct enabled or disabled state. Some of them activate and
deactivate automatically depending on the contents of the world raws. Others
store their state in world data. However a number of them have to be enabled
globally, and the init file is the right place to do it.

Most such plugins or scripts support the built-in ``enable`` and ``disable``
commands. Calling them at any time without arguments prints a list of enabled
and disabled plugins, and shows whether that can be changed through the same
commands. Passing plugin names to these commands will enable or disable the
specified plugins. For example, to enable the `manipulator` plugin::

  enable manipulator

It is also possible to enable or disable multiple plugins at once::

  enable manipulator search
