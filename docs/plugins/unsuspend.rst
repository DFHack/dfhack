unsuspend
=========

.. dfhack-tool::
    :summary: Unsuspends building construction jobs.
    :tags: fort productivity jobs

Unsuspends building construction jobs, except for jobs managed by `buildingplan`
and those where water flow is greater than 1. This allows you to quickly recover
if a bunch of jobs were suspended due to the workers getting scared off by
wildlife or items temporarily blocking building sites.

See `suspendmanager` in `gui/control-panel` to automatically suspend and
unsuspend jobs.

Usage
-----

``unsuspend``

Options
-------

``-q``, ``--quiet``
    Disable text output

``-s``, ``--skipblocking``
    Don't unsuspend construction jobs that risk blocking other jobs
