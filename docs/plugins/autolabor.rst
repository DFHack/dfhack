autolabor
=========
Automatically manage dwarf labors to efficiently complete jobs.
Autolabor tries to keep as many dwarves as possible busy but
also tries to have dwarves specialize in specific skills.

The key is that, for almost all labors, once a dwarf begins a job it will finish that
job even if the associated labor is removed. Autolabor therefore frequently checks
which dwarf or dwarves should take new jobs for that labor, and sets labors accordingly.
Labors with equipment (mining, hunting, and woodcutting), which are abandoned
if labors change mid-job, are handled slightly differently to minimise churn.

.. warning::

    *autolabor will override any manual changes you make to labors while
    it is enabled, including through other tools such as Dwarf Therapist*

Simple usage:

:enable autolabor:      Enables the plugin with default settings.  (Persistent per fortress)
:disable autolabor:     Disables the plugin.

Anything beyond this is optional - autolabor works well on the default settings.

By default, each labor is assigned to between 1 and 200 dwarves (2-200 for mining).
By default 33% of the workforce become haulers, who handle all hauling jobs as well
as cleaning, pulling levers, recovering wounded, removing constructions, and filling ponds.
Other jobs are automatically assigned as described above.  Each of these settings can be adjusted.

Jobs are rarely assigned to nobles with responsibilities for meeting diplomats or merchants,
never to the chief medical dwarf, and less often to the bookeeper and manager.

Hunting is never assigned without a butchery, and fishing is never assigned without a fishery.

For each labor a preference order is calculated based on skill, biased against masters of other
trades and excluding those who can't do the job.  The labor is then added to the best <minimum>
dwarves for that labor.  We assign at least the minimum number of dwarfs, in order of preference,
and then assign additional dwarfs that meet any of these conditions:

* The dwarf is idle and there are no idle dwarves assigned to this labor
* The dwarf has non-zero skill associated with the labor
* The labor is mining, hunting, or woodcutting and the dwarf currently has it enabled.

We stop assigning dwarfs when we reach the maximum allowed.

Advanced usage:

:autolabor <labor> <minimum> [<maximum>]:
                                Set number of dwarves assigned to a labor.
:autolabor <labor> haulers:     Set a labor to be handled by hauler dwarves.
:autolabor <labor> disable:     Turn off autolabor for a specific labor.
:autolabor <labor> reset:       Return a labor to the default handling.
:autolabor reset-all:           Return all labors to the default handling.
:autolabor list:                List current status of all labors.
:autolabor status:              Show basic status information.

See `autolabor-artisans` for a differently-tuned setup.

Examples:

``autolabor MINE``
        Keep at least 5 dwarves with mining enabled.
``autolabor CUT_GEM 1 1``
        Keep exactly 1 dwarf with gemcutting enabled.
``autolabor COOK 1 1 3``
        Keep 1 dwarf with cooking enabled, selected only from the top 3.
``autolabor FEED_WATER_CIVILIANS haulers``
        Have haulers feed and water wounded dwarves.
``autolabor CUTWOOD disable``
        Turn off autolabor for wood cutting.
