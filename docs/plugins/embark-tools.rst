embark-tools
============

.. dfhack-tool::
    :summary: Extend the embark screen functionality.
    :tags: untested embark fort interface

Usage
-----

::

    enable embark-tools
    embark-tools enable|disable all
    embark-tools enable|disable <tool> [<tool> ...]

Available tools are:

``anywhere``
    Allows embarking anywhere (including sites, mountain-only biomes, and
    oceans). Use with caution.
``mouse``
    Implements mouse controls (currently in the local embark region only).
``sand``
    Displays an indicator when sand is present in the currently-selected area,
    similar to the default clay/stone indicators.
``sticky``
    Maintains the selected local area while navigating the world map.

Example
-------

``embark-tools enable all``
    Enable all embark screen extensions.
