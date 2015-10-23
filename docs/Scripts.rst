##############
DFHack Scripts
##############

Lua or ruby scripts placed in the ``hack/scripts/`` directory are considered for
execution as if they were native DFHack commands. They are listed at the end
of the ``ls`` command output.

Note: scripts in subdirectories of hack/scripts/ can still be called, but will
only be listed by ls if called as ``ls -a``. This is intended as a way to hide
scripts that are obscure, developer-oriented, or should be used as keybindings
or from the init file.

``kill-lua`` stops any currently-running Lua scripts. By default, scripts can
only be interrupted every 256 instructions. Use ``kill-lua force`` to interrupt
the next instruction.


The following pages document all the standard DFHack scripts.

* Basic scripts are not stored in any subdirectory, and can be invoked directly.
  They are generally useful tools for any player.
* ``devel/*`` scripts are intended for developers, or still substantially unfinished.
  If you don't already know what they do, best to leave them alone.
* ``fix/*`` scripts fix various bugs and issues, some of them obscure.
* ``gui/*`` scripts implement dialogs in the main game window.

  In order to avoid user confusion, as a matter of policy all these tools
  display the word "DFHack" on the screen somewhere while active.
  When that is not appropriate because they merely add keybinding hints to
  existing DF screens, they deliberately use red instead of green for the key.

* ``modtools/*`` scripts provide tools for modders, often with changes
  to the raw files, and are not intended to be called manually by end-users.

  These scripts are mostly useful for raw modders and scripters. They all have
  standard arguments: arguments are of the form ``tool -argName1 argVal1
  -argName2 argVal2``. This is equivalent to ``tool -argName2 argVal2 -argName1
  argVal1``. It is not necessary to provide a value to an argument name: ``tool
  -argName3`` is fine. Supplying the same argument name multiple times will
  result in an error. Argument names are preceded with a dash. The ``-help``
  argument will print a descriptive usage string describing the nature of the
  arguments. For multiple word argument values, brackets must be used: ``tool
  -argName4 [ sadf1 sadf2 sadf3 ]``. In order to allow passing literal braces as
  part of the argument, backslashes are used: ``tool -argName4 [ \] asdf \foo ]``
  sets ``argName4`` to ``\] asdf foo``. The ``*-trigger`` scripts have a similar
  policy with backslashes.


.. toctree::
   :glob:
   :maxdepth: 2

   /docs/_auto/*


.. warning::

    The following documentation is pulled in from external ``README`` files.
    
    - it may not match the DFHack docs style
    - some scripts have incorrect names, or are documented but unavailable
    - some scripts have entries here and are also included above

.. toctree::
   :glob:
   :maxdepth: 2

   /scripts/3rdparty/*/*


ban-cooking
===========
A more convenient way to ban cooking various categories of foods than the
kitchen interface.  Usage:  ``ban-cooking <type>``.  Valid types are ``booze``,
``honey``, ``tallow``, ``oil``, and ``seeds`` (non-tree plants with seeds).

.. _binpatch:

binpatch
========
Implements functions for in-memory binpatches.  See `binpatches`.

.. _brainwash:

brainwash
=========
Modify the personality traits of the selected dwarf to match an 'ideal'
personality - as stable and reliable as possible. This makes dwarves very
stable, preventing tantrums even after months of misery.

burial
======
Sets all unowned coffins to allow burial.  ``burial -pets`` also allows burial
of pets.

.. _command-prompt:

command-prompt
==============
A one line command prompt in df. Same as entering command into dfhack console. Best
used as a keybinding. Can be called with optional "entry" that will start prompt with
that pre-filled.

.. image:: images/command-prompt.png

create-items
============
Spawn arbitrary items under the cursor.

The first argument gives the item category, the second gives the material,
and the optionnal third gives the number of items to create (defaults to 20).

Currently supported item categories: ``boulder``, ``bar``, ``plant``, ``log``,
``web``.

Instead of material, using ``list`` makes the script list eligible materials.

The ``web`` item category will create an uncollected cobweb on the floor.

