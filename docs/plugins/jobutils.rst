.. _job:

jobutils
========

.. dfhack-tool::
    :summary: Provides commands for interacting with jobs.
    :tags: untested fort inspection jobs
    :no-command:

.. dfhack-command:: job
    :summary: Inspect or modify details of workshop jobs.

.. dfhack-command:: job-duplicate
    :summary: Duplicates the highlighted job.

.. dfhack-command:: job-material
    :summary: Alters the material of the selected job.

Usage
-----

``job``
    Print details of the current job. The job can be selected in a workshop or
    the unit/jobs screen.
``job list``
    Print details of all jobs in the selected workshop.
``job item-material <item-idx> <material[:subtoken]>``
    Replace the exact material id in the job item.
``job item-type <item-idx> <type[:subtype]>``
    Replace the exact item type id in the job item.
``job-duplicate``
    Duplicates the highlighted job. Must be in :kbd:`q` mode and have a workshop
    or furnace building selected.
``job-material <inorganic-token>``
    Alters the material of the selected job (in :kbd:`q` mode) or jumps to the
    selected material when choosing the building component of a planned building
    (in :kbd:`b` mode). Note that this form of the command can only handle
    inorganic materials.

Use the ``job`` and ``job list`` commands to discover the type and material ids
for existing jobs, or use the following commands to see the full lists::

    lua @df.item_type
    lua "for i,mat in ipairs(df.global.world.raws.inorganics) do if mat.material.flags.IS_STONE and not mat.material.flags.NO_STONE_STOCKPILE then print(i, mat.id) end end"

Examples
--------

``job-material GNEISS``
    Change the selected "Construct rock Coffin" job at a Mason's workshop to
    "Construct gneiss coffin".
``job item-material 2 MARBLE``
    Change the selected "Construct Traction Bench" job (which has three source
    items: a table, a mechanism, and a chain) to specifically use a marble
    mechanism.
``job item-type 2 TABLE``
    Change the selected "Encrust furniture with blue jade" job (which has two
    source items: a cut gem and a piece of improvable furniture) to specifically
    use a table instead of just any furniture.
