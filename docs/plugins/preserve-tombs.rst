preserve-tombs
==============

.. dfhack-tool::
    :summary: Fix tombs being unassigned to units on death
    :tags: fort bugfix

If you find that the tombs you assign to units get unassigned from them when
they die (e.g. your nobles), this tool can help fix that.

Usage
-----

::

    enable preserve-tombs
    preserve-tombs status

This tool runs in the background. You can check the status of the plugin
by running ``preserve-tombs status`` which will show whether the plugin
is enabled and if so, display a list of all tracked tomb assignments
to living units.