Note that the script does not enforce anything, and will let you create
boulders of toad blood and stuff like that.
However the ``list`` mode will only show 'normal' materials.

Examples::

    create-items boulders COAL_BITUMINOUS 12
    create-items plant tail_pig
    create-items log list
    create-items web CREATURE:SPIDER_CAVE_GIANT:SILK
    create-items bar CREATURE:CAT:SOAP
    create-items bar adamantine

.. _deathcause:

deathcause
==========
Focus a body part ingame, and this script will display the cause of death of
the creature.
Also works when selecting units from the (``u``) unitlist viewscreen.

dfusion
=======
Interface to a lecacy script system.

.. _digfort:

digfort
=======
A script to designate an area for digging according to a plan in csv format.

This script, inspired from quickfort, can designate an area for digging.
Your plan should be stored in a .csv file like this::

    # this is a comment
    d;d;u;d;d;skip this tile;d
    d;d;d;i

Available tile shapes are named after the 'dig' menu shortcuts:
``d`` for dig, ``u`` for upstairs, ``d`` downstairs, ``i`` updown,
``h`` channel, ``r`` upward ramp, ``x`` remove designation.
Unrecognized characters are ignored (eg the 'skip this tile' in the sample).

Empty lines and data after a ``#`` are ignored as comments.
To skip a row in your design, use a single ``;``.

One comment in the file may contain the phrase ``start(3,5)``. It is interpreted
as an offset for the pattern: instead of starting at the cursor, it will start
3 tiles left and 5 tiles up from the cursor.

The script takes the plan filename, starting from the root df folder (where
``Dwarf Fortress.exe`` is found).

.. _drain-aquifer:

drain-aquifer
=============
Remove all 'aquifer' tag from the map blocks. Irreversible.

.. _elevate-mental:

elevate-mental
==============
Set all mental attributes of a dwarf to 2600, which is very high.
Other numbers can be passed as an argument:  ``elevate-mental 100``
for example would make the dwarf very stupid indeed.

.. _elevate-physical:

elevate-physical
================
As for elevate-mental, but for physical traits.  High is good for soldiers,
while having an ineffective hammerer can be useful too...

.. _exportlegends:

exportlegends
=============
Controls legends mode to export data - especially useful to set-and-forget large
worlds, or when you want a map of every site when there are several hundred.

The 'info' option exports more data than is possible in vanilla, to a
``region-date-legends_plus.xml`` file developed to extend the World
Viewer utility and potentially compatible with others.

Options:

:info:  Exports the world/gen info, the legends XML, and a custom XML with more information
:sites: Exports all available site maps
:maps:  Exports all seventeen detailed maps
:all:   Equivalent to calling all of the above, in that order

exterminate
===========
Kills any unit of a given race.

With no argument, lists the available races and count eligible targets.

With the special argument ``him``, targets only the selected creature.

With the special argument ``undead``, targets all undeads on the map,
regardless of their race.

When specifying a race, a caste can be specified to further restrict the
targeting. To do that, append and colon and the caste name after the race.

Any non-dead non-caged unit of the specified race gets its ``blood_count``
set to 0, which means immediate death at the next game tick. For creatures
such as vampires, it also sets animal.vanish_countdown to 2.

An alternate mode is selected by adding a 2nd argument to the command,
``magma``. In this case, a column of 7/7 magma is generated on top of the
targets until they die (Warning: do not call on magma-safe creatures. Also,
using this mode on birds is not recommended.)

Will target any unit on a revealed tile of the map, including ambushers,
but ignore caged/chained creatures.

Ex::

    exterminate gob
    exterminate gob:male

To kill a single creature, select the unit with the 'v' cursor and::

    exterminate him

To purify all elves on the map with fire (may have side-effects)::

    exterminate elve magma

fixnaked
========
Removes all unhappy thoughts due to lack of clothing.

.. _fix-ster:

fix-ster
========
Utilizes the orientation tag to either fix infertile creatures or inflict
infertility on creatures that you do not want to breed.  Usage::

    fix-ster [fert|ster] [all|animals|only:<creature>]

