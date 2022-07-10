petcapRemover
=============
Allows you to remove or raise the pet population cap. In vanilla
DF, pets will not reproduce unless the population is below 50 and the number of
children of that species is below a certain percentage. This plugin allows
removing the second restriction and removing or raising the first. Pets still
require PET or PET_EXOTIC tags in order to reproduce. Type ``help petcapRemover``
for exact usage. In order to make population more stable and avoid sudden
population booms as you go below the raised population cap, this plugin counts
pregnancies toward the new population cap. It can still go over, but only in the
case of multiple births.

Usage:

:petcapRemover:             cause pregnancies now and schedule the next check
:petcapRemover every n:     set how often in ticks the plugin checks for possible pregnancies
:petcapRemover cap n:       set the new cap to n. if n = 0, no cap
:petcapRemover pregtime n:  sets the pregnancy duration to n ticks. natural pregnancies are
                            300000 ticks for the current race and 200000 for everyone else
