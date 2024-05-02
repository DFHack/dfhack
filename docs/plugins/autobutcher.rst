autobutcher
===========

.. dfhack-tool::
    :summary: Automatically butcher excess livestock.
    :tags: fort auto fps animals

This plugin monitors how many pets you have of each gender and age and assigns
excess livestock for slaughter. It requires that you add the target race(s) to a
watch list. Units will be ignored if they are:

* Untamed
* Nicknamed (for custom protection; you can use the `rename` ``unit`` tool
  individually, or `zone` ``nick`` for groups)
* Caged, if and only if the cage is in a zone (to protect zoos)
* Trained for war or hunting

Creatures who will not reproduce (because they're not interested in the
opposite sex or have been gelded) will be butchered before those who will.
Older adults and younger children will be butchered first if the population
is above the target (defaults are: 2 male kids, 4 female kids, 2 male adults,
4 female adults). Note that you may need to set a target above 1 to have a
reliable breeding population due to asexuality etc. See `fix-ster` if this is a
problem.

Usage
-----

``enable autobutcher``
    Start processing livestock according to the configuration. Note that
    no races are watched by default. You have to add the ones you want to
    monitor with ``autobutcher watch``, ``autobutcher target`` or
    ``autobutcher autowatch``.
``autobutcher autowatch``
    Automatically add all new races (animals you buy from merchants, tame
    yourself, or get from migrants) to the watch list using the default target
    counts. This option is enabled by default.
``autobutcher noautowatch``
    Stop auto-adding new races to the watch list.
``autobutcher target <fk> <mk> <fa> <ma> all|new|<race> [<race> ...]``
    Set target counts for the specified races:
    - fk = number of female kids
    - mk = number of male kids
    - fa = number of female adults
    - ma = number of female adults
    If you specify ``all``, then this command will set the counts for all races
    on your current watchlist (including the races which are currently set to
    'unwatched') and sets the new default for future watch commands. If you
    specify ``new``, then this command just sets the new default counts for
    future watch commands without changing your current watchlist. Otherwise,
    all space separated races listed will be modified (or added to the watchlist
    if they aren't there already).
``autobutcher ticks <ticks>``
    Change the number of ticks between scanning cycles when the plugin is
    enabled. By default, a cycle happens every 6000 ticks (about 8 game days).
``autobutcher watch all|<race> [<race> ...]``
    Start watching the listed races. If they aren't already in your watchlist,
    then they will be added with the default target counts. If you specify the
    keyword ``all``, then all races in your watchlist that are currently marked
    as unwatched will become watched.
``autobutcher unwatch all|<race> [<race> ...]``
    Stop watching the specified race(s) (or all races on your watchlist if
    ``all`` is given). The current target settings will be remembered.
``autobutcher forget all|<race> [<race> ...]``
    Unwatch the specified race(s) (or all races on your watchlist if ``all`` is
    given) and forget target settings for it/them.
``autobutcher [list]``
    Print status and current settings, including the watchlist. This is the
    default command if autobutcher is run without parameters.
``autobutcher list_export``
    Print commands required to set the current settings in another fort.

To see a list of all races, run this command:

    devel/query --table df.global.world.raws.creatures.all --search ^creature_id --maxdepth 1

Though not all the races listed there are tameable/butcherable.

.. note::

    Settings and watchlist are stored in the savegame, so you can have different
    settings for each save. If you want to copy your watchlist to another,
    savegame, you can export the commands required to recreate your settings.

    To export, open an external terminal in the DF directory, and run
    ``dfhack-run autobutcher list_export > filename.txt``.  To import, load your
    new save and run ``script filename.txt`` in the DFHack terminal.

Examples
--------

Keep at most 7 kids (4 female, 3 male) and at most 3 adults (2 female, 1 male)
for turkeys. Once the kids grow up, the oldest adults will get slaughtered.
Excess kids will get slaughtered starting the the youngest to allow that the
older ones grow into adults::

    autobutcher target 4 3 2 1 BIRD_TURKEY

Configure useful limits for dogs, cats, geese (for eggs, leather, and bones),
alpacas, sheep, and llamas (for wool), and pigs (for milk and meat). All other
unnamed tame units will be marked for slaughter as soon as they arrive in your
fortress::

    enable autobutcher
    autobutcher target 2 2 2 2 DOG
    autobutcher target 1 1 2 2 CAT
    autobutcher target 50 50 14 2 BIRD_GOOSE
    autobutcher target 2 2 4 2 ALPACA SHEEP LLAMA
    autobutcher target 5 5 6 2 PIG
    autobutcher target 0 0 0 0 new
