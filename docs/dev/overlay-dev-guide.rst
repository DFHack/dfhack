.. _overlay-dev-guide:

DFHack overlay dev guide
=========================

.. highlight:: lua

This guide walks you through how to build overlay widgets and register them with
the `overlay` framework for injection into Dwarf Fortress viewscreens.

Why would I want to create an overlay widget?
---------------------------------------------

There are both C++ and Lua APIs for creating viewscreens and drawing to the
screen. If you need very specific low-level control, those APIs might be the
right choice for you. However, here are some reasons you might want to implement
an overlay widget instead:

#. You can draw directly to an existing viewscreen instead of creating an
   entirely new screen on the viewscreen stack. This allows the original
   viewscreen to continue processing uninterrupted and keybindings bound to
   that viewscreen will continue to function. This was previously only
   achievable by C++ plugins.
#. You'll get a free UI for enabling/disabling your widget and repositioning it
   on the screen. Widget state is saved for you and is automatically restored
   when the game is restarted.
#. You don't have to manage the C++ interposing logic yourself and can focus on
   the business logic, writing purely in Lua if desired.

In general, if you are writing a plugin or script and have anything you'd like
to add to an existing screen (including live updates of map tiles while the game
is unpaused), an overlay widget is probably your easiest path to get it done. If
your plugin or script doesn't otherwise need to be enabled to function, using
the overlay allows you to avoid writing any of the enable or lifecycle
management code that would normally be required for you to show info in the UI.

Overlay widget API
------------------

Overlay widgets are Lua classes that inherit from ``overlay.OverlayWidget``
(which itself inherits from `widgets.Panel <panel>`). The regular
``onInput(keys)``, ``onRenderFrame(dc, frame_rect)``, and ``onRenderBody(dc)``
functions work as normal, and they are called when the viewscreen that the
widget is associated with does its usual input and render processing. The widget
gets first dibs on input processing. If a widget returns ``true`` from its
``onInput()`` function, the viewscreen will not receive the input.

Overlay widgets can contain other Widgets and be as simple or complex as you
need them to be, just like you're building a regular UI element.

There are a few extra capabilities that overlay widgets have that take them
beyond your everyday `widgets.Widget <widget>`:

- If an ``overlay_onupdate(viewscreen)`` function is defined, it will be called
    just after the associated viewscreen's ``logic()`` function is called (i.e.
    a "tick" or a (non-graphical) "frame"). For hotspot widgets, this function
    will also get called after the top viewscreen's ``logic()`` function is
    called, regardless of whether the widget is associated with that viewscreen.
    If this function returns ``true``, then the widget's ``overlay_trigger()``
    function is immediately called. Note that the ``viewscreen`` parameter will
    be ``nil`` for hotspot widgets that are not also associated with the current
    viewscreen.
- If an ``overlay_trigger()`` function is defined, will be called when the
    widget's ``overlay_onupdate`` callback returns true or when the player uses
    the CLI (or a keybinding calling the CLI) to trigger the widget. The
    function must return either ``nil`` or the ``gui.Screen`` object that the
    widget code has allocated, shown, and now owns. Hotspot widgets will receive
    no callbacks from unassociated viewscreens until the returned screen is
    dismissed. Unbound hotspot widgets **must** allocate a Screen with this
    function if they want to react to the ``onInput()`` feed or be rendered. The
    widgets owned by the overlay framework must not be attached to that new
    screen, but the returned screen can instantiate and configure any new views
    that it wants to. See the `hotkeys` DFHack logo widget for an example.

The ``overlay_trigger()`` command enables the activation of overlay widgets
via the command line interface (CLI) or keybindings.
For example, executing ``overlay trigger notes.map_notes add Kitchen``::

    function MyOverlayWidget:overlay_trigger(arg1, arg2)
        if arg1 == 'add' then
            -- Add a new note to the map
            self:addSomething(arg2)
        elseif arg1 == 'delete' then
            self:deleteSomething(arg2)
        end
    end

This allows for dynamic updates to the game's UI overlays directly from the CLI.

If the widget can take up a variable amount of space on the screen, and you want
the widget to adjust its position according to the size of its contents, you can
modify ``self.frame.w`` and ``self.frame.h`` at any time -- in ``init()`` or in
any of the callbacks -- to indicate a new size. The overlay framework will
detect the size change and adjust the widget position and layout.