``fert`` or ``ster`` is a required argument; whether to make the target fertile
or sterile.  Optional arguments specify the target: no argument for the
selected unit, ``all`` for all units on the map, ``animals`` for all non-dwarf
creatures, or ``only:<creature>`` to only process matching creatures.

.. _forum-dwarves:

forum-dwarves
=============
Saves a copy of a text screen, formatted in bbcode for posting to the Bay12 Forums.
Use ``forum-dwarves help`` for more information.

.. _full-heal:

full-heal
=========
Attempts to fully heal the selected unit.  ``full-heal -r`` attempts to resurrect the unit.

.. _gaydar:

gaydar
======
Shows the sexual orientation of units, useful for social engineering or checking
the viability of livestock breeding programs.  Use ``gaydar -help`` for information
on available filters for orientation, citizenship, species, etc.

growcrops
=========
Instantly grow seeds inside farming plots.

With no argument, this command list the various seed types currently in
use in your farming plots.
With a seed type, the script will grow 100 of these seeds, ready to be
harvested. You can change the number with a 2nd argument.

For example, to grow 40 plump helmet spawn::

    growcrops plump 40

.. _hfs-pit:

hfs-pit
=======
Creates a pit to the underworld at the cursor.

Takes three arguments:  diameter of the pit in tiles, whether to wall off
the pit, and whether to insert stairs.  If no arguments are given, the default
is "hfs-pit 1 0 0", ie single-tile wide with no walls or stairs.::

    hfs-pit 4 0 1
    hfs-pit 2 1 0

First example is a four-across pit with stairs but no walls; second is a
two-across pit with stairs but no walls.

.. _hotkey-notes:

hotkey-notes
============
Lists the key, name, and jump position of your hotkeys in the DFHack console.

item-descriptions
=================
Exports a table with custom description text for every item in the game.
Used by `view-item-info`.

lever
=====
Allow manipulation of in-game levers from the dfhack console.

Can list levers, including state and links, with::

    lever list

To queue a job so that a dwarf will pull the lever 42, use ``lever pull 42``.
This is the same as 'q'uerying the building and queue a 'P'ull request.

To magically toggle the lever immediately, use::

    lever pull 42 --now

locate-ore
==========
Scan the map for metal ores.

Finds and designate for digging one tile of a specific metal ore.
Only works for native metal ores, does not handle reaction stuff (eg STEEL).

When invoked with the ``list`` argument, lists metal ores available on the map.

Examples::

    locate-ore list
    locate-ore hematite
    locate-ore iron

log-region
==========
When enabled in dfhack.init, each time a fort is loaded identifying information
will be written to the gamelog.  Assists in parsing the file if you switch
between forts, and adds information for story-building.

lua
===
There are the following ways to invoke this command:

1. ``lua`` (without any parameters)

   This starts an interactive lua interpreter.

2. ``lua -f "filename"`` or ``lua --file "filename"``

   This loads and runs the file indicated by filename.

3. ``lua -s ["filename"]`` or ``lua --save ["filename"]``

   This loads and runs the file indicated by filename from the save
   directory. If the filename is not supplied, it loads "dfhack.lua".

4. ``:lua`` *lua statement...*

   Parses and executes the lua statement like the interactive interpreter would.

make-legendary
==============
Makes the selected dwarf legendary in one skill, a group of skills, or all
skills.  View groups with ``make-legendary classes``, or all skills with
``make-legendary list``.  Use ``make-legendary MINING`` when you need something
dug up, or ``make-legendary all`` when only perfection will do.

make-monarch
============
Make the selected unit King or Queen of your civilisation.

.. _markdown:

markdown
========
Save a copy of a text screen in markdown (for reddit among others).
Use ``markdown help`` for more details.

masspit
=======
Designate all creatures in cages on top of a pit/pond activity zone for pitting.
Works best with an animal stockpile on top of the zone.

