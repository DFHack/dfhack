.. _digv:
.. _digtype:

dig
===

.. dfhack-tool::
    :summary: Provides commands for designating tiles for digging.
    :tags: fort design productivity map
    :no-command:

.. dfhack-command:: digv
    :summary: Designate all of the selected vein for digging.

.. dfhack-command:: digvx
    :summary: Dig a vein across z-levels, digging stairs as needed.

.. dfhack-command:: digl
    :summary: Dig all of the selected layer stone.

.. dfhack-command:: diglx
    :summary: Dig layer stone across z-levels, digging stairs as needed.

.. dfhack-command:: digcircle
    :summary: Designate circles.

.. dfhack-command:: digtype
    :summary: Designate all vein tiles of the same type as the selected tile.

.. dfhack-command:: digexp
    :summary: Designate dig patterns for exploratory mining.

This plugin provides commands to make complicated dig patterns easy.

Usage
-----

``digv [x] [-p<number>]``
    Designate all of the selected vein for digging.
``digvx [-p<number>]``
    Dig a vein across z-levels, digging stairs as needed. This is an alias for
    ``digv x``.
``digl [x] [undo] [-p<number>]``
    Dig all of the selected layer stone. If ``undo`` is specified, removes the
    designation instead (for if you accidentally set 50 levels at once).
``diglx [-p<number>]``
    Dig layer stone across z-levels, digging stairs as needed. This is an alias
    for ``digl x``.
``digcircle [<diameter>] [<solidity>] [<action>] [<designation>] [-p<number>]``
    Designate circles. The diameter is the number of tiles across the center of
    the circle that you want to dig. See the `digcircle`_ section below for
    options.
``digtype [<designation>] [-p<number>] [--zup|-u] [--zdown|-zu] [--cur-zlevel|-z] [--hidden|-h] [--no-auto|-a]``
    Designate all vein tiles of the same type as the selected tile. See the
    `digtype`_ section below for options.
``digexp [<pattern>] [<filter>] [-p<number>]``
    Designate dig patterns for exploratory mining. See the `digexp`_ section
    below for options.

All commands support specifying the priority of the dig designations with
``-p<number>``, where the number is from 1 to 7. If a priority is not specified,
the priority selected in-game is used as the default.

Examples
--------

``digcircle filled 3 -p2``
    Dig a filled circle with a diameter of 3 tiles at dig priority 2.
``digcircle``
    Do it again (previous parameters are reused).
``expdig diag5 hidden``
    Designate the diagonal 5 pattern over all hidden tiles on the current
    z-level.
``expdig ladder designated``
    Take existing designations on the current z-level and replace them with the
    ladder pattern.
``expdig``
    Do it again (previous parameters are reused).

digcircle
---------

The ``digcircle`` command can accept up to one option of each type below.

Solidity options:

``hollow``
    Designates hollow circles (default).
``filled``
    Designates filled circles.

Action options:

``set``
    Set designation (default).
``unset``
    Unset current designation.
``invert``
    Invert designations already present.

Designation options:

``dig``
    Normal digging designation (default).
``ramp``
    Dig ramps.
``ustair``
    Dig up staircases.
``dstair``
    Dig down staircases.
``xstair``
    Dig up/down staircases.
``chan``
    Dig channels.

After you have set the options, the command called with no options repeats with
the last selected parameters.

digtype
-------

For every tile on the map of the same vein type as the selected tile, this command
designates it to have the same designation as the selected tile. If the selected
tile has no designation, they will be dig designated. By default, only designates
visible tiles, and in the case of dig designation, applies automatic mining to them
(designates uncovered neighbouring tiles of the same type to be dug).

If an argument is given, the designation of the selected tile is ignored, and
all appropriate tiles are set to the specified designation.

Designation options:

``dig``
    Normal digging designation.
``channel``
    Dig channels.
``ramp``
    Dig ramps.
``updown``
    Dig up/down staircases.
``up``
    Dig up staircases.
``down``
    Dig down staircases.
``clear``
    Clear any designations.

Other options:

``-d``, ``--zdown``
    Only designates tiles on the cursor's z-level and below.
``-u``, ``--zup``
    Only designates tiles on the cursor's z-level and above.
