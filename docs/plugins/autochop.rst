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
that are inaccessible are likewise never designated. Tree cutting quota
agreements that you have made are respected (by default).

Please see `gui/autochop` for the interactive configuration dialog.

Usage
-----

::

    enable autochop
    autochop [status]
    autochop (designate|undesignate)
    autochop target <max> [<min>]
    autochop quota (abide|ignore)
    autochop restrict (set|add|remove) <burrow>[,<burrow>...]
    autochop clearcut (set|add|remove) <burrow>[,<burrow>...]
    autochop (protect|unprotect) <type>[,<type>...]

Examples
--------

Ensure we always have about 500 logs in stock, harvested from anywhere on the
map. Also ensure the caravan pathway and the area around the outer wall is
always clear (requires existence of appropriately-named burrows)::

    autochop target 500
    autochop clearcut CaravanPath,OuterWall
    enable autochop

Commands
--------

``status``
    Show current configuration and statistics, including tree cutting quota
    agreement status.

``designate``
    Designate trees for chopping right now according to the current
    configuration. This works even if ``autochop`` is not currently enabled.

``undesignate``
    Undesignates all trees.

``target <max> [<min>]``
    Set the target range for the number of logs you want to have in stock. If a
    minimum amount is not specified, it defaults to 20% less than the maximum.
    The default target is ``200`` (with a corresponding minimum of ``160``).

``quota (abide|ignore)``
    Choose whether to abide by or ignore tree cutting quota agreements that you
    may have signed with friendly elven civilizations. By default, ``autochop``
    will abide by any agreements you have made.

``restrict (set|add|remove) <burrow>[,<burrow>...]``
    Instead of choosing trees across the whole game map, restrict tree cutting
    to the given burrows. Burrows can be specified by name or internal ID.

``clearcut (set|add|remove) <burrow>[,<burrow>...]``
    Ensure the given burrows are always clear of trees. As soon as a tree
    appears in any of these burrows, it is designated for chopping.

``protect <type>[,<type>...]``, ``unprotect <type>[,<type>...]``
    Choose whether to exclude trees from chopping that produce any of the given
    types of food. Valid types are: ``brewable``, ``edible``, and ``cookable``.
