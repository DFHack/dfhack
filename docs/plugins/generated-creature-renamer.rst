generated-creature-renamer
==========================

.. dfhack-tool::
    :summary: Automatically renames generated creatures.
    :tags: unavailable
    :no-command:

.. dfhack-command:: list-generated
    :summary: List the token names of all generated creatures.

.. dfhack-command:: save-generated-raws
    :summary: Export a creature graphics file for modding.

Now, forgotten beasts, titans, necromancer experiments, etc. will have raw token
names that match the description given in-game instead of unreadable generated
strings.

Usage
-----

``enable generated-creature-renamer``
    Rename generated creatures when a world is loaded.
``list-generated [detailed]``
    List the token names of all generated creatures in the loaded save. If
    ``detailed`` is specified, then also show the accompanying description.
``save-generated-raws``
    Save a sample creature graphics file in the Dwarf Fortress root directory to
    use as a start for making a graphics set for generated creatures using the
    new names that they get with this plugin.

The new names are saved with the world.
