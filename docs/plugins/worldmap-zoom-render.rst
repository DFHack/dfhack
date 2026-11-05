worldmap-zoom-render
====================

.. dfhack-tool::
    :summary: Provides native composition for high-level world-map zoom.
    :tags: dev
    :no-command:

Purpose and client responsibilities
-----------------------------------

The use case is a world-map inspection window: a player enlarges a small site,
road, or biome at 2x/4x, pans around, and still reads the native interface at its
normal size. The backend supports the high-level embark Biome map and the normal
fortress world map. The initial native build is enabled on Windows, where the
external mod has game-testing evidence.

DF draws geographic layers through ``graphic_map_portst`` and draws interface
text/controls through separate renderer methods. This service intercepts the
former and flushes its result before the latter. Changing DF's font size or
scaling a screenshot of the entire window would also enlarge the interface.

The C++ plugin owns SDL textures, renderer hooks, and their lifetime. A Lua
world-map client owns the modal screen, world-camera movement, zoom/pan controls,
and input routing. Loading this service alone creates no window and changes no
camera. The split lets a future DFHack build compile the native interface against
its own structures while Lua clients implement the interaction.

The complete reference client is
`gui/worldmap-zoom.lua <https://github.com/Huitzilopochtli-98/df-world-map-zoom/blob/v1.0.0/dfhack-config/scripts/gui/worldmap-zoom.lua>`__.
Its camera/input helpers, installers, and GUI are maintained in that repository;
they are not installed by this backend. The released client has its own fixed
version guards and needs adaptation to drive a development build.

Methodology
-----------

Coordinates and composition
~~~~~~~~~~~~~~~~~~~~~~~~~~~

A native world-map cell is 16 pixels, independent of the text-grid font. Let
``M`` be the map's native output rectangle and ``z`` be 2 or 4. The client supplies
crop offsets ``x,y`` in native map cells. The canvas clamps them to
``[0, columns*(1-1/z)]`` / ``[0, rows*(1-1/z)]``, rounds their pixel offsets,
and selects a source rectangle of size ``M.width/z`` by ``M.height/z``.
Copying that source to the original rectangle ``M`` enlarges only the map.

For example, a 32-by-24-cell map occupies 512 by 384 pixels. At 2x, offsets
``8,6`` select its central 256-by-192-pixel area and enlarge it back to 512 by
384 pixels. At 4x, the visible area is 128 by 96 pixels.

The texture extends from output origin ``0,0`` to the bottom-right of ``M``.
DF's original draw methods therefore keep their normal, positive coordinates.
A clip limits drawing to the selected crop. This intentionally uses a target
texture rather than a negative, scaled viewport: the latter produced a black
map on a tested Direct3D backend even though native hover still worked.

``MapCanvas::begin`` makes the drawing sequence explicit:

1. Validate geometry and create/reuse an opaque ARGB8888 target texture, with
   nearest-neighbour scaling to preserve the pixel art.
2. Save DF's render target, scale, viewport, clip, and draw colour.
3. Select the target texture at unit scale and an origin-zero viewport.
4. Clear a full rebuild or a new, invalid crop to opaque black. Preserve the
   texture for later incremental updates. Restore DF's draw colour for its draw.
5. Clip to the source crop and let DF's original map method draw normally.

``MapCanvas::finish`` restores all saved SDL state, verifies the restoration,
then copies the crop to ``M``. Renderer hooks call it before native UI drawing
and before presentation; the interface is drawn on top at its original scale.
A blank-screen fill discards a pending map pass instead of compositing data that
DF is about to erase. Resize, fullscreen changes, video reinitialization, and
SDL device-reset events invalidate the cached texture.

Full and incremental redraws
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DF compares old/new map cells and often calls ``update_map_port_tile`` for only
the cells that changed. Consecutive tile calls share a target pass; compositing
once at the next UI boundary avoids one texture copy per changed cell.
A changed cell is cleared before DF redraws it so removed sites/clouds do not
leave stale pixels.

After a camera change, unchanged cells may still be needed in a newly exposed
crop. The canvas tracks which crop cells DF has painted. If an incremental
batch is incomplete at the UI boundary, the adapter invokes one full native map
draw into the crop. A complete batch needs no extra redraw. A recursion-depth
guard routes nested native vmethod calls directly through DF while this fallback
draw is in progress.

Each camera publication has a generation. A pending pass whose generation no
longer matches the published camera is restored/discarded rather than displaying
an old crop for a new pointer mapping. The next native redraw uses the new view.

Input belongs to the client
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Magnifying pixels does not change DF's mouse hit testing. The client must claim
map clicks before enabling zoom, map hover coordinates back to the source crop,
and restore all temporary pointer/button fields after calling the parent.
The original inspection client blocks native map selection by default; opting
into selection requires the same inverse transform for the forwarded click.

In native map-local pixel coordinates, with map origin ``ox,oy`` and the clamped
crop offsets converted to pixels ``cx,cy``, inverse mapping is::

    native_x = ox + cx + (pointer_x - ox) / zoom
    native_y = oy + cy + (pointer_y - oy) / zoom

