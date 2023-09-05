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

The squad assignment screen can be sorted by name, by migrant wave, by stress,
by various military-related skills or by long-term military potential.

If sorted by "any melee", then the citizen is sorted according to the "melee
skill effectiveness". This rating uses the highest skill they have in axes, short
swords, maces, warhammers or spears along with physical and mental attributes and
general fighting skill. Citizens with higher rating are expected to be more
effective in melee combat with their corresponding weapon.

If sorted by "any ranged", then the citizen is sorted according to the "ranged
skill effectiveness". This rating uses crossbow and general archery skills
along with mental and physical attributes. Citizens with higher rating are
expected to be more effective in ranged combat.

If sorted by "leadership", then the citizen is sorted according to the highest
skill they have in leader, teacher, or military tactics.

If sorting is done by "mental stability" citizens are arranged based on their
mental stability rating. This rating is a measure that takes into account
facets and values of an individual and correlates to better stress values.
It is designed to be higher for more stress-resistant citizens.

If sorting is done by "melee potential" citizens are arranged based on
their "melee combat potential" rating. This rating is a statistical measure
that takes into account genetic predispositions in physical and mental
attributes, as well as body size. Dwarves (and other humanoid creatures) with
higher rating are expected to be more effective in melee combat if they train
their attributes to their genetic maximum.

If sorting is done by "ranged potential" citizens are arranged based on their
ranged combat potential rating. This rating is a statistical measure that takes into
account genetic predispositions in physical and mental attributes. Dwarves
(and other humanoid creatures) with higher rating are expected to be more
effective in ranged combat if they train their attributes to the maximum.

You can search for a dwarf by name by typing in the Search field. You can also
type in the name of any job skill (military-related or not) and dwarves with
any experience in that skill will be shown. For example, to only see citizens
with military tactics skill, type in "tactics".

"Melee skill effectiveness", "ranged skill effectiveness", "melee combat potential"
and "ranged combat potential" are explained in detail here:
https://www.reddit.com/r/dwarffortress/comments/163kczo/enhancing_military_candidate_selection_part_3/
"Mental stability" is explained here:
https://www.reddit.com/r/dwarffortress/comments/1617s11/enhancing_military_candidate_selection_part_2/

You can see all the job skill names that you can search for by running::

    :lua @df.job_skill

in `gui/launcher`.
