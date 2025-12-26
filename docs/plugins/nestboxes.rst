nestboxes
=========

.. dfhack-tool::
    :summary: Protect fertile eggs incubating in a nestbox.
    :tags: fort auto animals
    :no-command:

This plugin will automatically scan for and forbid fertile eggs incubating in a
nestbox so that dwarves won't come to collect them for eating. The eggs will
hatch normally, even when forbidden.

You can control which eggs are collected and which eggs are protected by placing
the nestboxes whose contents should be protected inside a burrow and running
``nestboxes burrow <burrow_name>``. The default behavior of protecting all
fertile eggs in nest boxes can be reestablished by running ``nestboxes all``.

Usage
-----

::

    enable nestboxes
    nestboxes burrow <burrow_name>
    nestbox all
