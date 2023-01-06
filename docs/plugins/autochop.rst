autochop
========

.. dfhack-tool::
    :summary: Auto-harvest trees when low on stockpiled logs.
    :tags: fort auto plants

This plugin can automatically designate trees for chopping when your stocks are
low on logs. It can also ensure specified burrows are kept clear of trees, so,
for example, caravans always have a path to your depot and trees won't grow
close to your walls, giving invaders an unexpected path into your fort.

Autochop checks your stock of logs and designates appropriate trees for chopping
once every in-game day. Logs that are forbidden or inaccessible (e.g. in hidden
parts of the map, under water, etc.) are not counted towards your target. Trees
that are inaccessible are likewise never designated.

Please see `gui/autochop` for the interactive configuration dialog.

Usage
-----

::

    enable autochop
    autochop [status]
    autochop (designate|undesignate)
    autochop target <max> [<min>]
    autochop (chop|nochop) <burrow>[,<burrow>...]
    autochop (clearcut|noclearcut) <burrow>[,<burrow>...]
    autochop (protect|unprotect) <type>[,<type>...] <burrow>[,<burrow>...]

Examples
--------

Ensure we always have about 200 logs in stock, harvested from accessible trees
anywhere on the map::

    enable autochop

Ensure we always have about 500 logs in stock, harvested from a burrow named
"TreeFarm". Also ensure the caravan pathway and the area around the outer wall
("CaravanPath" and "OuterWall") are always clear::

    enable autochop
    autochop target 500
    autochop chop TreeFarm
    autochop clearcut CaravanPath,OuterWall

Clear all non-food-producing trees out of a burrow ("PicnicArea") intended to
contain only food-producing trees::

    autochop clearcut PicnicArea
    autochop protect brewable,edible,cookable PicnicArea
    autochop designate

Commands
--------

``status``
    Show current configuration and relevant statistics.

``designate``
    Designate trees for chopping right now according to the current
    configuration. This works even if ``autochop`` is not currently enabled.

``undesignate``
    Undesignates all trees.

``target <max> [<min>]``
    Set the target range for the number of logs you want to have in stock. If a
    minimum amount is not specified, it defaults to 20% less than the maximum.
    The default target is ``200`` (with a corresponding minimum of ``160``).

``(no)chop <burrow>[,<burrow>...]``
    Instead of choosing trees across the whole game map, restrict tree cutting
    to the given burrows. Burrows can be specified by name or internal ID.

``(no)clearcut <burrow>[,<burrow>...]``
    Ensure the given burrows are always clear of trees. As soon as a tree
    appears in any of these burrows, it is designated for chopping at priority
    2.

``(un)protect <type>[,<type>...] <burrow>[,<burrow>...]``
    Choose whether to exclude trees from chopping that produce any of the given
    types of food. Valid types are: ``brewable``, ``edible``, and ``cookable``.
