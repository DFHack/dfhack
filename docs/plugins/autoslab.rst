autoslab
========

.. dfhack-tool::
    :summary: Automatically engrave slabs for ghostly citizens.
    :tags: fort auto workorders
    :no-command:

Automatically queue orders to engrave slabs of existing ghosts. Will only queue
an order if there is no existing slab with that unit's memorial engraved and
there is not already an existing work order to engrave a slab for that unit.
Make sure you have spare slabs on hand for engraving! If you run
`orders import library/rockstock <orders>`, you'll be sure to always have
some slabs in stock.

Usage
-----

``enable autoslab``
    Enables the plugin and starts checking for ghosts that need memorializing.

``disable autoslab``
    Disables the plugin.
