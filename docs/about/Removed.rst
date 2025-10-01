#############
Removed tools
#############

This page lists tools (plugins or scripts) that were previously included in
DFHack but have been removed. It exists primarily so that internal links still
work (e.g. links from the `changelog`).

.. contents:: Contents
  :local:
  :depth: 1

.. _adv-rumors:

adv-rumors
==========
Converted to an `overlay` and merged into `advtools`.

.. _adv-fix-sleepers:

adv-fix-sleepers
================
Renamed to `fix/sleepers`.

.. _autohauler:

autohauler
==========
An automated labor management tool that only addressed hauling labors, leaving the assignment
of skilled labors entirely up to the player. Fundamentally incompatible with the work detail
system of labor management in v50 of Dwarf Fortress.

.. _automaterial:

automaterial
============
Moved frequently used materials to the top of the materials list when building
buildings. Also offered extended options when building constructions. All
functionality has been merged into `buildingplan`.

.. _automelt:

automelt
========
Automatically mark items for melting when they are brought to a monitored
stockpile. Merged into `logistics`.

.. _autotrade:

autotrade
=========
Automatically mark items for trading when they are brought to a monitored
stockpile. Merged into `logistics`.

.. _autounsuspend:

autounsuspend
=============
Replaced by `suspendmanager`.

.. _combine-drinks:

combine-drinks
==============
Replaced by the new `combine` script. Run
``combine here --types=drink``

.. _combine-plants:

combine-plants
==============
Replaced by the new `combine` script. Run
``combine here --types=plants``

.. _command-prompt:

command-prompt
==============
Replaced by `gui/launcher --minimal <gui/launcher>`.

.. _create-items:

create-items
============
Replaced by `gui/create-item`.

.. _deteriorateclothes:

deteriorateclothes
==================
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=clothes``.

.. _deterioratecorpses:

deterioratecorpses
==================
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=corpses``.

.. _deterioratefood:

deterioratefood
===============
Replaced by the new combined `deteriorate` script. Run
``deteriorate --types=food``.

.. _devel/find-offsets:

devel/find-offsets
==================
Used in pre-v50 times for memory structure analysis. No longer useful post-v50.

.. _devel/find-twbt:

devel/find-twbt
===============
Used in pre-v50 times for memory structure analysis. No longer useful post-v50.

.. _devel/prepare-save:

devel/prepare-save
==================
Used in pre-v50 times for memory structure analysis. No longer useful post-v50.

.. _devel/unforbidall:

devel/unforbidall
=================
Replaced by the `unforbid` script. Run ``unforbid all --quiet`` to match the
behavior of the original ``devel/unforbidall`` script.

.. _digfort:

digfort
=======
A script to designate an area for digging according to a plan in csv format.
Please use DFHack's more powerful `quickfort` script instead. You can use your
existing .csv files. Just move them to the ``blueprints`` folder in your DF
installation, and instead of ``digfort file.csv``, run
``quickfort run file.csv``.

.. _drain-aquifer:

drain-aquifer
=============
Replaced by `aquifer` and `gui/aquifer`.

.. _embark-tools:

embark-tools
============
Replaced by `gui/embark-anywhere`. Other functionality was replaced by the DF
v50 UI.

.. _faststart:

faststart
=========
Sped up the initial DF load sequence. Removed since Bay 12 rewrote the startup
sequence and it is now sufficiently fast on its own.

.. _fix-armory:

fix-armory
==========
Allowed the military to store equipment in barracks containers. Removed because
it required a binary patch to DF in order to function, and no such patch has
existed since DF 0.34.11.

.. _fix/build-location:

fix/build-location
==================
The corresponding DF :bug:`5991` was fixed in DF 0.40.05.

.. _fix/diplomats:

fix/diplomats
=============
The corresponding DF :bug:`3295` was fixed in DF 0.40.05.

.. _fix/fat-dwarves:

fix/fat-dwarves
===============
The corresponding DF :bug:`5971` was fixed in DF 0.40.05.

