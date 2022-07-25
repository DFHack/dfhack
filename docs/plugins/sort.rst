sort-items
==========
Sort the visible item list::

    sort-items order [order...]

Sort the item list using the given sequence of comparisons.
The ``<`` prefix for an order makes undefined values sort first.
The ``>`` prefix reverses the sort order for defined values.

Item order examples::

    description material wear type quality

The orderings are defined in ``hack/lua/plugins/sort/*.lua``

sort-units
==========
Sort the visible unit list::

    sort-units order [order...]

Sort the unit list using the given sequence of comparisons.
The ``<`` prefix for an order makes undefined values sort first.
The ``>`` prefix reverses the sort order for defined values.

Unit order examples::

    name age arrival squad squad_position profession

The orderings are defined in ``hack/lua/plugins/sort/*.lua``

:dfhack-keybind:`sort-units`
