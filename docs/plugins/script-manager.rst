script-manager
==============

.. dfhack-tool::
    :summary: Triggers startup tasks for scripts.
    :tags: dev
    :no-command:

This plugin loads all scripts that are declared as modules so that they can
put state change hooks in place for loading persistent data. It also scans for
global ``isEnabled()`` functions and gathers them for script enabled state
tracking and reporting for the `enable` command.

Please see `script-enable-api` for more details.