If you don't need to dynamically resize, just set ``self.frame.w`` and
``self.frame.h`` once in ``init()`` (or just leave them at the defaults). If
you don't need to render a widget on the screen at all, set your frame width
and/or height to 0. Your ``render`` function will still be called, but no
repositioning frame will be shown for the overlay in `gui/overlay`.

Widget attributes
*****************

The ``overlay.OverlayWidget`` superclass defines the following class attributes:

- ``name``
    This will be filled in with the display name of your widget, in case you
    have multiple widgets with the same implementation but different
    configurations. You should not set this property yourself.
- ``version``
    You can set this to any string. If the version string of a loaded widget
    does not match the saved settings for that widget, then the configuration
    for the widget (position, enabled status) will be reset to defaults.
- ``desc``
    A short (<100 character) description of what the overlay does. This text
    will be displayed in `gui/control-panel` on the "Overlays" tab.
- ``default_pos`` (default: ``{x=-2, y=-2}``)
    Override this attribute with your desired default widget position. See
    the `overlay` docs for information on what positive and negative numbers
    mean for the position. Players can change the widget position at any time
    via the `overlay position <overlay>` command, so don't assume that your
    widget will always be at the default position.
- ``default_enabled`` (default: ``false``)
    Override this attribute if the overlay should be enabled by default if it
    does not already have a state stored in ``dfhack-config/overlay.json``.
- ``viewscreens`` (default: ``{}``)
    The list of viewscreens that this widget should be associated with. When
    one of these viewscreens is on top of the viewscreen stack, your widget's
    callback functions for update, input, and render will be interposed into the
    viewscreen's call path. The name of the viewscreen is the name of the DFHack
    class that represents the viewscreen, minus the ``viewscreen_`` prefix and
    ``st`` suffix. For example, the fort mode main map viewscreen would be
    ``dwarfmode`` and the adventure mode map viewscreen would be
    ``dungeonmode``. If there is only one viewscreen that this widget is
    associated with, it can be specified as a string instead of a list of
    strings with a single element. If you only want your widget to appear in
    certain contexts, you can specify a focus path, in the same syntax as the
    `keybinding` command. For example, ``dwarfmode/Info/CREATURES/CITIZEN`` will
    ensure the overlay widget is only displayed when the "Citizens" subtab under
    the "Units" panel is active.
