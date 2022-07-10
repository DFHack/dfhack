strangemood
===========
Creates a strange mood job the same way the game itself normally does it.

Options:

:-force:        Ignore normal strange mood preconditions (no recent mood, minimum
                moodable population, artifact limit not reached).
:-unit:         Make the strange mood strike the selected unit instead of picking
                one randomly. Unit eligibility is still enforced.
:-type <T>:     Force the mood to be of a particular type instead of choosing randomly based on happiness.
                Valid values for T are "fey", "secretive", "possessed", "fell", and "macabre".
:-skill S:      Force the mood to use a specific skill instead of choosing the highest moodable skill.
                Valid values are "miner", "carpenter", "engraver", "mason", "tanner", "weaver",
                "clothier", "weaponsmith",  "armorsmith", "metalsmith", "gemcutter", "gemsetter",
                "woodcrafter", "stonecrafter", "metalcrafter", "glassmaker", "leatherworker",
                "bonecarver", "bowyer", and "mechanic".

Known limitations: if the selected unit is currently performing a job, the mood will not be started.
