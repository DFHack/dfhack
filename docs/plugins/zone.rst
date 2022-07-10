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
