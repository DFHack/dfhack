stockflow
=========
Allows the fortress bookkeeper to queue jobs through the manager,
based on space or items available in stockpiles.

Inspired by `workflow`.

Usage:

``stockflow enable``
    Enable the plugin.
``stockflow disable``
    Disable the plugin.
``stockflow fast``
    Enable the plugin in fast mode.
``stockflow list``
    List any work order settings for your stockpiles.
``stockflow status``
    Display whether the plugin is enabled.

While enabled, the :kbd:`q` menu of each stockpile will have two new options:

* :kbd:`j`:  Select a job to order, from an interface like the manager's screen.
* :kbd:`J`:  Cycle between several options for how many such jobs to order.

Whenever the bookkeeper updates stockpile records, new work orders will
be placed on the manager's queue for each such selection, reduced by the
number of identical orders already in the queue.

In fast mode, new work orders will be enqueued once per day, instead of
waiting for the bookkeeper.
