smooth-movement
===============

.. dfhack-tool::
    :summary: Smoothly animate movement in the fortress viewport.
    :tags: fort interface

This plugin makes creatures, hauled items, vehicles, and associated status
icons glide between tiles. It only changes rendering; gameplay, simulation
timing, and save data are unaffected.

The plugin requires the SDL 2D renderer.

Usage
-----

::

    enable smooth-movement
    smooth-movement
    smooth-movement flip on|off
    smooth-movement zlevel on|off

Running ``smooth-movement`` without arguments reports the plugin status and
current settings. Optional features are reset to their defaults whenever the
plugin is enabled.

Examples
--------

``enable smooth-movement``
    Enable smooth movement on the main z-level.

``smooth-movement flip on``
    Horizontally flip creatures so they face their direction of travel.

``smooth-movement zlevel on``
    Also animate movement on visible z-levels below the main level.

Settings
--------

``flip`` (default: off)
    Flip creature sprites horizontally according to their last horizontal
    movement. Items, vehicles, and designation icons keep their normal
    orientation.

``zlevel`` (default: off)
    Animate movement on visible lower z-levels. Sprite flipping remains
    independent of this setting.