- ``hotspot`` (default: ``false``)
    If set to ``true``, your widget's ``overlay_onupdate`` function will be
    called whenever the `overlay` plugin's ``plugin_onupdate()`` function is
    called (which corresponds to one call per call to the current top
    viewscreen's ``logic()`` function). This call to ``overlay_onupdate`` is in
    addition to any calls initiated from associated interposed viewscreens and
    will come after calls from associated viewscreens.
- ``fullscreen`` (default: ``false``)
    If set to ``true``, no widget frame will be drawn in `gui/overlay` for drag
    and drop repositioning. Overlay widgets that need their frame positioned
    relative to the screen and not just the scaled interface area should set
    this to ``true``.
- ``full_interface`` (default: ``false``)
    If set to ``true``, no widget frame will be drawn in `gui/overlay` for drag
    and drop repositioning. Overlay widgets that need access to the whole
    scaled interface area should set this to ``true``.
- ``overlay_onupdate_max_freq_seconds`` (default: ``5``)
    This throttles how often a widget's ``overlay_onupdate`` function can be
    called (from any source). Set this to the largest amount of time (in
    seconds) that your widget can take to react to changes in information and
    not annoy the player. Set to 0 to be called at the maximum rate. Be aware
    that running more often than you really need to will impact game FPS,
    especially if your widget can run while the game is unpaused. If you change
    the value of this attribute dynamically, it may not be noticed until the
    previous timeout expires. However, if you need a burst of high-frequency
    updates, set it to ``0`` and it will be noticed immediately.

Common widget attributes such as ``active`` and ``visible`` are also respected.
Note that those properties are checked *after* matching ``viewscreens`` focus
string(s), so you can assume they are evaluated in an consistent context. For
example, if your widget has ``viewscreens='dwarfmode/Trade/Default'``, then you
can assume your ``visible=function() ... end`` function will be executing while
the trade screen is active.

Registering a widget with the overlay framework
***********************************************

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
this table and registers the widgets on your behalf. Plugin lua code is loaded
with ``require()`` and script lua code is loaded with ``reqscript()``.
If your widget is in a script, ensure your script can be
`loaded as a module <reqscript>`, or else the widget will not be discoverable.
Whether the widget is enabled and the widget's position is restored according
to the state saved in the :file:`dfhack-config/overlay.json` file.

The overlay framework will instantiate widgets from the named classes and own
the resulting objects. The instantiated widgets must not be added as subviews to
any other View, including the Screen views that can be returned from the
``overlay_trigger()`` function.

Performance considerations
**************************

Overlays that do any processing or rendering during unpaused gameplay (that is,
nearly all of them) must be developed with performance in mind. DFHack has an
overall service level objective of no more than 10% performance impact during
unpaused gameplay with all overlays and background tools enabled. A single
overlay should seek to take up no more than a fraction of 1% of elapsed
gameplay time.

Please see the Core `performance-monitoring` section for details on how to get
a perf report while testing your overlay. The metric that you will be
interested in is the percentage of elapsed time that your overlay accounts for.

If you need to improve performance, here are some potential options:

1. Shard scanning over multiple passes. For example, instead of checking every
   item on the map in every update in your overlay, only check every Nth item
   and change the start offset every time you scan.

2. Reduce the frequency of state updates by moving calcuations to
   ``overlay_onupdate`` and setting the valud of the
   ``overlay_onupdate_max_freq_seconds`` attribute appropriately

3. Move hotspots into C++ code, either in a new core library function or in a
   dedicated plugin

Development workflows
---------------------

When you are developing an overlay widget, you will likely need to reload your
widget many times as you make changes. The process for this differs slightly
depending on whether your widget is attached to a plugin or is implemented in a
script.

Note that reloading a script does not clear its global environment. This is fine
if you are changing existing functions or adding new ones. If you remove a
global function or other variable from the source, though, it will stick around
in your script's global environment until you restart DF or run
`devel/clear-script-env`.

Scripts
*******

#. Edit the widget source
#. If the script is not in your `script-paths`, install your script (see the
   `modding-guide` for help setting up a dev environment so that you don't need
   to reinstall your scripts after every edit).
#. Call ``:lua require('plugins.overlay').rescan()`` to reload your overlay
   widget

Plugins
*******

#. Edit the widget source
#. Install the plugin so that the updated code is available in
   :file:`hack/lua/plugins/`
#. If you have changed the compiled plugin, `reload` it
#. If you have changed the lua code, run ``:lua reload('plugins.mypluginname')``
#. Call ``:lua require('plugins.overlay').rescan()`` to reload your overlay
   widget

Troubleshooting
---------------

You can check that your widget is getting discovered by the overlay framework
by running ``overlay list`` or by launching `gui/control-panel` and checking
the ``Overlays`` tab.

**If your widget is not listed, double check that:**

#. ``OVERLAY_WIDGETS`` is declared, is global (not ``local``), and references
   your widget class
#. (if a script) your script is `declared as a module <reqscript>`
   (``--@ module = true``) and it does not have side effects when loaded as a
   module (i.e. you check ``dfhack_flags.module`` and return before executing
   any statements if the value is ``true``)
#. your code does not have syntax errors -- run
   ``:lua ~reqscript('myscriptname')`` (if a script) or
   ``:lua ~require('plugins.mypluginname')`` (if a plugin) and make sure there
   are no errors and the global environment contains what you expect.

**If your widget is not running when you expect it to be running,** run
`gui/overlay` when on the target screen and check to see if your widget is
listed when showing overlays for the current screen. If it's not there, verify
that this screen is included in the ``viewscreens`` list in the widget class
attributes. Also, load `gui/control-panel` and make sure your widget is enabled.

Widget example 1: adding text to a DF screen
--------------------------------------------

