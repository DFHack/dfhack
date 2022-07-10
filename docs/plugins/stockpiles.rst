.. _stocksettings:

stockpiles
==========
Offers the following commands to save and load stockpile settings.
See `gui/stockpiles` for an in-game interface.

:copystock:     Copies the parameters of the currently highlighted stockpile to the custom
                stockpile settings and switches to custom stockpile placement mode, effectively
                allowing you to copy/paste stockpiles easily.
                :dfhack-keybind:`copystock`

:savestock:     Saves the currently highlighted stockpile's settings to a file in your Dwarf
                Fortress folder. This file can be used to copy settings between game saves or
                players.  e.g.:  ``savestock food_settings.dfstock``

:loadstock:     Loads a saved stockpile settings file and applies it to the currently selected
                stockpile.  e.g.:  ``loadstock food_settings.dfstock``

To use savestock and loadstock, use the :kbd:`q` command to highlight a stockpile.
Then run savestock giving it a descriptive filename. Then, in a different (or
the same!) gameworld, you can highlight any stockpile with :kbd:`q` then execute the
``loadstock`` command passing it the name of that file. The settings will be
applied to that stockpile.

Note that files are relative to the DF folder, so put your files there or in a
subfolder for easy access. Filenames should not have spaces.  Generated materials,
divine metals, etc are not saved as they are different in every world.
