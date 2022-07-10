prospect
========

**Usage:**

    ``prospect [all|hell] [<options>]``

Shows a summary of resources that exist on the map. By default, only the visible
part of the map is scanned. Include the ``all`` keyword if you want ``prospect``
to scan the whole map as if it were revealed. Use ``hell`` instead of ``all`` if
you also want to see the Z range of HFS tubes in the 'features' report section.

**Options:**

:``-h``, ``--help``:
    Shows this help text.
:``-s``, ``--show <sections>``:
    Shows only the named comma-separated list of report sections. Report section
    names are: summary, liquids, layers, features, ores, gems, veins, shrubs,
    and trees. If run during pre-embark, only the layers, ores, gems, and veins
    report sections are available.
:``-v``, ``--values``:
    Includes material value in the output. Most useful for the 'gems' report
    section.

**Examples:**

``prospect all``
    Shows the entire report for the entire map.

``prospect hell --show layers,ores,veins``
    Shows only the layers, ores, and other vein stone report sections, and
    includes information on HFS tubes when a fort is loaded.

``prospect all -sores``
    Show only information about ores for the pre-embark or fortress map report.

**Pre-embark estimate:**

If prospect is called during the embark selection screen, it displays an
estimate of layer stone availability. If the ``all`` keyword is specified, it
also estimates ores, gems, and vein material. The estimate covers all tiles of
the embark rectangle.

.. note::

    The results of pre-embark prospect are an *estimate*, and can at best be
    expected to be somewhere within +/- 30% of the true amount; sometimes it
    does a lot worse. Especially, it is not clear how to precisely compute how
    many soil layers there will be in a given embark tile, so it can report a
    whole extra layer, or omit one that is actually present.
