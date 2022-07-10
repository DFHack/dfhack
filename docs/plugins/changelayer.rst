changelayer
===========
Changes material of the geology layer under cursor to the specified inorganic
RAW material. Can have impact on all surrounding regions, not only your embark!
By default changing stone to soil and vice versa is not allowed. By default
changes only the layer at the cursor position. Note that one layer can stretch
across lots of z levels. By default changes only the geology which is linked
to the biome under the cursor. That geology might be linked to other biomes
as well, though. Mineral veins and gem clusters will stay on the map. Use
`changevein` for them.

tl;dr: You will end up with changing quite big areas in one go, especially if
you use it in lower z levels. Use with care.

Options:

:all_biomes:       Change selected layer for all biomes on your map.
                   Result may be undesirable since the same layer can AND WILL
                   be on different z-levels for different biomes. Use the tool
                   'probe' to get an idea how layers and biomes are distributed
                   on your map.
:all_layers:       Change all layers on your map (only for the selected biome
                   unless 'all_biomes' is added).
                   Candy mountain, anyone? Will make your map quite boring,
                   but tidy.
:force:            Allow changing stone to soil and vice versa. !!THIS CAN HAVE
                   WEIRD EFFECTS, USE WITH CARE!!
                   Note that soil will not be magically replaced with stone.
                   You will, however, get a stone floor after digging so it
                   will allow the floor to be engraved.
                   Note that stone will not be magically replaced with soil.
                   You will, however, get a soil floor after digging so it
                   could be helpful for creating farm plots on maps with no
                   soil.
:verbose:          Give some details about what is being changed.
:trouble:          Give some advice about known problems.

Examples:

``changelayer GRANITE``
   Convert layer at cursor position into granite.
``changelayer SILTY_CLAY force``
   Convert layer at cursor position into clay even if it's stone.
``changelayer MARBLE all_biomes all_layers``
   Convert all layers of all biomes which are not soil into marble.

.. note::

    * If you use changelayer and nothing happens, try to pause/unpause the game
      for a while and try to move the cursor to another tile. Then try again.
      If that doesn't help try temporarily changing some other layer, undo your
      changes and try again for the layer you want to change. Saving
      and reloading your map might also help.
    * You should be fine if you only change single layers without the use
      of 'force'. Still it's advisable to save your game before messing with
      the map.
    * When you force changelayer to convert soil to stone you might experience
      weird stuff (flashing tiles, tiles changed all over place etc).
      Try reverting the changes manually or even better use an older savegame.
      You did save your game, right?