.. _fix/feeding-timers:

fix/feeding-timers
==================
The corresponding DF :bug:`2606` was fixed in DF 0.40.12.

.. _fix/item-occupancy:

fix/item-occupancy
==================
Merged into `fix/occupancy`.

.. _fix/merchants:

fix/merchants
=============
Humans can now make trade agreements. This fix is no longer necessary.

.. _fix/tile-occupancy:

fix/tile-occupancy
==================
Merged into `fix/occupancy`.

.. _fix-unit-occupancy:

fix-unit-occupancy
==================
Merged into `fix/occupancy`.

.. _fortplan:

fortplan
========
Designates furniture for building according to a ``.csv`` file with
quickfort-style syntax. Please use DFHack's more powerful `quickfort`
script instead. You can use your existing .csv files. Just move them to the
``blueprints`` folder in your DF installation, and instead of
``fortplan file.csv`` run ``quickfort run file.csv``.

.. _gui/assign-rack:

gui/assign-rack
===============
This script is no longer useful in current DF versions. The script required a
binpatch <binpatches/needs-patch>`, which has not been available since DF
0.34.11.

.. _gui/automelt:

gui/automelt
============
Replaced by the `stockpiles` overlay and the gui for `logistics`.

.. _gui/create-tree:

gui/create-tree
===============
Replaced by `gui/sandbox`.

.. _gui/dig:

gui/dig
=======
Renamed to `gui/design`.

.. _gui/hack-wish:

gui/hack-wish
=============
Replaced by `gui/create-item`.

.. _gui/manager-quantity:

gui/manager-quantity
====================
Ability to modify manager order quantities has been added to the vanilla UI.

.. _gui/mechanisms:

gui/mechanisms
==============
Linked building interface has been added to the vanilla UI.

.. _gui/no-dfhack-init:

gui/no-dfhack-init
==================
Tool that warned the user when the ``dfhack.init`` file did not exist. Now that
``dfhack.init`` is autogenerated in ``dfhack-config/init``, this warning is no
longer necessary.

.. _masspit:

masspit
=======
Replaced with a GUI version: `gui/masspit`.

.. _max-wave:

max-wave
========
Set population cap based on parameters. Merged into `pop-control`.

.. _modtools/force:

modtools/force
==============
Merged into `force`.

.. _mousequery:

mousequery
==========
Functionality superseded by vanilla v50 interface.

.. _petcapRemover:

petcapRemover
=============
Renamed to `pet-uncapper`.

.. _plants:

plants
======
Renamed to `plant`.

.. _rename:

rename
======
Superseded by vanilla rename capabilities and `gui/rename`.

.. _resume:

resume
======
Allowed you to resume suspended jobs and displayed an overlay indicating
suspended building construction jobs. Replaced by `unsuspend` script.

.. _ruby:
.. _rb:

ruby
====
Support for the Ruby language in DFHack scripts was removed due to the issues
the Ruby library causes when used as an embedded language.

.. _search-plugin:

search
======
Functionality was merged into `sort`.

.. _show-unit-syndromes:

show-unit-syndromes
===================
Replaced with a GUI version: `gui/unit-syndromes`.

.. _stocksettings:

stocksettings
=============
Along with ``copystock``, ``loadstock`` and ``savestock``, replaced with the new
`stockpiles` API.

.. _title-version:

title-version
=============
Replaced with an `overlay`.

.. _unsuspend:

unsuspend
=========
Merged into `suspendmanager`.

.. _warn-starving:

warn-starving
=============
Functionality was merged into `gui/notify`.

.. _warn-stealers:

warn-stealers
=============
Functionality was merged into `gui/notify`.

.. _warn-stuck-trees:

warn-stuck-trees
================
The corresponding DF :bug:`9252` was fixed in DF 0.44.01.

.. _workorder-recheck:

workorder-recheck
=================
Tool to set 'Checking' status of the selected work order, allowing conditions
to be reevaluated. Merged into `orders`.
