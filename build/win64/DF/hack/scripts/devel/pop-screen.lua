-- Forcibly closes the current screen
--[====[

devel/pop-screen
================
Forcibly closes the current screen. This is usually equivalent to pressing
:kbd:`Esc` (``LEAVESCREEN``), but will bypass the screen's input handling. This is
intended primarily for development, if you have created a screen whose input
handling throws an error before it handles :kbd:`Esc` (or if you have forgotten
to handle :kbd:`Esc` entirely).

.. warning::

    If you run this script when the current screen does not have a parent,
    this will cause DF to exit **immediately**. These screens include:

    * The main fortress mode screen (``viewscreen_dwarfmodest``)
    * The main adventure mode screen (``viewscreen_dungeonmodest``)
    * The main legends mode screen (``viewscreen_legendsst``)
    * The title screen (``viewscreen_titlest``)

]====]

dfhack.screen.dismiss(dfhack.gui.getCurViewscreen())
