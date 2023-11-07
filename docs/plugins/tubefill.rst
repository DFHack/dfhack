tubefill
========

.. dfhack-tool::
    :summary: Replenishes mined-out adamantine.
    :tags: fort armok map

This tool replaces mined-out tiles of adamantine spires with fresh, undug
adamantine walls, ready to be re-harvested. Empty tiles within the spire that
used to contain special gemstones, obsidian, water, or magma will also be
replaced with fresh adamantine. Adamantine spires that were originally hollow
will be left hollow. See below for more details.

Usage
-----

::

    tubefill [hollow]

Specify ``hollow`` to fill in naturally hollow veins too, but be aware that this
will trigger a demon invasion on top of your miner when you dig into the region
that used to be hollow. You have been warned!
