.. include:: <isonum.txt>

========
Maps API
========

DFHack offers several ways to access and manipulate map data.

* C++: the ``Maps`` and ``MapCache`` modules
* Lua: the `dfhack.maps module <lua-maps>`_
* All languages: the ``map`` field of the ``world`` global contains raw map data
  when the world is loaded.

.. note::

    This page will eventually go into more detail about the available APIs.
    For now, it is just an overview of how DF map data is structured.

.. contents:: Contents
    :local:

Tiles
=====

The DF map has several types of tiles:

- **Local tiles** are at the smallest scale. In regular fortress/adventure mode
  play, the cursor takes up 1 local tile.

  Objects that use local tile coordinates include:

  - Units
  - Items
  - Projectiles

- **Blocks** are 16 |times| 16 |times| 1 groups of local tiles. Internally, many
  tile details are stored at the block level for space-efficiency reasons.
  Blocks are visible during zoomed-in fast travel in adventure mode.

  Objects that use block coordinates include:

  - Armies

- **Region tiles** are 3 |times| 3 groups of columns of blocks (they span the
  entire z-axis), or 48 |times| 48 columns of local tiles. DF sometimes refers
  to these as "mid-level tiles" (MLTs). Region tiles are visible when resizing
  a fortress before embarking, or in zoomed-out fast travel in adventure mode.

- **World tiles** are

  - 16 |times| 16 groups of region tiles, or
  - 48 |times| 48 groups of columns of blocks, or
  - 768 |times| 768 groups of columns of local tiles

  World tiles are visible on the world map before embarking, as well as in the
  civilization map in fortress mode and the quest log in adventure mode.

- Some map features are stored in 16 |times| 16 groups of world tiles, sometimes
  referred to as "feature shells".