This is a simple widget that displays a message at its position. The message
text is retrieved from the host script or plugin every ~20 seconds or when
the :kbd:`Alt`:kbd:`Z` hotkey is hit::

    local overlay = require('plugins.overlay')
    local widgets = require('gui.widgets')

    MessageWidget = defclass(MessageWidget, overlay.OverlayWidget)
    MessageWidget.ATTRS{
        desc='Sample widget that displays a message on the screen.',
        default_pos={x=5,y=-2},
        default_enabled=true,
        viewscreens={'dwarfmode', 'dungeonmode'},
        overlay_onupdate_max_freq_seconds=20,
    }

    function MessageWidget:init()
        self:addviews{
            widgets.Label{
                view_id='label',
                text='',
            },
        }
    end

    function MessageWidget:overlay_onupdate()
        local text = getImportantMessage() -- defined in the host script/plugin
        self.subviews.label:setText(text)
        self.frame.w = #text
    end

    function MessageWidget:onInput(keys)
        if keys.CUSTOM_ALT_Z then
            self:overlay_onupdate()
            return true
        end
        return MessageWidget.super.onInput(self, keys)
    end

    OVERLAY_WIDGETS = {message=MessageWidget}

Widget example 2: highlighting artifacts on the live game map
-------------------------------------------------------------

This widget is not rendered at its "position" at all, but instead monitors the
map and overlays information about where artifacts are located. Scanning for
which artifacts are visible on the map can slow, so that is only done every 10
seconds to avoid slowing down the entire game on every frame.

::

    local overlay = require('plugins.overlay')
    local widgets = require('gui.widgets')

    ArtifactRadarWidget = defclass(ArtifactRadarWidget, overlay.OverlayWidget)
    ArtifactRadarWidget.ATTRS{
        desc='Sample widget that highlights artifacts on the game map.',
        default_enabled=true,
        viewscreens={'dwarfmode', 'dungeonmode'},
        frame={w=0, h=0},
        overlay_onupdate_max_freq_seconds=10,
    }

    function ArtifactRadarWidget:overlay_onupdate()
        self.visible_artifacts_coords = getVisibleArtifactCoords()
    end

    function ArtifactRadarWidget:onRenderFrame()
        for _,pos in ipairs(self.visible_artifacts_coords) do
            -- highlight tile at given coordinates
        end
    end

    OVERLAY_WIDGETS = {radar=ArtifactRadarWidget}

Widget example 3: corner hotspot
--------------------------------

This hotspot reacts to mouseover events and launches a screen that can react to
input events. The hotspot area is a 2x2 block near the lower right corner of the
screen (by default, but the player can move it wherever).

::

    local overlay = require('plugins.overlay')
    local widgets = require('gui.widgets')

    HotspotMenuWidget = defclass(HotspotMenuWidget, overlay.OverlayWidget)
    HotspotMenuWidget.ATTRS{
        desc='Sample widget that reacts to mouse hover.',
        default_pos={x=-3,y=-3},
        default_enabled=true,
        frame={w=2, h=2},
        hotspot=true,
        viewscreens='dwarfmode',
        overlay_onupdate_max_freq_seconds=0, -- check for mouseover every tick
    }

    function HotspotMenuWidget:init()
        -- note this label only gets rendered on the associated viewscreen
        -- (dwarfmode), but the hotspot is active on all screens
        self:addviews{widgets.Label{text={'!!', NEWLINE, '!!'}}}
        self.mouseover = false
    end

    function HotspotMenuWidget:overlay_onupdate()
        local hasMouse = self:getMousePos()
        if hasMouse and not self.mouseover then -- only trigger on mouse entry
            self.mouseover = true
            return true
        end
        self.mouseover = hasMouse
    end

    function HotspotMenuWidget:overlay_trigger()
        return MenuScreen{hotspot_frame=self.frame}:show()
    end

    OVERLAY_WIDGETS = {menu=HotspotMenuWidget}

    MenuScreen = defclass(MenuScreen, gui.ZScreen)
    MenuScreen.ATTRS{
        focus_path='hotspot/menu',
        hotspot_frame=DEFAULT_NIL,
    }

    function MenuScreen:init()
        self.mouseover = false

        -- derive the menu frame from the hotspot frame so it
        -- can appear in a nearby location
        local frame = copyall(self.hotspot_frame)
        -- ...

        self:addviews{
            widgets.Window{
                frame=frame,
                autoarrange_subviews=true,
                subviews={
                    -- ...
                    },
                },
            },
        }
    end
