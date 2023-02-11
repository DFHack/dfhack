autoclothing
============

.. dfhack-tool::
    :summary: Automatically manage clothing work orders.
    :tags: fort auto workorders

This command allows you to set how many of each clothing type every citizen
should have.

Usage
-----

::

    autoclothing
    autoclothing <material> <item> [quantity]

``material`` can be "cloth", "silk", "yarn", or "leather". The ``item`` can be
anything your civilization can produce, such as "dress" or "mitten".

When invoked without parameters, it shows a summary of all managed clothing
orders, and the overall clothing situation in your fort.
When invoked with a material and item, but without a quantity, it shows
the current configuration for that material and item.


Examples
--------

``autoclothing cloth "short skirt" 10``
    Sets the desired number of cloth short skirts available per citizen to 10.
``autoclothing cloth dress``
    Displays the currently set number of cloth dresses chosen per citizen.
