#############
Removed tools
#############

This page lists tools (plugins or scripts) that were previously included in
DFHack but have been removed. It exists primarily so that internal links still
work (e.g. links from the `changelog`).

.. contents:: Contents
  :local:
  :depth: 1

.. _devel/unforbidall:

devel/unforbidall
=================

Replaced by the `unforbid` script. Run ``unforbid all --quiet`` to match the
behavior of the original ``devel/unforbidall`` script.

.. _deteriorateclothes:

deteriorateclothes
==================
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=clothes``.

.. _deterioratecorpses:

deterioratecorpses
==================
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=corpses``.

.. _deterioratefood:

deterioratefood
===============
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=food``.

.. _digfort:

digfort
=======
A script to designate an area for digging according to a plan in csv format.
Please use DFHack's more powerful `quickfort` script instead. You can use your
existing .csv files. Just move them to the ``blueprints`` folder in your DF
installation, and instead of ``digfort file.csv``, run
``quickfort run file.csv``.

.. _fortplan:

fortplan
========
Designates furniture for building according to a ``.csv`` file with
quickfort-style syntax. Please use DFHack's more powerful `quickfort`
script instead. You can use your existing .csv files. Just move them to the
``blueprints`` folder in your DF installation, and instead of
``fortplan file.csv`` run ``quickfort run file.csv``.

.. _warn-stuck-trees:

warn-stuck-trees
================
The corresponding DF bug, :bug:`9252` was fixed in DF 0.44.01.
