.. _blueprint-library-guide:
.. _quickfort-library-guide:

Blueprint Library Index
=======================

This guide contains a high-level overview of the blueprints available in the
:source:`quickfort blueprint library <data/blueprints/library>`. You can list
library blueprints by running ``quickfort list --library`` or by hitting
:kbd:`Alt`:kbd:`l` in the ``gui/quickfort`` file load dialog.

Each file is hyperlinked to its online version so you can see exactly what the
blueprints do before you run them. Also, if you use `gui/quickfort`, you will
get a live preview of which tiles will be modified by the blueprint before you
apply it to your map.

Whole fort blueprint sets
-------------------------

These files contain the plans for entire fortresses. Each file has one or more
help sections that walk you through how to build the fort, step by step.

- :source:`library/dreamfort.csv <data/blueprints/library/dreamfort.csv>`
- :source:`library/quickfortress.csv <data/blueprints/library/quickfortress.csv>`

.. _dreamfort:

Dreamfort
~~~~~~~~~

Dreamfort is a fully functional, self-sustaining fortress with defenses,
farming, a complete set of workshops, self-managing quantum stockpiles, a grand
dining hall, hospital, jail, fresh water well system, guildhalls, noble suites,
and bedrooms for hundreds of dwarves. It also comes with manager work orders to
automate basic fort needs, such as food, booze, and item production. It can
function by itself or as the core of a larger, more ambitious fortress. Read the
high-level walkthrough by running ``quickfort run library/dreamfort.csv`` and
list the walkthroughs for the individual levels by running ``quickfort list -l
dreamfort -m notes`` or ``gui/quickfort dreamfort notes``.

Dreamfort blueprints are available for easy viewing and copying `online
<https://drive.google.com/drive/folders/1iS90EEVqUkxTeZiiukVj1pLloZqabKuP>`__.

The online spreadsheets also include `embark profile suggestions
<https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=149144025>`__,
a complete `example embark profile
<https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=1727884387>`__,
and a convenient `checklist
<https://docs.google.com/spreadsheets/d/13PVZ2h3Mm3x_G1OXQvwKd7oIR2lK4A1Ahf6Om1kFigw/edit#gid=1459509569>`__
from which you can copy the ``quickfort`` commands.

If you like, you can download a fully built Dreamfort-based fort from
:dffd:`dffd <15434>`, load it, and explore it interactively.

Visual overview
```````````````

Here are annotated screenshots of the major Dreamfort levels (or click `here
<https://drive.google.com/drive/folders/14KdE2E2wQKj4F_E-NAe3G3E4x1wiWtrc>`__
for a slideshow).

Surface level
\\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=1YL_vQJLB2YnUEFrAg9y3HEdFq3Wpw9WP
  :alt: Annotated screenshot of the dreamfort surface level
  :target: https://drive.google.com/file/d/1YL_vQJLB2YnUEFrAg9y3HEdFq3Wpw9WP
  :align: center

Farming level
\\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=1fBC3G5Y888l4tVe5REAyAd_zeojADVme
  :alt: Annotated screenshot of the dreamfort farming level
  :target: https://drive.google.com/file/d/1fBC3G5Y888l4tVe5REAyAd_zeojADVme
  :align: center

Industry level
\\\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=1emMaHHCaUPcdRbkLQqvr-0ZCs2tdM5X7
  :alt: Annotated screenshot of the dreamfort industry level
  :target: https://drive.google.com/file/d/1emMaHHCaUPcdRbkLQqvr-0ZCs2tdM5X7
  :align: center

Services level
\\\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=13vDIkTVOZGkM84tYf4O5nmRs4VZdE1gh
  :alt: Annotated screenshot of the dreamfort services level
  :target: https://drive.google.com/file/d/13vDIkTVOZGkM84tYf4O5nmRs4VZdE1gh
  :align: center
.. image:: https://drive.google.com/uc?export=download&id=1jlGr6tAhS8i-XFTz8gowTZBhXcfjfL_L
  :alt: Annotated screenshot of the dreamfort cistern
  :target: https://drive.google.com/file/d/1jlGr6tAhS8i-XFTz8gowTZBhXcfjfL_L
  :align: center

.. _example-plumbing-to-fill-cisterns:

