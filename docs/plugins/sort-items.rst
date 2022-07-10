.. _sort:

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
