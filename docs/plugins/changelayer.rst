changelayer
===========

.. dfhack-tool::
    :summary: Change the material of an entire geology layer.
    :tags: fort armok map

Note that one layer can stretch across many z-levels, and changes to the geology
layer will affect all surrounding regions, not just your embark! Mineral veins
and gem clusters will not be affected. Use `changevein` if you want to modify
those.

tl;dr: You will end up with changing large areas in one go, especially if you
use it in lower z levels. Use this command with care!

Usage
-----

::

   changelayer <material RAW id> [<options>]

When run without options, ``changelayer`` will:

- only affect the geology layer at the current keyboard cursor position
- only affect the biome that covers the current keyboard cursor position
- not allow changing stone to soil and vice versa

You can use the `probe` command on various tiles around your map to find valid
material RAW ids and to get an idea how layers and biomes are distributed.

Examples
--------

``changelayer GRANITE``
   Convert the layer at the cursor position into granite.
``changelayer SAND_RED force``
   Convert the layer at the cursor position into red sand, even if it's
   currently stone.
``changelayer MARBLE all_biomes all_layers``
   Convert all layers of all biomes which are not soil into marble.

.. note::

    * If you use changelayer and nothing happens, try to pause/unpause the game
      for a while and move the cursor to another tile. Then try again. If that
      doesn't help, then try to temporarily change some other layer, undo your
      changes, and try again for the layer you want to change. Saving and
      reloading your map also sometimes helps.
    * You should be fine if you only change single layers without the use
      of 'force'. Still, it's advisable to save your game before messing with
      the map.
    * When you force changelayer to convert soil to stone, you might see some
      weird stuff (flashing tiles, tiles changed all over place etc). Try
      reverting the changes manually or even better use an older savegame. You
      did save your game, right?

Options
-------

``all_biomes``
   Change the corresponding geology layer for all biomes on your map. Be aware
   that the same geology layer can AND WILL be on different z-levels for
   different biomes.
``all_layers``
   Change all geology layers on your map (only for the selected biome unless
   ``all_biomes`` is also specified). Candy mountain, anyone? Will make your map
   quite boring, but tidy.
``force``
   Allow changing stone to soil and vice versa. **THIS CAN HAVE WEIRD EFFECTS,
   USE WITH CARE AND SAVE FIRST**. Note that soil will not be magically replaced
   with stone. You will, however, get a stone floor after digging, so it will
   allow the floor to be engraved. Similarly, stone will not be magically
   replaced with soil, but you will get a soil floor after digging, so it could
   be helpful for creating farm plots on maps with no soil.
``verbose``
   Output details about what is being changed.
