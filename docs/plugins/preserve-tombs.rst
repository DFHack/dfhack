preserve-tombs
==============

.. dfhack-tool::
    :summary: Fix tombs being unassigned to units on death
    :tags: fort bugfix

If you find that the tombs you assign to units get unassigned from them when
they die (e.g. your nobles), this tool can help fix that.

Usage
-----

``enable preserve-tombs``
    enable the plugin
``preserve-tombs status``
    check the status of the plugin, and if the plugin is enabled,
    lists all tracked tomb assignments
``preserve-tombs update``
    forces an immediate update of the tomb assignments.
``preserve-tombs freq [val]``
    changes the rate at which the plugin rechecks  and updates
    tomb assignments, in ticks (default is ``100``)

This tool runs in the background.