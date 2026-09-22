stockflow
=========

.. dfhack-tool::
    :summary: Queue manager jobs based on free space in stockpiles.
    :tags: fort auto workorders stockpile

With this plugin, you can tally up free space in specific stockpiles and
automatically queue jobs through the manager to produce items to fill the free
space.

When the plugin is enabled, selecting a stockpile in the UI will show the
stockflow overlay panel with two options:

* :kbd:`j`:  Select a job to order, from an interface like the manager's screen.
* :kbd:`J`:  Cycle between several options for how many such jobs to order.

About once per in-game day, new work orders will be placed on the manager's
queue for each such selection, reduced by the number of identical orders
already in the queue.

This plugin is similar to `workflow`, but uses stockpiles to manage job triggers
instead of abstract stock quantities.

Usage
-----

``enable stockflow``
    Enable the plugin.
``stockflow status``
    Display whether the plugin is enabled.
``stockflow list``
    List any work order settings for your stockpiles.
