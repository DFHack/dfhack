worldmap-zoom-render
====================

.. dfhack-tool::
    :summary: Provides native composition for high-level world-map zoom.
    :tags: dev
    :no-command:

This Windows-only plugin provides the rendering backend for 2x and 4x world-map
zoom. It draws the native geographic layers into an SDL2 target texture, then
composites a cropped portion beneath the interface. Text and controls retain
their normal size. Supported contexts are the high-level embark Biome map and
the normal fortress world map. Other map modes are left alone.

This is a Lua service, with no command or automatic activation. A client must
manage the camera, inverse mouse mapping, and controls. The external
`World Map Zoom project <https://github.com/Huitzilopochtli-98/df-world-map-zoom>`__
contains a reference client and validation evidence. Its released 1.0 client
has its own exact-version checks and needs adaptation for development builds;
this plugin does not replace those checks or install that client.

The plugin is built with DFHack's version metadata and generated structures.
Before accessing renderer fields, it checks that the running DFHack's source
and structure revisions match those linked into the plugin. A rebuild therefore
uses the new build's metadata instead of a hardcoded release hash. This does not
establish compatibility with changed renderer layouts or replace runtime testing.

Lua API
-------

Load the service with::

    local renderer = require('plugins.worldmap-zoom-render')

``renderer.prepare()``
    Verify the build, resolve the required SDL2 functions, and queue hook
    installation on the graphics thread. Returns ``false`` while installation
    is pending and ``true`` once ready. Call again on a later frame when it
    returns ``false``. Unsupported builds/renderers raise a Lua error. Check
    ``status().error`` if installation does not complete.

``renderer.setCamera(enabled, context, zoom, x, y)``
    Set the camera for the active map. Context is ``1`` for embark or ``2`` for
    fortress; ``0`` is allowed when disabled. Zoom must be ``1``, ``2``, or ``4``.
    Coordinates are finite offsets from the top-left corner of the native map,
    in 16-pixel map cells. The visible crop is clamped to the native map bounds.
    The service must be ready and the context must match the current screen
    when enabling. Call ``setCamera(false, 0, 1, 0, 0)`` when the client closes.

``renderer.status()``
    Return a table with readiness and diagnostic information. ``ready`` reports
    hook installation, ``error`` contains a rendering failure, and ``context``
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

Lifecycle and platform limitations
----------------------------------

Canvas creation, composition, SDL state restoration, resize invalidation, and
texture release run on the graphics thread. World/map unload disables the
camera. Plugin shutdown detaches hooks and removes the SDL event watch there.

The Windows module is pinned in memory until DF exits because queued graphics
callbacks and an already-fetched virtual call can outlive a logical unload.
Close DF before replacing the DLL. Linux is not built by this change: an
equivalent callback/module lifetime strategy and native validation are needed
before enabling that platform.
