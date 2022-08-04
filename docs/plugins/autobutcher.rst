autobutcher
===========
Tags:
:dfhack-keybind:`autobutcher`

Automatically butcher excess livestock. This plugin monitors how many pets you
have of each gender and age and assigns excess lifestock for slaughter. Requires
that you add the target race(s) to a watch list. Units will be ignored if they
are:

* Untamed
* Nicknamed (for custom protection; you can use the `rename` ``unit`` tool
  individually, or `zone` ``nick`` for groups)
* Caged, if and only if the cage is defined as a room (to protect zoos)
* Trained for war or hunting

Creatures who will not reproduce (because they're not interested in the
opposite sex or have been gelded) will be butchered before those who will.
Older adults and younger children will be butchered first if the population
is above the target (defaults are: 1 male kid, 5 female kids, 1 male adult,
5 female adults). Note that you may need to set a target above 1 to have a
reliable breeding population due to asexuality etc. See `fix-ster` if this is a
problem.

Usage:

``enable autobutcher``
    Start processing livestock according to the configuration. Note that
    no races are watched by default. You have to add the ones you want to
    monitor with ``autobutcher watch``, ``autobutcher target`` or
    ``autobutcher autowatch``.
``autobutcher autowatch``
    Automatically add all new races (animals you buy from merchants, tame
    yourself, or get from migrants) to the watch list using the default target
    counts.
``autobutcher noautowatch``
    Stop auto-adding new races to the watch list.
``autobutcher target <fk> <mk> <fa> <ma> all|new|<race> [<race> ...]``
    Set target counts for the specified races:
        fk = number of female kids
        mk = number of male kids
        fa = number of female adults
        ma = number of female adults
    If you specify ``all``, then this command will set the counts for all races
    on your current watchlist (including the races which are currenly set to
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
    Useful to run form dfhack-run like:
    ``dfhack-run autobutcher list_export > autobutcher.script``

To see a list of all races, run this command:

    devel/query --table df.global.world.raws.creatures.all --search ^creature_id --maxdepth 1

Though not all the races listed there are tameable/butcherable.

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

    autobutcher target 2 2 2 2 DOG
    autobutcher target 1 1 2 2 CAT
    autobutcher target 50 50 14 2 BIRD_GOOSE
    autobutcher target 2 2 4 2 ALPACA SHEEP LLAMA
    autobutcher target 5 5 6 2 PIG
    autobutcher target 0 0 0 0 new
    autobutcher autowatch

Options:

:example:      Print some usage examples.
:start:        Start running every X frames (df simulation ticks).
               Default: X=6000, which would be every 60 seconds at 100fps.
:stop:         Stop running automatically.
:sleep <x>:    Changes the timer to sleep X frames between runs.
:watch R:      Start watching a race. R can be a valid race RAW id (ALPACA,
               BIRD_TURKEY, etc) or a list of ids seperated by spaces or
               the keyword 'all' which affects all races on your current
               watchlist.
:unwatch R:    Stop watching race(s). The current target settings will be
               remembered. R can be a list of ids or the keyword 'all'.
:forget R:     Stop watching race(s) and forget it's/their target settings.
               R can be a list of ids or the keyword 'all'.
:autowatch:    Automatically adds all new races (animals you buy from merchants,
               tame yourself or get from migrants) to the watch list using
               default target count.
:noautowatch:  Stop auto-adding new races to the watchlist.
:list:         Print the current status and watchlist.
:list_export:  Print the commands needed to set up status and watchlist,
               which can be used to import them to another save (see notes).
:target <fk> <mk> <fa> <ma> <R>:
               Set target count for specified race(s).  The first four arguments
               are the number of female and male kids, and female and male adults.
               R can be a list of spceies ids, or the keyword ``all`` or ``new``.
               ``R = 'all'``: change target count for all races on watchlist
               and set the new default for the future. ``R = 'new'``: don't touch
               current settings on the watchlist, only set the new default
               for future entries.
:list_export:  Print the commands required to rebuild your current settings.

.. note::

    Settings and watchlist are stored in the savegame, so that you can have
    different settings for each save. If you want to copy your watchlist to
    another savegame you must export the commands required to recreate your settings.

    To export, open an external terminal in the DF directory, and run
    ``dfhack-run autobutcher list_export > filename.txt``.  To import, load your
    new save and run ``script filename.txt`` in the DFHack terminal.


Examples:

You want to keep max 7 kids (4 female, 3 male) and max 3 adults (2 female,
1 male) of the race alpaca. Once the kids grow up the oldest adults will get
slaughtered. Excess kids will get slaughtered starting with the youngest
to allow that the older ones grow into adults. Any unnamed cats will
be slaughtered as soon as possible. ::

     autobutcher target 4 3 2 1 ALPACA BIRD_TURKEY
     autobutcher target 0 0 0 0 CAT
     autobutcher watch ALPACA BIRD_TURKEY CAT
     autobutcher start

Automatically put all new races onto the watchlist and mark unnamed tame units
for slaughter as soon as they arrive in your fort. Settings already made
for specific races will be left untouched. ::

     autobutcher target 0 0 0 0 new
     autobutcher autowatch
     autobutcher start

Stop watching the races alpaca and cat, but remember the target count
settings so that you can use 'unwatch' without the need to enter the
values again. Note: 'autobutcher unwatch all' works, but only makes sense
if you want to keep the plugin running with the 'autowatch' feature or manually
add some new races with 'watch'. If you simply want to stop it completely use
'autobutcher stop' instead. ::

    autobutcher unwatch ALPACA CAT

const string autobutcher_help =