Works with a zone number as argument (eg ``Activity Zone #6`` -> ``masspit 6``)
or with the game cursor on top of the area.

multicmd
========
Run multiple dfhack commands. The argument is split around the
character ; and all parts are run sequentially as independent
dfhack commands. Useful for hotkeys.

Example::

    multicmd locate-ore iron ; digv

points
======
Sets available points at the embark screen to the specified number.  Eg.
``points 1000000`` would allow you to buy everything, or ``points 0`` would
make life quite difficult.

.. _position:

position
========
Reports the current time:  date, clock time, month, and season.  Also reports
location:  z-level, cursor position, window size, and mouse location.

pref-adjust
===========
A two-stage script:  ``pref-adjust clear`` removes preferences from all dwarves,
and ``pref-adjust`` inserts an 'ideal' set which is easy to satisfy::

    Feb Idashzefon likes wild strawberries for their vivid red color, fisher berries
    for their round shape, prickle berries for their precise thorns, plump helmets
    for their rounded tops, prepared meals, plants, drinks, doors, thrones, tables and
    beds. When possible, she prefers to consume wild strawberries, fisher berries,
    prickle berries, plump helmets, strawberry wine, fisher berry wine, prickle berry
    wine, and dwarven wine.

.. _putontable:

putontable
==========
Makes item appear on the table, like in adventure mode shops. Arguments:  '-a'
or '--all' for all items.

quicksave
=========
If called in dwarf mode, makes DF immediately auto-save the game by setting a flag
normally used in seasonal auto-save.

region-pops
===========
Show or modify the populations of animals in the region.

Usage:

:region-pops list [pattern]:
        Lists encountered populations of the region, possibly restricted by pattern.
:region-pops list-all [pattern]:
        Lists all populations of the region.
:region-pops boost <TOKEN> <factor>:
        Multiply all populations of TOKEN by factor.
        If the factor is greater than one, increases the
        population, otherwise decreases it.
:region-pops boost-all <pattern> <factor>:
        Same as above, but match using a pattern acceptable to list.
:region-pops incr <TOKEN> <factor>:
        Augment (or diminish) all populations of TOKEN by factor (additive).
:region-pops incr-all <pattern> <factor>:
        Same as above, but match using a pattern acceptable to list.

.. _rejuvenate:

rejuvenate
==========
Set the age of the selected dwarf to 20 years.  Useful if valuable citizens are
getting old, or there are too many babies around...

.. _remove-stress:

remove-stress
=============
Sets stress to -1,000,000; the normal range is 0 to 500,000 with very stable or
very stressed dwarves taking on negative or greater values respectively.
Applies to the selected unit, or use "remove-stress -all" to apply to all units.

remove-wear
===========
Sets the wear on all items in your fort to zero.

.. _repeat:

repeat
======
Repeatedly calls a lua script at the specified interval.

This allows neat background changes to the function of the game, especially when
invoked from an init file.  For detailed usage instructions, use ``repeat -help``.

Usage examples::

    repeat -name jim -time delay -timeUnits units -printResult true -command [ printArgs 3 1 2 ]
    repeat -time 1 -timeUnits months -command [ multicmd cleanowned scattered x; clean all ] -name clean

The first example is abstract; the second will regularly remove all contaminants
and worn items from the game.

``-name`` sets the name for the purposes of cancelling and making sure you don't schedule the
same repeating event twice.  If not specified, it's set to the first argument after ``-command``.
``-time delay -timeUnits units``; delay is some positive integer, and units is some valid time
unit for ``dfhack.timeout(delay,timeUnits,function)``.  ``-command [ ... ]`` specifies the
command to be run.

setfps
======
Run ``setfps <number>`` to set the FPS cap at runtime, in case you want to watch
combat in slow motion or something :)

.. _show-unit-syndromes:

show-unit-syndromes
===================
Show syndromes affecting units and the remaining and maximum duration, along
with (optionally) substantial detail on the effects.

Use one or more of the following options:

:showall:               Show units even if not affected by any syndrome
:showeffects:           Show detailed effects of each syndrome
:showdisplayeffects:    Show effects that only change the look of the unit
:ignorehiddencurse:     Hide syndromes the user should not be able to know about (TODO)
:selected:              Show selected unit
:dwarves:               Show dwarves
:livestock:             Show livestock
:wildanimals:           Show wild animals
:hostile:               Show hostiles (e.g. invaders, thieves, forgotten beasts etc)
:world:                 Show all defined syndromes in the world
:export:                ``export:<filename>`` sends output to the given file, showing all
                        syndromes affecting each unit with the maximum and present duration.

