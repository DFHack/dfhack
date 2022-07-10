generated-creature-renamer
==========================
Automatically renames generated creatures, such as forgotten beasts, titans,
etc, to have raw token names that match the description given in-game.

The ``list-generated`` command can be used to list the token names of all
generated creatures in a given save, with an optional ``detailed`` argument
to show the accompanying description.

The ``save-generated-raws`` command will save a sample creature graphics file in
the Dwarf Fortress root directory, to use as a start for making a graphics set
for generated creatures using the new names that they get with this plugin.

The new names are saved with the save, and the plugin, when enabled, only runs once
per save, unless there's an update.
