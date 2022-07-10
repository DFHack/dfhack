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
