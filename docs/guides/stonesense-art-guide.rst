.. _stonesense-art-guide:

Stonesense art creation guide
=============================

Understanding isometric perspective
-----------------------------------

Stonesense uses an isometric perspective, a form of pseudo-3D projection where objects are displayed at an
angle, typically with a 2:1 pixel ratio for diagonal lines. This perspective allows for a detailed and visually
appealing representation of a 3D world using 2D sprites. Unlike traditional top-down views, isometric projection
simulates depth while maintaining a consistent scale without vanishing points.

Understanding sprites
---------------------

Understanding how Stonesense deals with sprites is central to anyone who wishes to modify the content. The
scheme is not very complicated, and this guide will give a short introduction to how they work. With the
exception of floors, which we will discuss later, all sprites are 32x32 pixels and come in groups known
as sprite sheets. All sprites are loaded and rendered in 32-bit full-color PNGs. The image files should have
a transparent background but pure magenta (RGB: 255,0,255) is also treated as transparent.


.. image:: ../images/stonesense-sprite-sample.png
    :align: left

Here's an example of a typical Stonesense sprite.

When working with Stonesense sprites, it is important to understand how they fit into the isometric grid.
Each sprite is designed to align with the isometric perspective and must fit within a specific bounding area.
To illustrate this, here is a template for the area that should be used by Stonesense sprites:

.. image:: ../images/stonesense-sprite-template.png
    :align: left

The solid area is the floor space taken up by a sprite, while the dotted box indicates the volume above this
area corresponding to one z-level.

The way sprites are loaded is fairly generalized: the name of the sprite sheet, and the index of a sprite within that sheet.

Sprite sheets
-------------
There can be an arbitrary number of sprite sheets for Stonesense, though there are 3 sheets that are
always present as they contain default sprites (see further down). Configuring the XML to use new sheets is
outside the scope of this guide but there may be a guide for such added in the future.

Sprite index
------------
The sprite index, or sheet index, is the zero-indexed offset of a sprite on its sprite sheet.
The index starts with the upper left sprite which has index zero. It then increments to the right. Stonesense
is hardcoded to 20 sprite-wide sheets, this means that anything past 20 "sprite slots" is ignored, though less
than 20 slots is fine. The first sprite on the second row always has index 20 (even if there are fewer sprites per row in the sheet), the next row is 40, and so on. This
boundary is hardcoded and changing the size of the sheet will not affect it.

This image shows how sprites are indexed. Grid added for readability.

.. figure:: ../images/stonesense-indexed-sprites.png
    :align: left


Important sprite sheets
-----------------------
`objects.png <https://github.com/DFHack/stonesense/blob/master/resources/objects.png>`_ is the default sheet
for buildings and vegetation. Also used for all hard-coded content, like default plants, the cursor, default
walls and liquid.

`creatures.png <https://github.com/DFHack/stonesense/blob/master/resources/creatures.png>`_ is the default
sprite sheet for creatures. If no file is specified in a creature node, this is the sheet it will use.

`floors.png <https://github.com/DFHack/stonesense/blob/master/resources/floors.png>`_ holds all the Stonesense
floors. Unlike the other sprite sheet, this sheet is hard-coded with sprite dimensions of 32x20 pixels.
