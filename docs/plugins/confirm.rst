confirm
=======

.. dfhack-tool::
    :summary: Adds confirmation dialogs for destructive actions.
    :tags: fort interface

In the base game, it is frightenly easy to destroy hours of work with a single
misclick. Now you can avoid the consequences of accidentally disbanding a squad
(for example), or deleting a hauling route.

Usage
-----

``enable confirm``, ``confirm enable all``
    Enable all confirmation options. Replace with ``disable`` to disable all.
``confirm enable option1 [option2...]``
    Enable (or ``disable``) specific confirmation dialogs.

When run without parameters, ``confirm`` will report which confirmation dialogs
are currently enabled.
