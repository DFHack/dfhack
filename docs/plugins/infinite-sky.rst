.. _infinitesky:

infinite-sky
============

.. dfhack-tool::
    :summary: Automatically allocate new z-levels of sky.
    :tags: fort auto design map

If enabled, this plugin will automatically allocate new z-levels of sky at the
top of the map as you build up. Or it can allocate one or many additional levels
at your command.

Usage
-----

``enable infinite-sky``
    Enables monitoring of constructions. If you build anything in the second
    highest z-level, it will allocate one more sky level. You can build stairs
    up as high as you like!
``infinite-sky [<n>]``
    Raise the sky by n z-levels. If run without parameters, raises the sky by
    one z-level.

.. warning::

    :issue:`Sometimes <254>` new z-levels disappear and cause cave-ins.
    Saving and loading after creating new z-levels should fix the problem.