``-z``, ``--cur-zlevel``
    Only designates tiles on the same z-level as the cursor.
``-h``, ``--hidden``
    Allows designation of hidden tiles, and picking a hidden tile as the target type.
``-a``, ``--no-auto``
    No automatic mining mode designation - useful if you want to avoid dwarves digging where you don't want them.

digexp
------

This command is for :wiki:`exploratory mining <Exploratory_mining>`.

There are two variables that can be set: pattern and filter.

Patterns:

``diag5``
    Diagonals separated by 5 tiles.
``diag5r``
    The diag5 pattern rotated 90 degrees.
``ladder``
    A 'ladder' pattern.
``ladderr``
    The ladder pattern rotated 90 degrees.
``cross``
    A cross, exactly in the middle of the map.
``clear``
    Just remove all dig designations.

Filters:

``hidden``
    Designate only hidden tiles of z-level (default)
``all``
    Designate the whole z-level.
``designated``
    Take current designation and apply the selected pattern to it.

After you have a pattern set, you can use ``expdig`` to apply it again.

Overlay
-------

This tool also provides three overlays that are managed by the `overlay`
framework.

asciidesignated
~~~~~~~~~~~~~~~

The ``dig.asciidesignated`` overlay makes designations visible in ASCII mode.
It highlights tiles that are designated for digging, smoothing, engraving,
track carving, or fortification carving. The designations blink (slowly) so you
can still see what is underneath them.

Due to the limitations of the ASCII mode screen buffer, the designation
highlights may show through other interface elements that overlap the
designated area.

warmdamptoolbar
~~~~~~~~~~~~~~~

The ``dig.warmdamptoolbar`` overlay adds a button to the toolbar at the bottom
of the screen when mining mode is active. It allows you to turn on warm or damp
dig mode. Tiles designated for digging while warm and/or damp dig mode is
enabled will be marked with a special symbol in graphics mode or color pattern
in ASCII mode. The digging designations for these tiles are protected from
cancellation due to warmth or dampness (respectively). This is very useful when
digging beneath a lake or just above a magma flow.

If you also have one of the vanilla autodig modes enabled, the warm/damp dig
marker will be propagated along with the autodig designation. This allows you
to, for example, easily autodig a mineral vein that goes through a light
aquifer.

If you have already designated a z-level when you realize you need warm or damp
dig protection (e.g. you have run into a light aquifer and want to continue
digging), the toolbar button gives you a shortcut to add the warm or damp dig
marker to all designated tiles on the current z-level. Note that it only
affects tiles that are not yet revealed since revealed tiles don't benefit from
the warm or damp dig designations.

Click on the new mining toolbar icon or hit :kbd:`Ctrl`:kbd:`D` to bring up the
configuration submenu.

warmdamp
~~~~~~~~

The ``dig.warmdamp`` overlay makes a number of tile properties visible when in
mining or smoothing mode:

- In ASCII mode, it highlights warm tiles red and damp tiles in light blue. Box
  selection characters and the keyboard cursor will also change color as
  appropriate when over a warm or damp tile. These can show through UI elements
  that happen to overlap the highlighted areas, just like the ``asciicarve``
  overlay.
- The tiles marked with warm and/or damp dig from the ``warmdamptoolbar`` get
  badges (in graphics mode) or distinctive color patterns (in ASCII mode)
  showing their status.
- In graphics mode, the "water drop" signifying a damp tile will no longer
  disappear when the tile is being box selected for applying a designation.
- Aquifer tiles are shown with icons distinct from "just damp" tiles. In
  graphics mode, light aquifer tiles have a "two drip" icon and heavy aquifer
  tiles have three drips. In ASCII mode, light aquifer tiles blink slowly in
  blue and heavy aquifer tiles blink faster in blue.
- The warm/damp/aquifer status will be shown for tiles that are visible from
  the bottom. For example, if you dig out the layer underneath an aquifer and
  notice dripping, entering mining mode and looking at the tiles above will
  show their status, even if the tiles are otherwise unrevealed. This feature
  was added with the rationale that if the dwarves can see the effects of the
  tiles above them, the player should be able to as well.
