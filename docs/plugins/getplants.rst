getplants
=========
This tool allows plant gathering and tree cutting by RAW ID. Specify the types
of trees to cut down and/or shrubs to gather by their plant names, separated
by spaces.

Options:

:``-t``: Tree: Select trees only (exclude shrubs)
:``-s``: Shrub: Select shrubs only (exclude trees)
:``-f``: Farming: Designate only shrubs that yield seeds for farming. Implies -s
:``-c``: Clear: Clear designations instead of setting them
:``-x``: eXcept: Apply selected action to all plants except those specified (invert
     selection)
:``-a``: All: Select every type of plant (obeys ``-t``/``-s``/``-f``)
:``-v``: Verbose: Lists the number of (un)designations per plant
:``-n *``: Number: Designate up to * (an integer number) plants of each species

Specifying both ``-t`` and ``-s`` or ``-f`` will have no effect. If no plant IDs are
specified, all valid plant IDs will be listed, with ``-t``, ``-s``, and ``-f``
restricting the list to trees, shrubs, and farmable shrubs, respectively.

.. note::

    DF is capable of determining that a shrub has already been picked, leaving
    an unusable structure part behind. This plugin does not perform such a check
    (as the location of the required information has not yet been identified).
    This leads to some shrubs being designated when they shouldn't be, causing a
    plant gatherer to walk there and do nothing (except clearing the
    designation). See :issue:`1479` for details.

    The implementation another known deficiency: it's incapable of detecting that
    raw definitions that specify a seed extraction reaction for the structural part
    but has no other use for it cannot actually yield any seeds, as the part is
    never used (parts of :bug:`6940`, e.g. Red Spinach), even though DF
    collects it, unless there's a workshop reaction to do it (which there isn't
    in vanilla).