Example plumbing to fill cisterns
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.. image:: https://drive.google.com/uc?export=download&id=1GvhX_pVDOlmqTi2OujoBqCG_qX36ExAv
  :alt: Annotated screenshot of an example aqueduct addition to the dreamfort cistern
  :target: https://drive.google.com/file/d/1GvhX_pVDOlmqTi2OujoBqCG_qX36ExAv
  :align: center

Guildhall level
\\\\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=17jHiCKeZm6FSS-CI4V0r0GJZh09nzcO_
  :alt: Annotated screenshot of the dreamfort guildhall level
  :target: https://drive.google.com/file/d/17jHiCKeZm6FSS-CI4V0r0GJZh09nzcO_
  :align: center

Noble suites
\\\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=1IBqCf6fF3lw7sHiBE_15Euubysl5AAiS
  :alt: Annotated screenshot of the dreamfort noble suites
  :target: https://drive.google.com/file/d/1IBqCf6fF3lw7sHiBE_15Euubysl5AAiS
  :align: center

Apartments
\\\\\\\\\\

.. image:: https://drive.google.com/uc?export=download&id=1mDQQXG8BnXqasRGFC9R5N6xNALiswEyr
  :alt: Annotated screenshot of the dreamfort apartments
  :target: https://drive.google.com/file/d/1mDQQXG8BnXqasRGFC9R5N6xNALiswEyr
  :align: center

The Quick Fortress
~~~~~~~~~~~~~~~~~~

The Quick Fortress is an updated version of the example fortress that came with
`Python Quickfort 2.0 <https://github.com/joelpt/quickfort>`__ (the utility that
inspired DFHack quickfort). While it is not a complete fortress by
itself, it is much simpler than Dreamfort and is good for a first introduction
to `quickfort` blueprints. Read its walkthrough with ``quickfort run
library/quickfortress.csv`` or view the blueprints `online
<https://docs.google.com/spreadsheets/d/1WuLYZBM6S2nt-XsPS30kpDnngpOQCuIdlw4zjrcITdY>`__.

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
patterns can be repeated up or down z-levels (via quickfort's ``--repeat``
commandline option) for exploring through the depths.

- :source:`library/exploratory-mining/tunnels.csv <data/blueprints/library/exploratory-mining/tunnels.csv>`
- :source:`library/exploratory-mining/vertical-mineshafts.csv <data/blueprints/library/exploratory-mining/vertical-mineshafts.csv>`
- :source:`library/exploratory-mining/connected-mineshafts.csv <data/blueprints/library/exploratory-mining/connected-mineshafts.csv>`

Miscellaneous
-------------

Extra blueprints that are useful in specific situations.

- :source:`library/aquifer_tap.csv <data/blueprints/library/aquifer_tap.csv>`
- :source:`library/embark.csv <data/blueprints/library/embark.csv>`
- :source:`library/pump_stack.csv <data/blueprints/library/pump_stack.csv>`

Light Aquifer Tap
~~~~~~~~~~~~~~~~~

The aquifer tap helps you create a safe, everlasting source of fresh water from
a light aquifer. See the step-by-step guide, including informaton on how to
create a drainage system so your dwarves don't drown when digging the tap, by
running ``quickfort run library/aquifer_tap.csv -n /help``.

You can see how to nullify the water pressure (so you don't flood your fort) in
the `Dreamfort screenshot above <example-plumbing-to-fill-cisterns>`.

The blueprint spreadsheet is also available
`online <https://docs.google.com/spreadsheets/d/1kwuCipF9FYAHNP9C_XlMpqVseaPu4SmL9YLUSQkbW4s/edit#gid=611877584>`__.

Post-embark
~~~~~~~~~~~

The embark blueprints are useful directly after embark. It contains a ``#build``
blueprint that builds important starting workshops (mason, carpenter, mechanic,
and craftsdwarf) and a ``#place`` blueprint that lays down a pattern of useful
starting stockpiles.

Pump Stack
~~~~~~~~~~

The pump stack blueprints help you move water and magma up to more convenient
locations in your fort. See the step-by-step guide for using it by running
``quickfort run library/pump_stack.csv -n /help``.

The blueprint spreadsheet is also available
`online <https://docs.google.com/spreadsheets/d/1TP2n-W-O9f30Dtl6yoTcn6yczWQRu11iM7U6TEE9634/edit#gid=0>`__.
