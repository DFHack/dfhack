confirm
=======

.. dfhack-tool::
    :summary: Adds confirmation dialogs for destructive actions.
    :tags: fort interface

Now you can get the chance to avoid accidentally disbanding a squad or deleting a
hauling route in case you hit the key accidentally.

Usage
-----

``enable confirm``, ``confirm enable all``
    Enable all confirmation options. Replace with ``disable`` to disable all.
``confirm enable option1 [option2...]``
    Enable (or ``disable``) specific confirmation dialogs.

When run without parameters, ``confirm`` will report which confirmation dialogs
are currently enabled.
