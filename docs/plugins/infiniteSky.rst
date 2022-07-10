infiniteSky
===========
Automatically allocates new z-levels of sky at the top of the map as you build up,
or on request allocates many levels all at once.

Usage:

``infiniteSky n``
  Raise the sky by n z-levels.
``infiniteSky enable/disable``
  Enables/disables monitoring of constructions. If you build anything in the second to highest z-level, it will allocate one more sky level. This is so you can continue to build stairs upward.

.. warning::

    :issue:`Sometimes <254>` new z-levels disappear and cause cave-ins.
    Saving and loading after creating new z-levels should fix the problem.
