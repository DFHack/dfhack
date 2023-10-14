petcapRemover
=============

.. dfhack-tool::
    :summary: Modify the pet population cap.
    :tags: unavailable

In vanilla DF, pets will not reproduce unless the population is below 50 and the
number of children of that species is below a certain percentage. This plugin
allows removing these restrictions and setting your own thresholds. Pets still
require PET or PET_EXOTIC tags in order to reproduce. In order to make
population more stable and avoid sudden population booms as you go below the
raised population cap, this plugin counts pregnancies toward the new population
cap. It can still go over, but only in the case of multiple births.

Usage
-----

``enable petcapRemover``
    Enables the plugin and starts running with default settings.
``petcapRemover cap <value>``
    Set the new population cap per species to the specified value. If set to 0,
    then there is no cap (good luck with all those animals!). The default cap
    is 100.
``petcapRemover``
    Impregnate female pets that have access to a compatible male, up to the
    population cap.
``petcapRemover every <ticks>``
    Set how often the plugin will cause pregnancies. The default frequency is
    every 10,000 ticks (a little over 8 game days).
``petcapRemover pregtime <ticks>``
    Sets the pregnancy duration to the specified number of ticks. The default
    value is 200,000 ticks, which is the natural pet pregnancy duration.
