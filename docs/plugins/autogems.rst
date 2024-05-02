autogems
========

.. dfhack-tool::
    :summary: Automatically cut rough gems.
    :tags: untested fort auto workorders
    :no-command:

.. dfhack-command:: autogems-reload
    :summary: Reloads the autogems configuration file.

Automatically cut rough gems. This plugin periodically scans your stocks of
rough gems and creates manager orders for cutting them at a Jeweler's Workshop.

Usage
-----

``enable autogems``
    Enables the plugin and starts autocutting gems according to its
    configuration.
``autogems-reload``
    Reloads the autogems configuration file. You might need to do this if you
    have manually modified the contents while the game is running.

Run `gui/autogems` for a configuration UI, or access the ``Auto Cut Gems``
option from the Current Workshop Orders screen (:kbd:`o`-:kbd:`W`).
