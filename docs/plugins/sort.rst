sort
====

.. dfhack-tool::
    :summary: Search and sort lists shown in the DF interface.
    :tags: fort productivity interface
    :no-command:

The ``sort`` tool provides search and sort functionality for lists displayed in
the DF interface.

Searching and sorting functionality is provided by `overlay` widgets, and
widgets for individual lists can be moved via `gui/overlay` or turned on or off
via `gui/control-panel`.

Squad assignment overlay
------------------------

The squad assignment overlay adds annotation, sorting, and filtering
capabilities to the squad assignment list. The currently selected annotation is
shown in the panel to the left of the unit list. Clicking on the arrow icon
above the annotations will sort the unit list by that annotation value, or, if
the list is already sorted by the annotation value, will switch between
descending and ascending sort.

Annotations and sort orders
~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can change which attribute is shown in the annotation panel by clicking on
the ``Show:`` button or hitting the :kbd:`S` hotkey. Alternately, if you
hover the mouse over the ``Show:`` button, a popout panel will appear where you
can select the annotation attribute directly. Selecting a new annotation
attribute will also automatically sort by that attribute. You can then select
one of the vanilla sorts to keep that annotation while sorting by another field.

You can choose to annotate and sort by many useful attributes, including
current combat effectiveness, stress level, various military-related skills,
and long-term military potential. The annotation selection and filter settings
will be saved for you when you close and reopen the squad member assignment
panel.

If "melee effectiveness" is selected (the default), then the citizens are
annotated according to how well they will perform in battle when using the
weapon they have the most skill in. The effectiveness rating also takes into
account physical and mental attributes as well as general fighting (non-weapon)
skills.

Simiarly, the "ranged effectiveness" annotation indicates expected
effectiveness with a crossbow. This rating also takes into account relevant
physical and mental attributes.

The "effectiveness" ratings are the ones you should be using if you need the
best squad you can make right now. The numbers to the left of the unit list
indicate exactly how effective that dwarf is expected to be. Light green numbers
indicate the best of the best, while red numbers indicate dwarves that will not
be effective in the military in their current state (though see "melee
potential" and "ranged potential" ratings below for predictions about future
effectiveness).

The "arrival order" sorts your citizens according to the most recent time they
entered your map. The numbers on the left indicate the relative arrival order,
and the numbers for the group of dwarves that most recently entered the map
will be at the top and be colored bright green. If you run this sort after you
get a new group of migrants, the migrant wave will be colored bright green.
Dwarves that arrived earlier will have numbers in yellow, and your original
dwarves (if any still survive and have never left and re-entered the map) will
have numbers in red.

The "stress" sort order will bring your most stressed dwarves to the top, ready
for addition to a :wiki:`therapy squad <Stress#Recovering_unhappy_dwarves>` to
help improve their mood.

Similarly, sorting by "need for training" will show you the dwarves that are
feeling the most unfocused because they are having their military training
needs unmet.

Both "stress" and "need for training" sorts use the dwarf happiness indicators
to show how dire the dwarf's situation is and how much their mood might be
improved if you add them to an appropriate squad.

If sorting is done by "melee potential", then citizens are arranged based on
genetic predispositions in physical and mental attributes, as well as body
size. Dwarves (and other humanoid creatures) with higher ratings are expected
to be more effective in melee combat if they train their attributes to their
genetic maximum.

Similarly, the "ranged potential" sort orders citizens by genetic
predispositions in physical and mental attributes that are relevant to ranged
combat. Dwarves (and other humanoid creatures) with higher rating are expected
to be more effective in ranged combat if they train their attributes to the
maximum.

"Melee skill effectiveness", "ranged skill effectiveness", "melee combat
potential", and "ranged combat potential" are explained in detail here:
https://www.reddit.com/r/dwarffortress/comments/163kczo/enhancing_military_candidate_selection_part_3/

Filters
~~~~~~~

The squad assignment panel also offers options for filtering which dwarves are
shown. Each filter option can by cycled through "Include", "Only", and
"Exclude" settings. "Include" does no filtering, "Only" shows only units that
match the filter, and "Exclude" shows only units that do *not* match the filter.

The following filters are provided:

- Other squads: Units that are assigned to other squads
- Officials: Elected and appointed officials, like you mayor, priests, tavern
  keepers, etc.
- Nobility: Your monarch, barons, counts, etc.
- Infant carriers: Mothers with infants (you may not want mothers using their
  babies as shields)
- Hates combat: Units that have facets and values that indicate that they will
  react poorly to the stresses of battle
- Maimed: Critically injured units that have lost their ability to see, grasp
  weapons, or walk

The "Hates combat" filter is explained in more detail here:
https://www.reddit.com/r/dwarffortress/comments/1617s11/enhancing_military_candidate_selection_part_2/

Info tabs overlay
-----------------

The Info overlay adds search support to many of the fort-wide "Info" panels
(e.g. "Creatures", "Tasks", etc.). When searching for units, you can search by
name (with either English or native language last names), profession, or
special status (like "necromancer"). If there is text in the second column, you
can search for that text as well. This is often a job name or a status, like
"caged". The work animal assignment page can also filter by squad or burrow
membership.

Work animals overlay
--------------------

In addition to the search and filter widgets provided by the Info tabs overlay,
the work animal assignment screen has an additional overlay that annotates each
visible unit with the number of work animals that unit already has.

Interrogation overlay
---------------------

In the interrogation and conviction screens under the "Justice" tab, you can
search for units by name. You can also filter by the classification of the
unit. The classification groups are ordered by how likely a member of that
group is to be involved in a plot. The groups are: All, Risky visitors, Other
visitors, Residents, Citizens, Animals, Deceased, and Others. "Risky" visitors are those who are especially likely to be involved in plots, such as criminals,
necromancers, necromancer experiments, and intelligent undead.

On the interrogations screen, you can also filter units by whether they have
already been interrogated.

Candidates overlay
------------------

When you select the button to choose a candidate to assign to a noble role on
the nobles screen, you can search for units by name, profession, or any of the
skills in which they have achieved at least "novice" level. For example, when
assigning a broker, you can search for "appraisal" to find candidates that have
at least some appraisal skill.

Location selection overlay
--------------------------

When choosing the type of guildhall or temple to dedicate, you can search for
the relevant profession, religion, or deity by name. For temples, you can also
search for the "spheres" associated with the deity or religion, such as
"wealth" or "lies".

You can also choose whether to filter out temple or guildhall types that you
have already established.

Slab engraving overlay
----------------------

When choosing a unit to engrave a slab for, you can search for units by name,
either in their native language or in English (though only their native name
will be displayed). This overlay also adds a filter for showing only units that
would need a slab in order to prevent them rising as a ghost.

World overlay
-------------

Searching is supported for the Artifacts list when viewing the world map (where
you can initiate raids).
