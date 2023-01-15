getplants
=========

.. dfhack-tool::
    :summary: Designate trees for chopping and shrubs for gathering.
    :tags: untested fort productivity plants

Specify the types of trees to cut down and/or shrubs to gather by their plant
names.

Usage
-----

``getplants [-t|-s|-f]``
    List valid tree/shrub ids, optionally restricted to the specified type.
``getplants <id> [<id> ...] [<options>]``
    Designate trees/shrubs of the specified types for chopping/gathering.

Examples
--------

``getplants``
    List all valid IDs.
``getplants -f -a``
    Gather all plants on the map that yield seeds for farming.
``getplants NETHER_CAP -n 10``
    Designate 10 nether cap trees for chopping.

Options
-------

``-t``
    Tree: Select trees only (exclude shrubs).
``-s``
    Shrub: Select shrubs only (exclude trees).
``-f``
    Farming: Designate only shrubs that yield seeds for farming.
``-a``
    All: Select every type of plant (obeys ``-t``/``-s``/``-f``).
``-c``
    Clear: Clear designations instead of setting them.
``-x``
    eXcept: Apply selected action to all plants except those specified (invert
    selection).
``-v``
    Verbose: Lists the number of (un)designations per plant.
``-n <num>``
    Number: Designate up to the specified number of plants of each species.

.. note::

    DF is capable of determining that a shrub has already been picked, leaving
    an unusable structure part behind. This plugin does not perform such a check
    (as the location of the required information has not yet been identified).
    This leads to some shrubs being designated when they shouldn't be, causing a
    plant gatherer to walk there and do nothing (except clearing the
    designation). See :issue:`1479` for details.
