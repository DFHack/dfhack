getplants
=========

.. dfhack-tool::
    :summary: Designate trees for chopping and shrubs for gathering.
    :tags: fort productivity plants

Specify the types of trees to cut down and/or shrubs to gather by their plant
names.

Usage
-----

``getplants [-t|-s|-f] [<trait filters>]``
    List valid tree/shrub ids, optionally restricted to the specified type and
    traits.
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
``getplants --brewable -a``
    Designate all shrubs that can be brewed into alcohol.
``getplants --brewable``
    List all valid brewable plant IDs.

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
``-d``, ``--dry-run``
    Dry run: report how many designations would be (un)set without changing
    anything.
``--brewable``
    Restrict the selection to plants that can be brewed into alcohol.
``--edible``
    Restrict the selection to plants with parts that can be eaten raw.
``--oil``
    Restrict the selection to plants with seeds, nuts, or fruits that can be
    pressed into oil.
``--cloth``
    Restrict the selection to plants that yield thread for cloth.
``--dye``
    Restrict the selection to plants that yield dyes.

Trait filters can be combined, in which case a plant that matches any of the
specified traits qualifies. They apply to the whole selection, so
``getplants --brewable -x QUARRY_BUSH`` designates every brewable plant except
quarry bushes.
