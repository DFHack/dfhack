cleanowned
==========

.. dfhack-tool::
    :summary: Confiscates and dumps garbage owned by dwarves.
    :tags: fort productivity items

This tool gets dwarves to give up ownership of scattered items and items with
heavy wear and then marks those items for dumping. Now you can finally get your
dwarves to give up their rotten food and tattered loincloths and go get new
ones!

Usage
-----

::

    cleanowned [<types>] [dryrun]

When run without parameters, ``cleanowned`` will confiscate and dump rotten
items and owned food that is left behind on the floor. Specify the ``dryrun``
parameter to just print out what would be done, but don't actually confiscate
anything.

You can confiscate additional types of items by adding them to the commandline:

``scattered``
    Confiscate/dump all items scattered on the floor.
``x``
    Confiscate/dump items with wear level 'x' (lightly worn) and more.
``X``
    Confiscate/dump items with wear level 'X' (heavily worn) and more.
``nodump``
    Cause dwarves to drop confiscated items, but don't mark them for dumping.

Or you can confiscate all owned items by specifying ``all``.

Example
-------

``cleanowned scattered X``
    Confiscate and dump rotten and dropped food, garbage on the floors, and any
    worn items with 'X' damage and above.
