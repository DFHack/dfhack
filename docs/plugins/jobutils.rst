.. _job:

job
===
Command for general job query and manipulation.

Options:

*no extra options*
    Print details of the current job. The job can be selected
    in a workshop, or the unit/jobs screen.
**list**
    Print details of all jobs in the selected workshop.
**item-material <item-idx> <material[:subtoken]>**
    Replace the exact material id in the job item.
**item-type <item-idx> <type[:subtype]>**
    Replace the exact item type id in the job item.

job-duplicate
=============
In :kbd:`q` mode, when a job is highlighted within a workshop or furnace
building, calling ``job-duplicate`` instantly duplicates the job.

:dfhack-keybind:`job-duplicate`

job-material
============
Alter the material of the selected job.  Similar to ``job item-material ...``

Invoked as::

    job-material <inorganic-token>

:dfhack-keybind:`job-material`

* In :kbd:`q` mode, when a job is highlighted within a workshop or furnace,
  changes the material of the job. Only inorganic materials can be used
  in this mode.
* In :kbd:`b` mode, during selection of building components positions the cursor
  over the first available choice with the matching material.
