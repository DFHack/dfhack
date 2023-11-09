mousequery
==========

.. dfhack-tool::
    :summary: Adds mouse controls to the DF interface.
    :tags: unavailable

Adds mouse controls to the DF interface. For example, with ``mousequery`` you
can click on buildings to configure them, hold the mouse button to draw dig
designations, or click and drag to move the map around.

Usage
-----

::

    enable mousequery
    mousequery [rbutton|track|edge|live] [enable|disable]
    mousequery drag [left|right|disable]
    mousequery delay [<ms>]

:rbutton:   When the right mouse button is clicked, cancel out of menus or
            scroll the main map if you r-click near an edge.
:track:     Move the cursor with the mouse instead of the cursor keys when you
            are in build or designation modes.
:edge:      Scroll the map when you move the cursor to a map edge. See ``delay``
            below. If enabled also enables ``track``.
:delay:     Set delay in milliseconds for map edge scrolling. Omit the amount to
            display the current setting.
:live:      Display information in the lower right corner of the screen about
            the items/building/tile under the cursor, even while unpaused.

Examples
--------

``mousequery rbutton enable``
    Enable using the right mouse button to cancel out of menus and scroll the
    map.
``mousequery delay 300``
    When run after ``mousequery edge enable``, sets the edge scrolling delay to
    300ms.