``snapshotUi``/``uiHit`` distinguish native text-grid interface cells from map
pixels. Capture the parent with a neutral pointer before drawing the client's
controls, so a temporary inspection label does not become a clickable control.
Forward UI input unchanged. Transform only map input. A stale UI snapshot returns
``nil``; the client must wait for a fresh snapshot before forwarding map input.

Concrete client sequence
~~~~~~~~~~~~~~~~~~~~~~~~

Within a modal world-map client that already owns input, these are the renderer
calls for a centered 2x fortress-map preview::

    dfhack.run_command('load', 'worldmap-zoom-render')
    local renderer = require('plugins.worldmap-zoom-render')

    local function start_preview()
        -- Recheck the native screen on every retry; the player may have left.
        local screen = dfhack.gui.getDFViewscreen(true)
        if not df.viewscreen_worldst:is_instance(screen) or
                screen.view_mode ~= df.world_view_mode_type.NORMAL then
            return
        end
        if not renderer.prepare() then
            dfhack.timeout(1, 'frames', start_preview)
            return
        end
        local port = df.global.gps.main_map_port
        renderer.setCamera(true, 2, 2, port.dim_x / 4, port.dim_y / 4)
    end

    local function close_preview()
        renderer.setCamera(false, 0, 1, 0, 0)
    end

Call ``snapshotUi`` after the parent render and before the client's widgets.
For each map hover, apply the inverse transform above around the parent's
logic/render calls. For panning beyond the current native map, the client first
updates the native screen's ``region_cent_x/y`` under the core lock, then
publishes crop offsets relative to that native camera. Recompute the crop after
resize, and close the client if its parent screen or supported mode changes.

Thread ownership
~~~~~~~~~~~~~~~~

The two-thread protocol is described in `core-suspender` and `g_src-notes`.
Viewscreens fill buffers on the simulation thread; native SDL renderer methods
consume the display buffers on the main/render thread. A ``CoreSuspender`` parks
simulation, **not rendering**, and must never be acquired by a renderer hook.

- Lua API calls and ``plugin_onupdate`` execute under the core lock. They inspect
  the native screen mode/map identity and publish a small camera snapshot.
- Renderer hooks read only that snapshot and their native display-side
  renderer/map-port arguments. They never follow a viewscreen pointer or call
  ``Gui::getDFViewscreen``. SDL objects and the canvas stay on this thread.
- The camera/error mutex protects short copies. The admission mutex serializes
  Lua entry points against stopping. Renderer hooks never acquire admission or
  the core lock, so teardown cannot create a render-to-Lua lock cycle.
- The SDL event watch touches atomics only. It records focus-loss generations
  (including loss/regain between client updates) and invalidates device targets.
  SDL2 dispatch/removal use the same event-watcher mutex; cleanup removes the
  watch outside dispatch, on the render thread, before acknowledging completion.

Clean unload
~~~~~~~~~~~~

All plugin render work uses the core-owned `render-thread-tasks` handles. The
completion fence covers both callback execution and destruction of its captures.
An empty core queue wrapper can remain queued after a cancelled task without
retaining a plugin function/manager. A callback merely setting a ``done`` atomic
at its last statement would not provide this guarantee.

Stopping closes admission, cancels pending work, and queues a render-thread
cleanup after any running work. Cleanup restores/releases the canvas, removes
the event watch and all renderer hooks, and requests a native full redraw.
Because removal occurs on the only thread that invokes those renderer methods,
no renderer hook can be in flight when that cleanup returns. ``status().stopped``
becomes true only after the core has destroyed the cleanup callback as well.
There is no Windows module pin and no platform-specific include in the backend.

Even ``stop()`` on a never-prepared service queues cleanup. The idle unload
check therefore verifies that there is no queued task or cleanup handle and
closes admission under the same mutex as Lua calls before granting unload.
The native title-screen regression stops an unprepared service, attempts unload,
waits for cleanup if needed, drains later frames with the DLL unloaded, and reloads.

An ordinary console ``unload worldmap-zoom-render`` owns no core lock and can
wait for this fence. The wait is bounded: if SDL is not progressing, unload is
refused while the module stays loaded. Retrying after a frame is safe.
An unload attempted from Lua/simulation while render cleanup is pending is refused rather
than waiting with the core lock held. A Lua owner can stop asynchronously::

    local function unload_when_stopped()
        if renderer.status().stopped then
            dfhack.run_command('unload', 'worldmap-zoom-render')
        else
            dfhack.timeout(1, 'frames', unload_when_stopped)
        end
    end
    renderer.stop()
    unload_when_stopped()

The client must close first and cancel any pending launch retries. ``stop``
ends the backend session; a new session requires reloading the plugin. Normal
client close uses ``setCamera(false, 0, 1, 0, 0)`` and leaves the service available.
During process shutdown, Core is already on the render thread after its SDL
loop and before native renderer destruction; cleanup runs inline, cancels
queued tasks, and waits for no future frame.

