preserve-tombs
==============

.. dfhack-tool::
    :summary: Preserve tomb assignments when assigned units die.
    :tags: fort bugfix

If you find that the tombs you assign to units get unassigned from them when
they die (e.g. your nobles), this tool can help fix that.

Usage
-----

``enable preserve-tombs``
    enable the plugin
``preserve-tombs [status]``
    check the status of the plugin, and if the plugin is enabled,
    lists all currently tracked tomb assignments
``preserve-tombs now``
    forces an immediate update of the tomb assignments. This plugin
    automatically updates the tomb assignments once every 100 ticks.

This tool runs in the background.
