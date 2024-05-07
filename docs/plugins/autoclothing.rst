autoclothing
============

.. dfhack-tool::
    :summary: Automatically manage clothing work orders.
    :tags: fort auto workorders

This command allows you to configure a "uniform" that your civilians should
wear. ``autoclothing`` will then keep track of your stock of the configured
clothing (including clothing that your citizens are already wearing) and will
generate manager orders to manufacture more when your stock drops below a
configured per-citizen threshold.

If you are wondering whether you should enable `tailor` instead, see the
comparsion in the last section below.

Usage
-----

::

    autoclothing
    autoclothing <material> <item>
    autoclothing <material> <item> <quantity>

``<material>`` can be "cloth", "silk", "yarn", or "leather". The ``<item>`` can
be anything your civilization can produce, such as "dress" or "mitten".

The quantity is **per-citizen**, so keep the values low. A quantity of ``1``
makes one per citizen. ``2`` will ensure everyone has a pre-made spare. Usually,
``1`` is enough.

When invoked without parameters, it shows a summary of all managed clothing
orders, and the overall clothing situation in your fort. When invoked with a
material and item, but without a quantity, it shows the current configuration
for that material and item.

Examples
--------

``autoclothing cloth "short skirt" 1``
    Ensures that every citizen will have a cloth short skirts available (as
    long as there is cloth available to make them out of).
``autoclothing cloth dress``
    Displays the currently set number of cloth dresses chosen per citizen.

Which should I enable: autoclothing or tailor?
----------------------------------------------

Both ``autoclothing`` and `tailor` generate manager orders for needed clothing,
but they make different choices about when and what to order.

Enable ``autoclothing`` when:

- you want to set specific configuration for each clothing type for more
  control over exactly what your citizens wear
- you want to keep a cache of spare clothing before it is needed

Enable `tailor` when:

- you want your citizens clothed and don't care specifically what they wear
- you want a tool that can run effectively with a default configuration

You can even enable both tools if you only want to set configuration for a few
specific clothing types (e.g. if you'd prefer that most pople wear dresses).
You can set the configuration for those types in ``autoclothing`` and let
`tailor` automatically manage the rest.
