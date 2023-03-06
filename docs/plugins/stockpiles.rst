.. _stocksettings:

stockpiles
==========

.. dfhack-tool::
    :summary: Import and export stockpile settings.
    :tags: fort design productivity stockpiles
    :no-command:

.. dfhack-command:: savestock
   :summary: Exports the configuration of the selected stockpile.

.. dfhack-command:: loadstock
   :summary: Imports configuration for the selected stockpile.

Select a stockpile in the UI first to use these commands.

Usage
-----

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

``savestock food``
    Export the stockpile settings for the currently selected stockpile to a
    file named ``food.dfstock``.
``loadstock food``
    Set the selected stockpile settings to those saved in the ``food.dfstock``
    file.
