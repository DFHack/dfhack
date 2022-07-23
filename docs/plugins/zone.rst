.. _autobutcher:
.. _autonestbox:

zone
====
Helps a bit with managing activity zones (pens, pastures and pits) and cages.

:dfhack-keybind:`zone`

Options:

:set:         Set zone or cage under cursor as default for future assigns.
:assign:      Assign unit(s) to the pen or pit marked with the 'set' command.
              If no filters are set a unit must be selected in the in-game ui.
              Can also be followed by a valid zone id which will be set
              instead.
:unassign:    Unassign selected creature from it's zone.
:nick:        Mass-assign nicknames, must be followed by the name you want
              to set.
:remnick:     Mass-remove nicknames.
:enumnick:    Assign enumerated nicknames (e.g. "Hen 1", "Hen 2"...). Must be
              followed by the prefix to use in nicknames.
:tocages:     Assign unit(s) to cages inside a pasture.
:uinfo:       Print info about unit(s). If no filters are set a unit must
              be selected in the in-game ui.
:zinfo:       Print info about zone(s). If no filters are set zones under
              the cursor are listed.
:verbose:     Print some more info.
:filters:     Print list of valid filter options.
:examples:    Print some usage examples.
:not:         Negates the next filter keyword.

Filters:

:all:           Process all units (to be used with additional filters).
:count:         Must be followed by a number. Process only n units (to be used
                with additional filters).
:unassigned:    Not assigned to zone, chain or built cage.
:minage:        Minimum age. Must be followed by number.
:maxage:        Maximum age. Must be followed by number.
:race:          Must be followed by a race RAW ID (e.g. BIRD_TURKEY, ALPACA,
                etc). Negatable.
:caged:         In a built cage. Negatable.
:own:           From own civilization. Negatable.
:merchant:      Is a merchant / belongs to a merchant. Should only be used for
                pitting, not for stealing animals (slaughter should work).
:war:           Trained war creature. Negatable.
:hunting:       Trained hunting creature. Negatable.
:tamed:         Creature is tame. Negatable.
:trained:       Creature is trained. Finds war/hunting creatures as well as
                creatures who have a training level greater than 'domesticated'.
                If you want to specifically search for war/hunting creatures use
                'war' or 'hunting' Negatable.
:trainablewar:  Creature can be trained for war (and is not already trained for
                war/hunt). Negatable.
:trainablehunt: Creature can be trained for hunting (and is not already trained
                for war/hunt). Negatable.
:male:          Creature is male. Negatable.
:female:        Creature is female. Negatable.
:egglayer:      Race lays eggs. Negatable.
:grazer:        Race is a grazer. Negatable.
:milkable:      Race is milkable. Negatable.

Usage with single units
-----------------------
One convenient way to use the zone tool is to bind the command 'zone assign' to
a hotkey, maybe also the command 'zone set'. Place the in-game cursor over
a pen/pasture or pit, use 'zone set' to mark it. Then you can select units
on the map (in 'v' or 'k' mode), in the unit list or from inside cages
and use 'zone assign' to assign them to their new home. Allows pitting your
own dwarves, by the way.

Usage with filters
------------------
All filters can be used together with the 'assign' command.

Restrictions: It's not possible to assign units who are inside built cages
or chained because in most cases that won't be desirable anyways.
It's not possible to cage owned pets because in that case the owner
uncages them after a while which results in infinite hauling back and forth.

Usually you should always use the filter 'own' (which implies tame) unless you
want to use the zone tool for pitting hostiles. 'own' ignores own dwarves unless
you specify 'race DWARF' (so it's safe to use 'assign all own' to one big
pasture if you want to have all your animals at the same place). 'egglayer' and
'milkable' should be used together with 'female' unless you have a mod with
egg-laying male elves who give milk or whatever. Merchants and their animals are
ignored unless you specify 'merchant' (pitting them should be no problem,
but stealing and pasturing their animals is not a good idea since currently they
are not properly added to your own stocks; slaughtering them should work).

Most filters can be negated (e.g. 'not grazer' -> race is not a grazer).

Mass-renaming
-------------
Using the 'nick' command you can set the same nickname for multiple units.
If used without 'assign', 'all' or 'count' it will rename all units in the
current default target zone. Combined with 'assign', 'all' or 'count' (and
further optional filters) it will rename units matching the filter conditions.

Cage zones
----------
Using the 'tocages' command you can assign units to a set of cages, for example
a room next to your butcher shop(s). They will be spread evenly among available
cages to optimize hauling to and butchering from them. For this to work you need
to build cages and then place one pen/pasture activity zone above them, covering
all cages you want to use. Then use 'zone set' (like with 'assign') and use
'zone tocages filter1 filter2 ...'. 'tocages' overwrites 'assign' because it
would make no sense, but can be used together with 'nick' or 'remnick' and all
the usual filters.

Examples
--------
``zone assign all own ALPACA minage 3 maxage 10``
   Assign all own alpacas who are between 3 and 10 years old to the selected
   pasture.
``zone assign all own caged grazer nick ineedgrass``
   Assign all own grazers who are sitting in cages on stockpiles (e.g. after
   buying them from merchants) to the selected pasture and give them
   the nickname 'ineedgrass'.
``zone assign all own not grazer not race CAT``
   Assign all own animals who are not grazers, excluding cats.
``zone assign count 5 own female milkable``
   Assign up to 5 own female milkable creatures to the selected pasture.
``zone assign all own race DWARF maxage 2``
   Throw all useless kids into a pit :)
``zone nick donttouchme``
   Nicknames all units in the current default zone or cage to 'donttouchme'.
   Mostly intended to be used for special pastures or cages which are not marked
   as rooms you want to protect from autobutcher.
``zone tocages count 50 own tame male not grazer``
   Stuff up to 50 owned tame male animals who are not grazers into cages built
   on the current default zone.
autobutcher
===========
Assigns lifestock for slaughter once it reaches a specific count. Requires that
you add the target race(s) to a watch list. Only tame units will be processed.

Units will be ignored if they are:

* Nicknamed (for custom protection; you can use the `rename` ``unit`` tool
  individually, or `zone` ``nick`` for groups)
* Caged, if and only if the cage is defined as a room (to protect zoos)
* Trained for war or hunting

Creatures who will not reproduce (because they're not interested in the
opposite sex or have been gelded) will be butchered before those who will.
Older adults and younger children will be butchered first if the population
is above the target (default 1 male, 5 female kids and adults).  Note that
you may need to set a target above 1 to have a reliable breeding population
due to asexuality etc.  See `fix-ster` if this is a problem.

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
autonestbox
===========
Assigns unpastured female egg-layers to nestbox zones. Requires that you create
pen/pasture zones above nestboxes. If the pen is bigger than 1x1 the nestbox
must be in the top left corner. Only 1 unit will be assigned per pen, regardless
of the size. The age of the units is currently not checked, most birds grow up
quite fast. Egglayers who are also grazers will be ignored, since confining them
to a 1x1 pasture is not a good idea. Only tame and domesticated own units are
processed since pasturing half-trained wild egglayers could destroy your neat
nestbox zones when they revert to wild. When called without options autonestbox
will instantly run once.

Options:

:start:        Start running every X frames (df simulation ticks).
               Default: X=6000, which would be every 60 seconds at 100fps.
:stop:         Stop running automatically.
:sleep:        Must be followed by number X. Changes the timer to sleep X
               frames between runs.
