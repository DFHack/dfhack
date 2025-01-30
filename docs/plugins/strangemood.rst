strangemood
===========

.. dfhack-tool::
    :summary: Trigger a strange mood.
    :tags: fort armok units

Usage
-----

::

    strangemood [<options>]

Example
-------

``strangemood --force --unit --type secretive --skill armorsmith``
    Trigger a strange mood for the selected unit that will cause them to become
    a legendary armorsmith.

Options
-------

``--force``
    Ignore normal strange mood preconditions (no recent mood, minimum moodable
    population, artifact limit not reached, etc.).
``--unit``
    Make the strange mood strike the selected unit instead of picking one
    randomly. Unit eligibility is still enforced (unless ``--force`` is also
    specified).
``--type <type>``
    Force the mood to be of a particular type instead of choosing randomly based
    on happiness. Valid values are "fey", "secretive", "possessed", "fell", and
    "macabre".
``--skill <skill>``
    Force the mood to use a specific skill instead of choosing the highest
    moodable skill. Valid values are "miner", "carpenter", "engraver", "mason",
    "stonecutter", "stonecarver", "tanner", "weaver", "clothier",
    "weaponsmith",  "armorsmith", "metalsmith", "gemcutter", "gemsetter",
    "woodcrafter", "stonecrafter", "metalcrafter", "glassmaker",
    "leatherworker", "bonecarver", "bowyer", and "mechanic".

Known limitations: if the selected unit is currently performing a job, the mood
will not be triggered.
