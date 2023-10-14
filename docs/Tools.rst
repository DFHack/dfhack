.. _tools:

DFHack tools
============

DFHack comes with **a lot** of tools. This page attempts to make it clearer
what they are, how they work, and how to find the ones you want.

.. contents:: Contents
  :local:

What tools are and how they work
--------------------------------

DFHack is a Dwarf Fortress memory access and modification framework, so DFHack
tools normally access Dwarf Fortress internals and make some specific changes.

Some tools just make a targeted change when you run them, like `unforbid`, which
scans through all your items and removes the ``forbidden`` flag from each of
them.

Some tools need to be enabled, and then they run in the background and make
changes to the game on your behalf, like `autobutcher`, which monitors your
livestock population and automatically marks excess animals for butchering.

And some tools just exist to give you information that is otherwise hard to
come by, like `gui/petitions`, which shows you the active petitions for
guildhalls and temples that you have agreed to.

Finding the tool you need
-------------------------

DFHack tools are tagged with categories to make them easier to find. These
categories are listed in the next few sections. Note that a tool can belong to
more than one category. If you already know what you're looking for, try the
`search` or Ctrl-F on this page. If you'd like to see the full list of tools in
one flat list, please refer to the `annotated index <all-tag-index>`.

Some tools are part of our back catalog and haven't been updated yet for v50 of
Dwarf Fortress. These tools are tagged as
`unavailable <unavailable-tag-index>`. They will still appear in the
alphabetical list at the bottom of this page, but unavailable tools will not
listed in any of the indices.

DFHack tools by game mode
-------------------------

.. include:: tags/bywhen.rst

DFHack tools by theme
---------------------

.. include:: tags/bywhy.rst

DFHack tools by what they affect
--------------------------------

.. include:: tags/bywhat.rst

All DFHack tools alphabetically
-------------------------------

.. toctree::
  :glob:
  :maxdepth: 1
  :titlesonly:

  tools/*
  tools/*/*
