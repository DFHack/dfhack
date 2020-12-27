.. _quickfort-library-guide:

Quickfort Library Guide
=======================

This guide contains a high-level overview of the blueprints available in the
:source:`quickfort blueprint library <data/blueprints/library>`. You can list
library blueprints by running ``quickfort list --library`` or by hitting
:kbd:`Alt`:kbd:`l` in the ``quickfort gui`` interactive dialog.

Each file is hyperlinked to its online version so you can see exactly what the
blueprints do.

Whole fort blueprint sets
-------------------------

These files contain the plans for entire fortresses. Each file has one or more
help sections that walk you through how to build the fort, step by step.

- :source:`library/dreamfort.csv <data/blueprints/library/dreamfort.csv>`
- :source:`library/quickfortress.csv <data/blueprints/library/quickfortress.csv>`

Dreamfort
~~~~~~~~~

Dreamfort is a fully functional, self-sustaining fortress with defenses,
farming, a complete set of workshops, self-managing quantum stockpiles, a grand
dining hall, hospital, jail, fresh water well system, guildhalls, and bedrooms
for hundreds of dwarves. It also comes with manager work orders to automate
basic fort needs, such as food and booze production. It can function by itself
or as the core of a larger, more ambitious fortress. Read the high-level
walkthrough by running ``quickfort run library/dreamfort.csv`` and list the
walkthroughs for the individual levels by running ``quickfort list dreamfort -l
-m notes`` or by opening the ``quickfort gui`` dialog, enabling the library
with :kbd:`Alt`:kbd:`l`, and setting the filter to ``dreamfort notes``.

Dreamfort blueprints are also available for easy viewing and copying `online
<https://drive.google.com/drive/folders/1iS90EEVqUkxTeZiiukVj1pLloZqabKuP>`__.

The Quick Fortresses
~~~~~~~~~~~~~~~~~~~~

The Quick Fortress is an updated version of the example fortress that came with
`Python Quickfort 2.0 <https://github.com/joelpt/quickfort>`__ (the program
DFHack quickfort was inspired by). While it is not a complete fortress by
itself, it is much simpler than Dreamfort and is good for a first introduction
to `quickfort` blueprints. Read its walkthrough with ``quickfort run
library/quickfortress.csv``.

Layout helpers
--------------

These files simply draw diagonal marker-mode lines starting from the cursor.
They are especially useful for finding the center of the map when you are
planning your fortress. Once you are done using them for alignment, use
``quickfort undo`` at the same cursor position to make them disappear. Since
these ``#dig`` blueprints can only mark undug wall tiles for mining, they are
best used underground. They won't do much on the surface, where there aren't
many walls.

- :source:`library/layout-helpers/mark_up_left.csv <data/blueprints/library/layout-helpers/mark_up_left.csv>`
- :source:`library/layout-helpers/mark_up_right.csv <data/blueprints/library/layout-helpers/mark_up_right.csv>`
- :source:`library/layout-helpers/mark_down_right.csv <data/blueprints/library/layout-helpers/mark_down_right.csv>`
- :source:`library/layout-helpers/mark_down_left.csv <data/blueprints/library/layout-helpers/mark_down_left.csv>`

Bedrooms
--------

These are popular bedroom layouts from the :wiki:`Bedroom design` page on the
wiki. Each file has ``#dig``, ``#build``, and ``#query`` blueprints to dig the
rooms, build the furniture, and configure the beds as bedrooms, respectively.

- :source:`library/bedrooms/48-4-Raynard_Whirlpool_Housing.csv <data/blueprints/library/bedrooms/48-4-Raynard_Whirlpool_Housing.csv>`
- :source:`library/bedrooms/95-9-Hactar1_3_Branch_Tree.csv <data/blueprints/library/bedrooms/95-9-Hactar1_3_Branch_Tree.csv>`
- :source:`library/bedrooms/28-3-Modified_Windmill_Villas.csv <data/blueprints/library/bedrooms/28-3-Modified_Windmill_Villas.csv>`

Tombs
-----

These blueprints have burial plot layouts for fortress that expect a lot of
casualties.

- :source:`library/tombs/Mini_Saracen.csv <data/blueprints/library/tombs/Mini_Saracen.csv>`
- :source:`library/tombs/The_Saracen_Crypts.csv <data/blueprints/library/tombs/The_Saracen_Crypts.csv>`

Exploratory mining
------------------

Several mining patterns to choose from when searching for gems or ores. The
patterns can be repeated up or down z-levels for exploring through the depths.

- :source:`library/exploratory-mining/tunnels.csv <data/blueprints/library/exploratory-mining/tunnels.csv>`
- :source:`library/exploratory-mining/vertical-mineshafts.csv <data/blueprints/library/exploratory-mining/vertical-mineshafts.csv>`
- :source:`library/exploratory-mining/connected-mineshafts.csv <data/blueprints/library/exploratory-mining/connected-mineshafts.csv>`

Miscellaneous
-------------

Extra blueprints that are useful in specific situations.

- :source:`library/embark.csv <data/blueprints/library/embark.csv>`

The embark blueprints are useful directly after embark. It contains a ``#build``
blueprint that builds important starting workshops (mason, carpenter, mechanic,
and craftsdwarf) and a ``#place`` blueprint that lays down a pattern of useful
starting stockpiles.
