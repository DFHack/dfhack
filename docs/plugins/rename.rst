rename
======

.. dfhack-tool::
    :summary: Easily rename things.
    :tags: unavailable

Use `gui/rename` for an in-game interface.

Usage
-----

``rename squad <ordinal> "<name>"``
    Rename the indicated squad. The ordinal is the number that corresponds to
    the list of squads in the squads menu (:kbd:`s`). The first squad is ordinal
    ``1``.
``rename hotkey <ordinal> "<name>"``
    Rename the indicated hotkey. The ordinal the the number that corresponds to
    the list of hotkeys in the hotkeys menu (:kbd:`H`). The first hotkey is
    ordinal ``1``.
``rename unit "<name>"``
    Give the selected unit the given nickname.
``rename unit-profession "<name>"``
    Give the selected unit the given profession name.
``rename building "<name>"``
    Set a custom name to the selected building. The building must be a
    stockpile, workshop, furnace, trap, siege engine, or activity zone.

Example
-------

``rename squad 1 "The Whiz Bonkers"``
    Rename the first squad to The Whiz Bonkers.
