stonesense
==========

.. dfhack-tool::
    :summary: A 3D isometric visualizer.
    :tags: adventure fort graphics map

.. dfhack-command:: ssense
   :summary: An alias for stonesense.

Usage
-----

``stonesense`` or ``ssense``
    Open the visualiser in a new window.

The viewer window has read-only access to the game, and can follow the game view
or be moved independently. Configuration for Stonesense can be set in the
``dfhack-config/stonesense/init.txt`` file in your DF game directory.

.. figure:: ../images/stonesense-roadtruss.jpg
   :align: center
   :target: http://www.bay12forums.com/smf/index.php?topic=48172.msg3198664#msg3198664

   The above-ground part of the fortress *Roadtruss*.

Controls
--------
Mouse controls are hard-coded and cannot be changed.

:Left click:    Move debug cursor (if available)
:Right click:   Recenter screen
:Scrollwheel:   Move up and down
:Ctrl-Scroll:   Increase/decrease Z depth shown

Follow mode makes the Stonesense view follow the location and zoom level of the DF
window. The offset can be adjusted by holding :kbd:`Alt` while using the
keyboard window movement keys.

You can take screenshots with :kbd:`F5`, larger screenshots with
:kbd:`Ctrl`:kbd:`F5`, and screenshot the whole map at full resolution with
:kbd:`Ctrl`:kbd:`Shift`:kbd:`F5`.  Screenshots are saved to the DF directory.
Note that feedback is printed to the DFHack console, and you may need
to zoom out before taking very large screenshots.

See ``dfhack-config/stonesense/keybinds.txt`` to learn or set keybindings, including
zooming, changing the dimensions of the rendered area, toggling various
views, fog, and rotation. Here's the important section:

.. include:: ../../plugins/stonesense/configs/keybinds.txt
   :literal:
   :end-before: VALID ACTIONS:


Streaming Stonesense on Windows
-------------------------------
If you wish to stream Stonesense thru a broadcasting software such as `OBS <https://obsproject.com/>`_
then you may find that opening Stonesense causes your main DF window to flicker
between DF and Stonesense. While it is unknown exactly what causes this, a fix
does exist. Simply make sure that both DF and Stonesense are using ``Window Capture``
and NOT ``Game Capture``. This will stop the flickering from happening and enable
you to stream Stonesense for all to enjoy. This has been tested in OBS on Windows 10 but
should work on Windows 11 and in `Streamlabs <https://streamlabs.com/>`_. Linux, having no
``Game Capture`` option should be unaffected by this issue.

Known Issues
------------
If Stonesense gives an error saying that it can't load
:file:`creatures/large_256/*.png`, your video card cannot handle the high
detail sprites used. Either open :file:`creatures/init.txt` and remove the
line containing that folder, or :dffd:`use these smaller sprites <6096>`.

Sometimes if you have opened Stonesense and then resize the DF window, DF will appear to be
unresponsive. This bug is graphical only and if you hit :kbd:`Ctrl`:kbd:`Alt`:kbd:`S` and wait
a minute or so (since you can't see when the game finishes saving) the game should quicksave.

If you have Stonesense open in a fort and want to load a new fort, you MUST close Stonesense before
loading the new fort or the game will crash.

Stonesense requires working graphics acceleration, and we recommend
at least a dual core CPU to avoid slowing down your game of DF.

Yellow cubes and missing sprites
--------------------------------
If you are seeing yellow cubes in Stonesense, then there is something on the map that
Stonesense does not have a sprite for.

.. figure:: ../images/stonesense-yellowcubes.png
   :align: center

   An example of the yellow cubes.

If you would like to help us in fixing this, there are two things you can do:

* Make an issue on `GitHub <https://github.com/DFHack/stonesense/issues>`_ with what
  item is missing and pictures of what it looks like in DF.
* Create the art yourself. For help with this, please see the `stonesense-art-guide`.

Useful links
------------
- Report issues on `Github <https://github.com/DFHack/stonesense/issues>`_
- `support`
- `Stonesense Subreddit <https://www.reddit.com/r/stonesense/>`_
- :forums:`Official Stonesense thread <106497>`
- :forums:`Screenshots thread <48172>`
- :wiki:`Main wiki page <Utility:Stonesense>`
- :wiki:`How to add content <Utility:Stonesense/Adding_Content>`
- `Stonesense on Github <https://github.com/DFHack/stonesense>`_
