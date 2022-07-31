sort
====
Tags:
:dfhack-keybind:`sort-items`
:dfhack-keybind:`sort-units`

Sort the visible item or unit list.

Usage::

    sort-items <property> [<property> ...]
    sort-units <property> [<property> ...]

Both commands sort the visible list using the given sequence of comparisons.
Each property can be prefixed with a ``<`` or ``>`` character to indicate
whether elements that don't have the given property defined go first or last
(respectively) in the sorted list.

Examples
--------

``sort-items material type quality``
    Sort a list of items by material, then by type, then by quality
``sort-units profession name``
    Sort a list of units by profession, then by name

Properties
----------

Items can be sorted by the following properties:

- ``type``
- ``description``
- ``base_quality``
- ``quality``
- ``improvement``
- ``wear``
- ``material``

Units can be sorted by the following properties:

- ``name``
- ``age``
- ``arrival``
- ``noble``
- ``profession``
- ``profession_class``
- ``race``
- ``squad``
- ``squad_position``
- ``happiness``
