autoclothing
============

Tags:
:dfhack-keybind:`autoclothing`

Automatically manage clothing work orders. It allows you to set how many of each
clothing type every citizen should have.

Usage::

    enable autoclothing
    autoclothing <material> <item> [number]

``material`` can be "cloth", "silk", "yarn", or "leather". The ``item`` can be
anything your civilization can produce, such as "dress" or "mitten".

When invoked without a number, it shows the current configuration for that
material and item.

Examples
--------

* ``autoclothing cloth "short skirt" 10``:
    Sets the desired number of cloth short skirts available per citizen to 10.
* ``autoclothing cloth dress``:
    Displays the currently set number of cloth dresses chosen per citizen.
