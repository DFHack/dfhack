autolabor
=========

.. dfhack-tool::
    :summary: Automatically manage dwarf labors.
    :tags: fort auto labors

Autolabor attempts to keep as many dwarves as possible busy while allowing
dwarves to specialize in specific skills.

Autolabor frequently checks how many jobs of each type are available and sets
labors proportionally in order to get them all done quickly. Labors with
equipment -- mining, hunting, and woodcutting -- which are abandoned if labors
change mid-job, are handled slightly differently to minimise churn.

Dwarves on active military duty or dwarves assigned to burrows are left
untouched by autolabor.

.. warning::

    autolabor will override any manual changes you make to labors while it is
    enabled, including through other tools such as Dwarf Therapist.

Usage::

    enable autolabor

Anything beyond this is optional - autolabor works well with the default
settings. Once you have enabled it in a fortress, it stays enabled until you
explicitly disable it, even if you save and reload your game.

By default, each labor is assigned to between 1 and 200 dwarves (2-200 for
mining). 33% of the workforce become haulers, who handle all hauling jobs as
well as cleaning, pulling levers, recovering wounded, removing constructions,
and filling ponds. Other jobs are automatically assigned as described above.
Each of these settings can be adjusted.

Jobs are rarely assigned to nobles with responsibilities for meeting diplomats
or merchants, never to the chief medical dwarf, and less often to the bookeeper
and manager.

Hunting is never assigned without a butchery, and fishing is never assigned
without a fishery.

For each labor, a preference order is calculated based on skill, biased against
masters of other trades and excluding those who can't do the job. The labor is
then added to the best <minimum> dwarves for that labor, then to additional
dwarfs that meet any of these conditions:

* The dwarf is idle and there are no idle dwarves assigned to this labor
* The dwarf has non-zero skill associated with the labor
* The labor is mining, hunting, or woodcutting and the dwarf currently has it enabled.

We stop assigning dwarves when we reach the maximum allowed.

Examples
--------

``autolabor MINE 5``
    Keep at least 5 dwarves with mining enabled.
``autolabor CUT_GEM 1 1``
    Keep exactly 1 dwarf with gemcutting enabled.
``autolabor COOK 1 1 3``
    Keep 1 dwarf with cooking enabled, selected only from the top 3.
``autolabor FEED_WATER_CIVILIANS haulers``
    Have haulers feed and water wounded dwarves.
``autolabor CUTWOOD disable``
    Turn off autolabor for wood cutting.

Advanced usage
--------------

``autolabor list``
    List current status of all labors. Use this command to see the IDs for all
    labors.
``autolabor status``
    Show basic status information.
``autolabor <labor> <minimum> [<maximum>] [<talent pool>]``
    Set range of dwarves assigned to a labor, optionally specifying the size of
    the pool of most skilled dwarves that will ever be considered for this
    labor.
``autolabor <labor> haulers``
    Set a labor to be handled by hauler dwarves.
``autolabor <labor> disable``
    Turn off autolabor for a specific labor.
``autolabor reset-all|<labor> reset``
    Return a labor (or all labors) to the default handling.

See `autolabor-artisans` for a differently-tuned setup.
