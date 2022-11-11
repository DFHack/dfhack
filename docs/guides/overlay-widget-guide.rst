.. _overlay-widget-guide:

.. highlight:: lua

DFHack overlay widget dev guide
===============================

This guide walks you through how to build overlay widgets and register them with
the `overlay` framework for injection into Dwarf Fortress viewscreens.

Why would I want to create an overlay widget?
---------------------------------------------

There are both C++ and Lua APIs for creating viewscreens and drawing to the
screen. If you need very specific low-level control, those APIs might be the
right choice for you. Here are some reasons you might want to implement an
overlay widget instead:

1. You can draw directly to an existing viewscreen instead of creating an
    entirely new screen on the viewscreen stack. This allows the original
    viewscreen to continue processing uninterrupted and keybindings bound to
    that viewscreen will continue to function. This was previously only
    achievable by C++ plugins.
1. Your widget will be listed along with other widgets, making it more
    discoverable for players who don't already have it enabled.
1. You don't have to manage the C++ interposing logic yourself and can focus on
    the business logic, writing purely in Lua if desired.
1. You get the state of whether your widget is enabled and its (configurable)
    position managed for you for free.

In general, if you are writing a plugin or script and have anything you'd like
to add to an existing screen (including overlaying map tiles), an overlay widget
is probably your easiest path to get it done. If your plugin or script doesn't
otherwise need to be enabled to function, using the overlay allows you to avoid
writing any of the enable management code that would normally be required for
you to show info in the UI.

What is an overlay widget?
--------------------------

Overlay widgets are Lua classes that inherit from ``overlay.OverlayWidget``
(which itself inherits from ``widgets.Widget``). The regular ``onInput(keys)``,
``onRenderFrame(dc, frame_rect)``, and ``onRenderBody(dc)`` functions work as
normal, and they are called when the viewscreen that the widget is associated
with does its usual input and render processing.

Overlay widgets can contain other Widgets, just like you're building a regular
UI element.

There are a few extra capabilities that overlay widgets have that take them
beyond your everyday ``Widget``:

bool = overlay_onupdate(viewscreen) if defined, will be called on every viewscreen logic() execution, but no more frequently than what is specified in the overlay_onupdate_max_freq_seconds class attribute. Widgets that need to update their state according to game changes can do it here. The viewscreen parameter is the viewscreen that this widget is attached to at the moment. For hotspot widgets, viewscreen will be nil. Returns whether overlay should subsequently call the widget's overlay_trigger() function.
screen = overlay_trigger() if defined, will be called when the overlay_onupdate callback returns true or when the player uses the CLI (or a keybinding calling the CLI) to trigger the widget. must return either nil or the Screen object that the widget code has allocated, shown, and now owns. Overlay widgets will receive no callbacks until the returned screen is dismissed. Unbound hotspot widgets must allocate a Screen if they want to react to the onInput() feed or be rendered. The widgets owned by the overlay must not be attached to that new screen, but the returned screen can instantiate and configure new views.
overlay_onupdate() will always get called for hotspots. Un-hotspotted widgets bound to particular viewscreens only get callbacks called when the relevant functions of the viewscreen are called (that is, the widget will be rendered when that viewscreen's render() function is run; the widget will get its onInput(keys) function called when the viewscreen's feed() function is run; the overlay_onupdate(viewscreen) function is called when that viewscren's logic() function is run).

How do I register a widget with the overlay framework?
------------------------------------------------------

Anywhere in your code after the widget classes are declared, define a table
named ``OVERLAY_WIDGETS``. The keys are the display names for your widgets and
the values are the widget classes. For example, the `dwarfmonitor` widgets are
declared like this::

    OVERLAY_WIDGETS = {
        cursor=CursorWidget,
        date=DateWidget,
        misery=MiseryWidget,
        weather=WeatherWidget,
    }

When the `overlay` plugin is enabled, it scans all plugins and scripts for
this table and registers the widgets on your behalf. The widget is enabled if it
was enabled the last time the `overlay` plugin was loaded and the widget's
position is restored according to the state saved in the
:file:`dfhack-config/overlay.json` file.

Widget example 1: adding text, hotkeys, or functionality to a DF screen
-----------------------------------------------------------------------


Widget example 2: highlighting artifacts on the live game map
-------------------------------------------------------------


Widget example 3: corner hotspot
--------------------------------


Here is a fully functional widget that displays a message on the screen::

local overlay = require('plugins.overlay')

MessageWidget = defclass(MessageWidget, overlay.OverlayWidget)
MessageWidget.ATTRS{
    default_pos={x=-16,y=4}, -- default position near the upper right corner
    viewscreens={'dungeonmode', 'dwarfmode'}, -- only display on main maps
}

function MessageWidget:init()
    self.message = ''

    self:addviews{widgets.}

    self:overlay_onupdate()
end

function MessageWidget:overlay_onupdate()
    -- getMessage() can be implemented elsewhere in the lua file or even
    -- in a host plugin (e.g. exported with DFHACK_PLUGIN_LUA_COMMANDS)
    local message = getMessage()

    self.frame.w = #message
    self.message = message
end

-- onRenderBody will be called whenever the associated viewscreen is
-- visible, even if it is not currently the top viewscreen
function MessageWidget:onRenderBody(dc)
    dc:string(self.message, COLOR_GREY)
end

function MessageWidget:onInput(keys)

end

-- register our widgets with the overlay
OVERLAY_WIDGETS = {
    cursor=CursorWidget,
    date=DateWidget,
    misery=MiseryWidget,
    weather=WeatherWidget,
}




Widget lifecycle
----------------

Overlay will instantiate and own the widgets. The instantiated widgets must not be added as subviews to any other View.



The overlay widget can modify self.frame.w and self.frame.h at any time (in init() or in any of the callbacks) to indicate a new size.


Widget state
------------
whether the widget is enabled
the screen position of the widget (relative to any edge)


Widget architecture
-------------------

bool = overlay_onupdate(viewscreen) if defined, will be called on every viewscreen logic() execution, but no more frequently than what is specified in the overlay_onupdate_max_freq_seconds class attribute. Widgets that need to update their state according to game changes can do it here. The viewscreen parameter is the viewscreen that this widget is attached to at the moment. For hotspot widgets, viewscreen will be nil. Returns whether overlay should subsequently call the widget's overlay_trigger() function.
screen = overlay_trigger() if defined, will be called when the overlay_onupdate callback returns true or when the player uses the CLI (or a keybinding calling the CLI) to trigger the widget. must return either nil or the Screen object that the widget code has allocated, shown, and now owns. Overlay widgets will receive no callbacks until the returned screen is dismissed. Unbound hotspot widgets must allocate a Screen if they want to react to the onInput() feed or be rendered. The widgets owned by the overlay must not be attached to that new screen, but the returned screen can instantiate and configure new views.
overlay_onupdate() will always get called for hotspots. Un-hotspotted widgets bound to particular viewscreens only get callbacks called when the relevant functions of the viewscreen are called (that is, the widget will be rendered when that viewscreen's render() function is run; the widget will get its onInput(keys) function called when the viewscreen's feed() function is run; the overlay_onupdate(viewscreen) function is called when that viewscren's logic() function is run).



Widget attributes
-----------------

Your widget must inherit from ``overlay.OverlayWidget``, which defines the
following class properties:

* ``name``
    This will be filled in with the display name of your widget, in case you
    have multiple widgets with the same implementation but different
    configurations.
* ``default_pos`` (default: ``{x=-2, y=-2}``)
    Override this attribute with your desired default widget position. See
    the `overlay` docs for information on what positive and negative numbers
    mean for the position.
* ``viewscreens`` (default: ``{}``)
    The list of viewscreens that this widget should be associated with. When
    one of these viewscreens is on top, your widget's callback functions for
    update, input, and render will be interposed into the viewscreen's call
    path.
* ``hotspot`` (default: ``false``)
    If set to ``true``, your widget's ``overlay_onupdate`` function will be
    called whenever the `overlay` plugin's ``plugin_onupdate()`` function is
    called (which corresponds to one call to the current top viewscreen's
    ``logic()`` function). This is in addition to any calls to
    ``overlay_onupdate`` initiated from associated interposed viewscreens.
* ``overlay_onupdate_max_freq_seconds`` (default: ``5``)
    This throttles how often a widget's ``overlay_onupdate`` function can be
    called. Set this to the largest amount of time (in seconds) that your
    widget can take to react to changes in information and not annoy the player.
    Set to 0 to be called at the maximum rate. Be aware that running more often
    than you really need to will impact game FPS, especially if your widget is
    bound to the main map screen.
