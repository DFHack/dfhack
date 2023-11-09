dwarfvet
========

.. dfhack-tool::
    :summary: Allow animals to be treated at hospitals.
    :tags: fort gameplay animals

Annoyed that your dragons become useless after a minor injury? Well, with
dwarfvet, injured animals will be treated at a hospital. Dwarfs with the Animal
Caretaker labor enabled will come to the hospital to treat the animals. Normal
medical skills are used (and trained), but no experience is given to the Animal
Caretaker skill itself.

You can enable ``dwarfvet`` in `gui/control-panel`, and you can choose to start
``dwarfvet`` automatically in new forts in the ``Autostart`` tab.

Usage
-----

::

    enable dwarfvet
    dwarfvet [status]
    dwarfvet now

Examples
--------

``dwarfvet``
    Report on how many animals are being treated and how many are in need of
    treatment.

``dwarfvet now``
    Assign injured animals to a free floor spot in a nearby hospital,
    regardless of whether the plugin is enabled.
