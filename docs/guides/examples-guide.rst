.. _config-examples-guide:
.. _dfhack-examples-guide:

DFHack config file examples
===========================

The :source:`hack/examples <data/examples>` folder contains ready-to-use
examples of various DFHack configuration files. You can use them by copying them
to appropriate folders where DFHack and its plugins can find them (details
below). You can use them unmodified, or you can customize them to better suit
your preferences.

The ``init/`` subfolder
-----------------------

The :source:`init/ <data/dfhack-config/init/examples>` subfolder contains useful
DFHack `init-files` that you can copy into your :file:`dfhack-config/init`
folder -- the same directory as ``dfhack.init``.

.. _onMapLoad-dreamfort-init:

:source:`onMapLoad_dreamfort.init <data/dfhack-config/init/examples/onMapLoad_dreamfort.init>`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the config file that designed for the `dreamfort` set of blueprints, but
it is useful (and customizable) for any fort. It includes the following config:

- Calls `ban-cooking` for items that have important alternate uses and should
  not be cooked. This configuration is only set when a fortress is first
  started, so later manual changes will not be overridden.
- Automates calling of various fort maintenance scripts, like `cleanowned` and
  `fix/stuckdoors`.
- Keeps your manager orders intelligently ordered with `orders` ``sort`` so no
  orders block other orders from ever getting completed.
- Periodically enqueues orders to shear and milk shearable and milkable pets.
- Sets up `autofarm` to grow 30 units of every crop, except for pig tails, which
  is set to 150 units to support the textile industry.
- Sets up `seedwatch` to protect 30 of every type of seed.
- Configures `prioritize` to automatically boost the priority of important and
  time-sensitive tasks that could otherwise get ignored in busy forts, like
  hauling food, tanning hides, storing items in vehicles, pulling levers, and
  removing constructions.
- Optimizes `autobutcher` settings for raising geese, alpacas, sheep, llamas,
  and pigs. Adds sensible defaults for all other animals, including dogs and
  cats. There are instructions in the file for customizing the settings for
  other combinations of animals. These settings are also only set when a
  fortress is first started, so any later changes you make to autobutcher
  settings won't be overridden.
- Enables `automelt`, `tailor`, `zone`, `nestboxes`, and `autonestbox`.