.. _siren:

siren
=====
Wakes up sleeping units, cancels breaks and stops parties either everywhere,
or in the burrows given as arguments. In return, adds bad thoughts about
noise, tiredness and lack of protection. Also, the units with interrupted
breaks will go on break again a lot sooner. The script is intended for
emergencies, e.g. when a siege appears, and all your military is partying.

soundsense-season
=================
It is a well known issue that Soundsense cannot detect the correct
current season when a savegame is loaded and has to play random
season music until a season switch occurs.

This script registers a hook that prints the appropriate string
to gamelog.txt on every map load to fix this. For best results
call the script from ``dfhack.init``.

source
======
Create an infinite magma or water source or drain on a tile.

This script registers a map tile as a liquid source, and every 12 game ticks
that tile receives or remove 1 new unit of flow based on the configuration.

Place the game cursor where you want to create the source (must be a
flow-passable tile, and not too high in the sky) and call::

    source add [magma|water] [0-7]

The number argument is the target liquid level (0 = drain, 7 = source).

To add more than 1 unit everytime, call the command again on the same spot.

To delete one source, place the cursor over its tile and use ``source delete``.
To remove all existing sources, call ``source clear``.

The ``list`` argument shows all existing sources.

Examples::

    source add water     - water source
    source add magma 7   - magma source
    source add water 0   - water drain

startdwarf
==========
Use at the embark screen to embark with the specified number of dwarves.  Eg.
``startdwarf 500`` would lead to a severe food shortage and FPS issues, while
``startdwarf 10`` would just allow a few more warm bodies to dig in.
The number must be 7 or greater.

stripcaged
==========
For dumping items inside cages. Will mark selected items for dumping, then
a dwarf may come and actually dump it. See also `autodump`.

With the ``items`` argument, only dumps items laying in the cage, excluding
stuff worn by caged creatures. ``weapons`` will dump worn weapons, ``armor``
will dump everything worn by caged creatures (including armor and clothing),
and ``all`` will dump everything, on a creature or not.

``stripcaged list`` will display on the dfhack console the list of all cages
and their item content.

Without further arguments, all commands work on all cages and animal traps on
the map. With the ``here`` argument, considers only the in-game selected cage
(or the cage under the game cursor). To target only specific cages, you can
alternatively pass cage IDs as arguments::

  stripcaged weapons 25321 34228

.. _superdwarf:

superdwarf
==========
Similar to `fastdwarf`, per-creature.

To make any creature superfast, target it ingame using 'v' and::

    superdwarf add

Other options available: ``del``, ``clear``, ``list``.

This script also shortens the 'sleeping' and 'on break' periods of targets.

.. _teleport:

teleport
========
Teleports a unit to given coordinates.

Examples::

    teleport -showunitid                 - prints unitid beneath cursor
    teleport -showpos                    - prints coordinates beneath cursor
    teleport -unit 1234 -x 56 -y 115 -z 26  - teleports unit 1234 to 56,115,26

undump-buildings
================
Undesignates building base materials for dumping.

.. _unsuspend:

unsuspend
=========
Unsuspend jobs in workshops, on a one-off basis.  See `autounsuspend`
for regular use.

.. _view-item-info:

view-item-info
==============
A script to extend the item or unit viewscreen with additional information
including a custom description of each item (when available), and properties
such as material statistics, weapon attacks, armor effectiveness, and more.

The associated script ``item-descriptions.lua`` supplies custom descriptions
of items.  Individual descriptions can be added or overridden by a similar
script ``raw/scripts/more-item-descriptions.lua``.  Both work as sparse lists,
so missing items simply go undescribed if not defined in the fallback.

warn-starving
=============
If any (live) units are starving, very thirsty, or very drowsy, the game will
be paused and a warning shown and logged to the console.  Use with the
`repeat` command for regular checks.

Use ``warn-starving all`` to display a list of all problematic units.
