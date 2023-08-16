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

The squad assignment screen can be sorted by name, by migrant wave, or by various military-related skills.

If sorted by "any melee", then the citizen is sorted according to the highest
skill they have in axes, short swords, maces, warhammers, spears, or general
fighting.

If sorted by "any ranged", then the citizen is sorted according to the highest
skill they have in crossbows or general ranged combat.

If sorted by "leadership", then the citizen is sorted according to the highest
skill they have in leader, teacher, or military tactics.

You can search for a dwarf by name by typing in the Search field. You can also
type in the name of any job skill (military-related or not) and dwarves with
any experience in that skill will be shown. For example, to only see citizens
with military tactics skill, type in "tactics".

You can see all the job skill names that you can search for by running::

    :lua @df.job_skill

in `gui/launcher`.
