sort
====

.. dfhack-tool::
    :summary: Search and sort lists shown in the DF interface.
    :tags: fort productivity interface
    :no-command:

The ``sort`` tool provides overlays that sorting and searching options for
lists displayed in the DF interface.

Searching and sorting functionality is provided by `overlay` widgets, and widgets for individual lists can be moved via `gui/overlay` or turned on or off via `gui/control-panel`.

Squad assignment overlay
------------------------

You can search for a dwarf by name by typing in the Search field. The search
field is always focused, so any lowercase letter you type will appear there.

The squad assignment screen can be sorted by name, by arrival order, by stress,
by various military-related skills, or by long-term military potential.

If sorted by "melee effectiveness" (the default), then the citizens are sorted
according to how well they will perform in battle when using the weapon they
have the most skill in. The effectiveness rating also takes into account
physical and mental attributes as well as general fighting (non-weapon) skills.

The "ranged effectiveness" sort order does a similar sort for expected
effectiveness with a crossbow. This sort also takes into account relevant
physical and mental attributes.

The "effectiveness" sorts are the ones you should be using if you need the best
squad you can make right now. The numbers to the left of the unit list indicate
exactly how effective that dwarf is expected to be. Light green numbers
indicate the best of the best, while red numbers indicate dwarves that will not
be effective in the military in their current state (though see "melee
potential" and "ranged potential" sorts below for predictions about future
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

The squad assignment panel also offers options for filtering which dwarves are
shown. Each filter option can by cycled through "Include", "Only", and
"Exclude" settings. "Include" does no filtering, "Only" shows only units that
match the filter, and "Exclude" shows only units that do *not* match the filter.

The following filters are provided:

- Units that are assigned to other squads
- Elected and appointed officials (e.g. mayor, priests, tavern keepers, etc.)
- Nobility (e.g. monarch, barons, counts, etc.)
- Mothers with infants (you may not want mothers using their babies as shields)
- Weak mental fortitude (units that have facets and values that indicate that
  they will react poorly to the stresses of battle)
- Critically injured (units that have lost their ability to see, grasp weapons,
  or walk)

"Melee skill effectiveness", "ranged skill effectiveness", "melee combat potential"
and "ranged combat potential" are explained in detail here:
https://www.reddit.com/r/dwarffortress/comments/163kczo/enhancing_military_candidate_selection_part_3/
"Mental stability" is explained here:
https://www.reddit.com/r/dwarffortress/comments/1617s11/enhancing_military_candidate_selection_part_2/
