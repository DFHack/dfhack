labormanager
============

.. dfhack-tool::
    :summary: Automatically manage dwarf labors via work details.
    :tags: fort auto labors

``labormanager`` is the modern mode of the `autolabor` plugin. Instead of
writing the labor matrix directly, it expresses labor policy through Dwarf
Fortress's work detail system, so the game's job auction chooses which
dwarves take which jobs. The plugin's managed work details are visible on
the work details screen (``u`` -> Labor -> Work Details) with names
beginning with ``auto:``. The DFHack overlay panel on that screen (the
same panel that provides work detail save/load buttons from
`gui/settings-manager`) lets you switch modes and tune the plugin's
priorities.

Since the job auction already prefers skilled dwarves for skilled jobs,
labormanager focuses on the things the auction does poorly:

* **Unskilled job starvation.** The auction tends to starve jobs that
  require no skill (hauling, cleaning, lever pulling, and so on) in favor of
  skilled work. Labormanager maintains a laborer pool, assigned to the
  ``auto:Laborers`` work detail and restricted to unskilled labors, so there
  are always dwarves that *can only* take unskilled work. The pool grows and
  shrinks based on how many unclaimed unskilled job postings are piling up.

* **Skill growth and specialization.** Citizens with a useful skill are
  assigned to a specialized ``auto:`` work detail covering the labors that
  train that skill, so they spend their time practicing rather than hauling.
  If the fortress has a guild hall dedicated to the dwarf's profession,
  labormanager also protects some of that specialist's time so they can
  participate in guild activities (see ``idle-reserve`` below).

Labormanager keeps statistics on which skills the fortress actually
exercises, based on the job postings that resolve on the job board. That
history shapes specialization:

* **No dead skills.** Skills the fort never uses (animal dissection and
  gelding in most forts, for example) get no reserved work detail at all --
  a dwarf whose only talent is an unused skill stays a generalist instead
  of being pinned to irrelevant work.
* **Demand-weighted specialization.** Frequently-used skills are
  prioritized over rarely-used ones when choosing which specialty to
  assign, and units skilled in them are protected harder from laborer
  duty.
* **Quality skills.** Skills whose products carry skill-scaled quality
  (smithing, crafting, cooking, mechanics, engraving, and so on) are
  preferred for higher-skilled workers -- both when choosing a specialty
  and when deciding who to keep out of the laborer pool.
* **Strange-mood shaping.** Dwarves that can still have a strange mood
  (their race has the ``STRANGE_MOODS`` token and they haven't had one
  yet) and that have already started a moodable craft are steered toward
  skills a strange mood can advance -- preferably valuable ones like
  weaponsmithing or armorsmithing -- so a mood pays out in a useful
  legendary skill.

Workshop restrictions are honored as well: when a job is posted at a
workshop that has a permitted-worker list, the engine ensures at least
one listed worker is assigned the job's labor, and when the workshop
restricts jobs by skill level it ensures at least one worker within the
allowed range holds that labor. Units on any workshop's permitted list
are treated as reserved for that shop's work -- they are preferentially
assigned the labors its jobs require and deprioritized for the laborer
pool and unrelated specialties.

Labormanager also reads each citizen's unmet personal needs and biases
assignments toward work that satisfies them:

* Citizens desperate to stay occupied, acquire things, or help others are
  preferentially drafted into the laborer pool, where hauling, stockpiling,
  depot runs, and patient care satisfy those needs.
* Citizens needing social time (gregarious, family-oriented, or just
  wanting to take it easy) are kept out of the restrictive pools so they
  can idle and socialize; guilded specialists with these needs earn guild
  rest sooner.
* Citizens who crave wandering are steered toward fishing, hunting, and
  plant gathering; those craving excitement toward trapping and hunting;
  those craving creativity toward crafts. A completely unskilled citizen
  can be assigned a specialty purely on inclination.

Dwarves on active military duty, in meetings, or assigned to burrows are
left untouched. Children are unaffected: their chores are governed by the
vanilla chores settings.

.. warning::

    While labormanager is enabled it will overwrite the membership of its
    ``auto:`` work details on every cycle. Player-created work details are
    left alone, but manual edits to the managed details will be lost.

Usage
-----

::

    labormanager enable

Anything beyond this is optional. Once enabled in a fortress, it stays
enabled until you explicitly disable it, even if you save and reload your
game.

The same plugin binary also provides the legacy `autolabor` command, which
uses the classic labor-matrix algorithm, and a *monitor* mode that does no
labor management at all -- it only watches the job board and reports
starving job postings (see the end of this document). Only one mode can be
active at a time; use ``labormanager mode`` (or the overlay on the work
details screen) to see or change which mode will run.

Examples
--------

``labormanager balance staffing``
    Prioritize preventing unskilled jobs from starving. Specialists will be
    pulled into the laborer pool more aggressively when hauling backs up.
``labormanager balance skills``
    Prioritize skill growth. Specialists keep their specialized labors even
    when unskilled work is piling up.
``labormanager labor MINE unmanaged``
    Leave the mining labor to the vanilla work details and any manual
    assignments, while managing everything else.

Advanced usage
--------------

``labormanager list``
    List every labor, whether labormanager is managing it, and which labors
    currently have starving (long-unclaimed) job postings.
``labormanager status``
    Show the current balance setting, role counts, job board statistics
    (how many postings resolved, their average and longest waits, and how
    many are currently starving), and the most-used skills observed on the
    job board.
``labormanager balance <0-4|staffing|lean-staffing|balanced|lean-skills|skills>``
    Set where the balance slider sits. Lower values grow the laborer pool
    more aggressively to keep unskilled jobs staffed; higher values protect
    skill specialization. This is the same control shown as a slider on the
    work details overlay.
``labormanager idle-reserve <0-100>``
    Percentage of time to keep free for specialists whose profession has a
    matching guild hall, so they can take part in guild demonstrations and
    other off-duty activities. Defaults to 30.
``labormanager labor <labor> managed|unmanaged``
    Include or exclude a specific labor from labormanager's work details.
    Unmanaged labors are entirely governed by the player's own work detail
    configuration.
``labormanager allow-fishing|forbid-fishing``
    Allow/disallow fisherdwarf specialists. Fishing is forbidden by default
    and also requires a fishery.
``labormanager allow-hunting|forbid-hunting``
    Allow/disallow hunter specialists. Hunting is forbidden by default and
    also requires a butchery.
``labormanager mode [legacy|modern|monitor]``
    Show or change which engine runs when the plugin is enabled. The same
    command exists under `autolabor`. In monitor mode the plugin performs
    no labor management -- all work details are left to the player -- but
    still tracks the job board and shows the task starvation warning.

Monitor mode (``labormanager mode monitor`` or the work details overlay)
runs the same tracking without any labor management, for players who want
the starvation warning while keeping full manual control.

The tracking works as follows: job postings that sit unclaimed on the job
board are timed, and if any posting goes more than a full game day (1200
ticks) without being
claimed, a "task starvation" warning appears in the DFHack notification
panel (alongside the other fort warnings), naming the longest-starving
job and how long it has been waiting; clicking the warning zooms the map
to that job's location. The warning can be toggled in `gui/notify` under
``labor_starvation``.

All of labormanager's accumulated state -- job posting ages, skill usage
history, starvation streaks, and the guild rest rotation -- is saved with
the fortress and restored when the save is loaded, so statistics and
rotation carry over between play sessions.
