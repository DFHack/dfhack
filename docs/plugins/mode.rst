mode
====

.. dfhack-tool::
    :summary: See and change the game mode.
    :tags: untested armok dev gameplay

.. warning::

    Only use ``mode`` after making a backup of your save!

    Not all combinations are good for every situation and most of them will
    produce undesirable results. There are a few good ones though.

Usage
-----

``mode``
    Print the current game mode.
``mode set``
    Enter an interactive commandline menu where you can set the game mode.

Examples
--------

Scenario 1:

* You are in fort game mode, managing your fortress and paused.
* You switch to the arena game mode, *assume control of a creature* and then
* switch to adventure game mode.

You just lost a fortress and gained an adventurer.

Scenario 2:

* You are in fort game mode, managing your fortress and paused at the Esc menu.
* You switch to the adventure game mode, assume control of a creature, then save or retire.

You just created a returnable mountain home and gained an adventurer.
