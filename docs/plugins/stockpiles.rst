.. _stocksettings:

stockpiles
==========

.. dfhack-tool::
    :summary: Import and export stockpile settings.
    :tags: untested fort design productivity stockpiles
    :no-command:

.. dfhack-command:: copystock
   :summary: Copies the configuration of the selected stockpile.

.. dfhack-command:: savestock
   :summary: Exports the configuration of the selected stockpile.

.. dfhack-command:: loadstock
   :summary: Imports the configuration of the selected stockpile.

When the plugin is enabled, the :kbd:`q` menu of each stockpile will have an
option for saving or loading the stockpile settings. See `gui/stockpiles` for
an in-game interface.

Usage
-----

``enable stockpiles``
    Add a hotkey that you can hit to easily save and load settings from
    stockpiles selected in :kbd:`q` mode.
``copystock``
    Copies the parameters of the currently highlighted stockpile to the custom
    stockpile settings and switches to custom stockpile placement mode,
    effectively allowing you to copy/paste stockpiles easily.
``savestock <filename>``
    Saves the currently highlighted stockpile's settings to a file in your
    Dwarf Fortress folder. This file can be used to copy settings between game
    saves or players.
``loadstock <filename>``
    Loads a saved stockpile settings file and applies it to the currently
    selected stockpile.

Filenames with spaces are not supported. Generated materials, divine metals,
etc. are not saved as they are different in every world.

Examples
--------

``savestock food_settings.dfstock``
    Export the stockpile settings for the stockpile currently selected in
    :kbd:`q` mode to a file named ``food_settings.dfstock``.
``loadstock food_settings.dfstock``
    Set the selected stockpile settings to those saved in the
    ``food_settings.dfstock`` file.