Build and validation boundary
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin checks runtime source/structure revisions against metadata linked
into its own build before enabling hooks. Rebuilding uses the current build's
metadata; it does not establish compatibility with changed renderer layouts.
The released mod has Windows in-game testing, including successful player use.
The refactored backend has also passed native title-screen API, cancellation,
physical DLL unload/reload, and prepared process-shutdown checks in an isolated
DF 53.16 instance. Full embark/fortress zoom-GUI acceptance remains a separate
test from this backend-lifecycle validation.

The canvas CTest exercises this production header through injected SDL calls,
including state restoration/failures, coverage, invalidation, and renderer
replacement. The task CTest compiles the core implementation separately and
tests cancellation, callback/capture destruction, a callback in flight, bounded
waits, and exception cleanup. Neither test claims a native DF/graphics-driver
acceptance result.

An optional desktop graphics check is provided in
``plugins/worldmap-zoom-canvas-sdl.test.cpp``. It runs the production canvas on
each available SDL renderer and checks every map pixel at 2x/4x against a native
1x baseline, plus unchanged interface pixels. The baseline matters: framebuffer
colour encoding can differ from the nominal draw colours on OpenGL ES 2. The
comparison stays exact; it does not hide a discrepancy with a colour tolerance.

For example, from ``build/manual-gpu`` in an x64 Visual Studio developer shell,
using the SDL2 headers/import library downloaded by DFHack's configuration::

    cl /std:c++20 /MD /EHsc /I"../../depends/SDL2/SDL2-2.26.2/include" ../../plugins/worldmap-zoom-canvas-sdl.test.cpp /Fe:canvas-sdl-test.exe /link "../../depends/SDL2/SDL2-2.26.2/lib/x64/SDL2.lib"

Place the game's ``SDL2.dll`` beside the executable and run
``canvas-sdl-test.exe --require-direct3d``. The windows are hidden; no DF instance
or save is loaded. A driver without a usable context is reported as skipped;
``--require-direct3d`` fails if the original Direct3D regression was not exercised.
``--driver opengles2`` restricts the check to one renderer. This desktop test is
optional because headless build agents may have no usable graphics device.

Lua API
-------

Load the service with::

    dfhack.run_command('load', 'worldmap-zoom-render')
    local renderer = require('plugins.worldmap-zoom-render')

The explicit load also handles reopening after an unload: Lua may still have a
cached module table whose native command bindings have been disconnected.

``renderer.prepare()``
    Verify the build, resolve the required SDL2 functions, and queue hook
    installation on the graphics thread. Returns ``false`` while installation
    is pending and ``true`` once ready. Call again on a later frame when it
    returns ``false``. Unsupported builds/renderers raise a Lua error. Check
    ``status().error`` if installation does not complete.

``renderer.stop()``
    Begin nonblocking teardown and return whether the completion fence is ready.
    Poll ``status().stopped`` on later frames. See the clean-unload sequence above.

``renderer.setCamera(enabled, context, zoom, x, y)``
    Set the camera for the active map. Context is ``1`` for embark or ``2`` for
    fortress; ``0`` is allowed when disabled. Zoom must be ``1``, ``2``, or ``4``.
    Coordinates are finite offsets from the top-left corner of the native map,
    in 16-pixel map cells. The visible crop is clamped to the native map bounds.
    The service must be ready and the context must match the current screen
    when enabling. Call ``setCamera(false, 0, 1, 0, 0)`` when the client closes.

``renderer.status()``
    Return a table with readiness and diagnostic information. ``ready`` reports
    hook installation, ``stopped`` reports the teardown fence, ``error`` contains a rendering failure, and ``context``
    identifies the current supported native screen (or ``0``). ``window_focused``
    and ``focus_epoch`` let a client cancel gestures after focus loss, even if
    focus was regained between updates. Output/desktop dimensions and counters
    for canvas creation, composition, and SDL-state restoration are also included.

``renderer.snapshotUi()``
    Snapshot occupied native text/interface cells after the parent screen renders
    and before drawing client controls. Use a neutral mouse position so temporary
    hover labels do not become clickable interface regions. Refresh after native
    interface changes or resize.

``renderer.uiHit(x, y)``
    Return whether a text-grid cell belongs to the captured native interface.
    Returns ``nil`` if the snapshot is missing or its dimensions are stale.
    Out-of-bounds cells return ``true``. Clients must block map actions when the
    result is ``nil`` and account for their own controls separately.

Diagnostic helpers
------------------

These functions require a prepared renderer. They are intended for manual
validation, not normal client interaction:

``renderer.capture(path)``
    Queue a PPM screenshot of the next rendered frame at a UTF-8 filesystem path.
    Read ``status().last_capture`` and ``status().capture_error`` for completion.

``renderer.profile(start)`` and ``renderer.frameProfile()``
    Start/reset bounded timing collection when ``start`` is true; return the map
    draw samples or complete frame samples, respectively, in microseconds.

``renderer.testWindow(width, height, fullscreen)``
    Queue a native window resize for testing. Fullscreen must be false; use DF's
    settings for fullscreen tests.

``renderer.testFocus(focused)``
    Queue an SDL focus event to test client gesture cancellation.

The original external client's snake_case function names remain as aliases for
the corresponding camelCase functions.
