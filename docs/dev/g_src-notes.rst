.. _g_src-notes:

#########################################
Notes on the Bay12 ``g_src`` sources
#########################################

This document describes the open-source portion of Dwarf Fortress that Bay12
ships in its Linux packages (``g_src``), and how that code maps onto the
symbols and types defined in `df-structures`_ (the ``df.g_src.*.xml`` files and
related globals such as ``enabler``, ``gps``, ``gview``, ``init``,
``musicsound``).

.. _df-structures: https://github.com/DFHack/df-structures

Bay12 releases this code so that Linux packagers can rebuild the parts of DF
that link against distribution-specific libraries (SDL2, OpenGL, curses, etc.).

The reference tree used here is the ``g_src`` folder from the Linux Classic build of DF 53.16
as available for `download from Bay12 <https://www.bay12games>`__. Line numbers are approximate.

.. contents:: Contents
   :local:

Source inventory and XML correspondence
=======================================

.. list-table::
   :header-rows: 1

   * - g_src file
     - Role
     - df-structures file
   * - ``enabler.h``, ``enabler.cpp``
     - ``enablerst`` — SDL main loop, frame scheduler, thread coordination,
       ``renderer`` base class, ``textures``, ``Either``, ``zoom_commands``,
       ``texture_fullid``
     - ``df.g_src.enabler.xml``
   * - ``enabler_input.h``, ``enabler_input.cpp``
     - ``enabler_inputst`` (base of ``enablerst``), key translation, repeat
       timing, macros
     - ``df.g_src.enabler.xml`` (parts), ``df.g_src.keybindings.xml``
   * - ``graphics.h``, ``graphics.cpp``
     - ``graphicst`` (``gps``), ``graphic_viewportst``,
       ``graphic_map_portst``, ``interface_setst``, ``texblitst``,
       screen/tile buffers
     - ``df.g_src.graphics.xml``
   * - ``renderer_2d.hpp``
     - ``renderer_2d_base``, ``renderer_2d`` — SDL_Renderer based tile
       renderer, texture cache
     - ``df.g_src.renderer_2d.xml``
   * - ``renderer_curses.cpp``
     - ``renderer_curses`` (PRINT_MODE:TEXT)
     - ``df.g_src.renderer_2d.xml``
   * - ``renderer_offscreen.cpp``
     - offscreen renderer for benchmarking
     - ``df.g_src.renderer_2d.xml``
   * - ``renderer_opengl.hpp``
     - legacy OpenGL renderer (not compiled — ``#include`` commented out in
       ``enabler.cpp:194``)
     - n/a
   * - ``interface.h``, ``interface.cpp``
     - ``interfacest`` (``gview``), scrolling/string-entry helpers
     - ``df.g_src.interface.xml``, ``df.interface.xml``
   * - ``ViewBase.h``, ``ViewBase.cpp``
     - ``viewscreenst``, ``abstract_interfacest``, ``widgets::*`` widget tree
     - ``df.g_src.ViewBase.xml``
   * - ``keybindings.h``, ``keybindings.cpp``
     - ``InterfaceKey`` enum, key binding names bimaps
     - ``df.g_src.keybindings.xml``
   * - ``KeybindingScreen.*``
     - keybinding-editing UI (viewscreen)
     - ``df.g_src.KeybindingScreen.xml``
   * - ``init.h``, ``init.cpp``
     - ``initst`` (``init``) — init.txt/colors.txt parsing, font texpos
       tables
     - ``df.g_src.init.xml``
   * - ``files.h``, ``files.cpp``, ``find_files.h``
     - ``filest``, ``stringvectst``, path handling, compressed file I/O
     - ``df.g_src.files.xml``, ``df.g_src.basics.xml``
   * - ``basics.h``, ``basics.cpp``
     - misc helpers, ``gamelog``/``errorlog``, ``flagarrayst``
     - ``df.g_src.basics.xml``
   * - ``texture_handler.h``, ``textures.cpp``
     - ``texture_handlerst``, ``palettest``, tile-page loading
     - ``df.g_src.texture_handler.xml``
   * - ``platform.h``, ``win32_compat.cpp``
     - Win32 API shims for Linux (``GetTickCount``, ``MessageBox``,
       ``DWORD``, ...)
     - ``df.g_src.platform.xml``
   * - ``mail.hpp``
     - ``Chan<T>`` channel, ``Either<L,R>``
     - modeled inline in ``df.g_src.enabler.xml``
   * - ``music_and_sound_g.h``, ``music_and_sound*.cpp``
     - ``musicsoundst`` (``musicsound``), ``Song``/``MusicEvent``/
       ``MusicContext`` enums
     - ``df.g_src.music_and_sound_g.xml``
   * - ``random.h``, ``random.cpp``
     - Mersenne Twister wrapper, ``trandom*``, seed stack
     - ``df.random_objects.xml``
   * - ``command_line.h``, ``command_line.cpp``
     - ``command_linest``
     - ``df.g_src.command_line.xml``
   * - ``lua.h``, ``lua.cpp``
     - ``LuaState`` RAII wrapper, DF Lua globals
     - not mapped (see "Other modules" below)
   * - ``textlines.h``
     - ``textlinesst`` raw-file line loader
     - ``df.g_src.basics.xml``
   * - ``svector.h``, ``bimap.h``, ``pool.h``, ``array_vec.hpp``,
       ``template.h``, ``endian.h``, ``resize++.h``
     - internal utility containers/algorithms
     - mostly not mapped (STL-internal)
   * - ``dfhooks.h``, ``dfhooks.cpp``
     - the DF ↔ DFHack plugin boundary
     - n/a — see "DFHack hook placement" below

Process architecture — the two threads
======================================

``enabler.cpp`` implements a two-thread design. Understanding which thread
runs what is the single most important thing about ``g_src`` for DFHack.

Boot sequence (``main``, ``enabler.cpp:1270``)
----------------------------------------------

1. ``SDL_Init(SDL_INIT_TIMER)`` on the **main thread**.
2. ``enabler.renderer_threadid = SDL_ThreadID()`` — the **main thread is the
   renderer/event thread** (``enabler.cpp:1282``).
3. ``SDL_CreateThread(call_loop, ...)`` spawns the **simulation thread**,
   which runs ``enablerst::async_loop()`` (``enabler.cpp:1265-1268``,
   ``746``).
4. ``init.begin()`` parses ``init_default.txt``, ``prefs/init.txt``,
   ``colors.txt`` into ``init``/``gps`` (``init.cpp:93``).
5. ``SDL_InitSubSystem(SDL_INIT_VIDEO)``, ``keybinding_init()``, keybindings
   loaded (``prefs/interface.txt`` then ``data/init/interface.txt`` — prefs
   first because ``load_keybindings`` does not overwrite).
6. ``enabler.loop(cmdline)`` → ``beginroutine()`` (closed source) → renderer
   creation (``renderer_2d`` unless PRINT_MODE:TEXT → ``renderer_curses``) →
   ``hooks_init()`` → ``eventLoop_SDL()`` (``enabler.cpp:1073-1125``).

So: the thread that runs ``main()`` owns SDL events, the SDL window/renderer,
and presentation. The created thread owns the simulation and the
screen-buffer writes.

The channels (``mail.hpp``, ``enabler.h:891-899``)
--------------------------------------------------

Three ``Chan<T>`` queues plus one semaphore, all members of ``enablerst``:

- ``async_tobox : Chan<async_cmd>`` — **main → sim** commands: ``pause``,
  ``start``, ``render``, ``inc`` (grant N sim frames), ``set_fps``.
- ``async_frombox : Chan<async_msg>`` — **sim → main** messages: ``quit``,
  ``complete``, ``set_fps``, ``set_gfps``, ``push_resize``, ``pop_resize``,
  ``reset_textures``, ``show_message``.
- ``async_zoom : Chan<zoom_commands>`` — **sim → main** zoom/fullscreen
  requests (``zoom_display()`` writes here; the event loop drains it).
- ``async_fromcomplete : std::binary_semaphore`` — released by the main
  thread after it has serviced an ``async_frombox`` request; callers block
  on it (see ``override_grid_size``, ``set_fps``, ``set_gfps``,
  ``show_message_box``).

``async_msg`` uses a ``union`` for payloads: ``fps``, ``{x,y}``, or
``{text, caption, type}`` — the wide-string pointers for ``show_message``
are raw borrowed pointers whose lifetime is guarded by the caller blocking
on ``async_fromcomplete`` until the main thread has shown the box
(``enabler.cpp:733-738``; ``enabler.h`` ``show_message_box``).

The sim thread — ``enablerst::async_loop`` (``enabler.cpp:746``)
----------------------------------------------------------------

The sim thread spins:

1. Drain ``async_tobox`` (blocking ``read`` when paused or out of frame
   budget, ``try_read`` otherwise):

   - ``pause`` → set ``async_paused``, ack with ``async_msg::complete``.
   - ``start`` → unpause, reset ``async_frames``.
   - ``render`` → if ``ENABLERFLAG_RENDER``: ``renderer->swap_arrays()``,
     ``render_things()`` (fills ``gps`` buffers), clear flag,
     ``update_gfps()``; then ack ``complete``. **This is the screen-render
     tick, on the sim thread.**
   - ``inc`` → add to ``async_frames`` budget (clamped to ``fps*3``).
   - ``set_fps`` → update local fps.

2. If unpaused and budget remains (or ``ENABLERFLAG_MAXFPS``): call
   ``mainloop()`` — the closed-source simulation + UI step — then
   ``simticks++``, ``clear_text_input()``.
3. ``hooks_update()`` **every iteration**, on the sim thread
   (``enabler.cpp:812``). This is where ``DFHack::Core::Update()`` runs.

If ``mainloop()`` returns true, it posts ``async_msg::quit`` and exits; the
main thread sees ``quit`` in ``async_wait``, sets ``loopvar=0``, and both
loops end.

The main thread — ``eventLoop_SDL`` + ``do_frame`` (``enabler.cpp:892``, ``816``)
---------------------------------------------------------------------------------

Each iteration:

1. Drain ``async_zoom``; if any zooms arrive, **pause the sim first**
   (``pause_async_loop`` writes ``pause`` to ``async_tobox`` and blocks in
   ``async_wait`` until ``complete``).
2. ``SDL_PollEvent`` loop — again, the first event triggers
   ``pause_async_loop``, so **all input processing happens while the sim
   thread is parked**. For each event: ``hooks_sdl_event(&event)`` (main
   thread, can consume the event entirely), then the switch: keys/wheel/
   quit → ``enabler.add_input`` (input state machine); buttons →
   ``enabler.mouse_*``; window events → resize/cursor-focus/
   ``force_full_display_count``/``ENABLERFLAG_RENDER``.
3. Mouse state → ``gps.precise_mouse_x/y``, ``gps.mouse_x/y``,
   ``enabler.tracking_on`` (written while sim is paused).
4. ``unpause_async_loop()``.
5. ``hooks_sdl_loop_fn()`` — main thread, just before ``do_frame``.
6. ``do_frame()``:

   - accumulate wall-clock time into ``outstanding_frames`` (sim) and
     ``outstanding_gframes`` (graphics), capped at 3 gframes;
   - ``inc`` command grants that many sim ticks;
   - each gframe: send ``render``, ``async_wait()`` for ``complete``
     (servicing ``set_fps``/``set_gfps``/``push_resize``/``pop_resize``/
     ``reset_textures``/``show_message`` while it waits), then
     ``gps.main_thread_requesting_reshape`` handling,
     ``tidy_tile_cache()``, ``display()``, ``render()`` →
     ``SDL_RenderPresent``.
   - ``emit_logs()`` when it sleeps.

7. ``musicsound.update()``.

What the pause protocol actually protects
-----------------------------------------

``pause_async_loop`` only stops ``mainloop()`` and ``render_things`` — it
does **not** lock ``gps``. The buffers are protected by *protocol*, not
mutexes:

- ``gps.screen``/``screentexpos*`` (the "front" buffers) are written by the
  sim thread inside the ``render`` command handler, *before* it posts
  ``complete``.
- ``renderer::screen``/``screentexpos*`` (the "back" buffers) are read by
  the main thread in ``display()``, *after* ``complete`` arrives.
- ``renderer::swap_arrays()`` (``enabler.cpp:513``) exchanges the
  front/back pointer sets, so the sim thread always draws into the buffer
  the renderer is *not* currently scanning. The swap runs on the sim
  thread, inside the ``render`` command, and completes before ``complete``
  is acked — so there is a happens-before edge via the channel.
- The per-viewport ``screentexpos_*_old`` arrays are likewise rotated in
  ``swap_arrays`` so ``display()`` can diff them against the fresh arrays
  for incremental redraws (``enabler.cpp:264-398``).

Things that are **not** safe by protocol (see "Problematic code" below):
``enabler.flag`` (``ENABLERFLAG_RENDER`` written by main, read by sim),
``enabler.async_paused`` (written by ``set_fps`` on whichever thread calls
it), ``simticks``/``gputicks``/``async_frames`` (plain ints read
cross-thread), ``gps.mouse_x``/``mouse_y`` (written by main while
"paused", read by sim at arbitrary times from viewscreen code — races are
possible because the sim thread resumes after ``unpause_async_loop``), and
input statics in ``enabler_input.cpp`` (``timeline``, ``pressed_keys``,
``modState``).

DFHack hook placement (``dfhooks.cpp``)
---------------------------------------

``g_src/dfhooks.cpp`` is a tiny trampoline: ``hooks_init()`` ``dlopen``\ s
``./libdfhooks.so`` (or ``dfhooks.dll``/``libdfhooks.dylib``) and resolves
``dfhooks_init``, ``dfhooks_shutdown``, ``dfhooks_update``,
``dfhooks_prerender``, ``dfhooks_sdl_event``, ``dfhooks_ncurses_key``,
``dfhooks_sdl_loop`` by name.

``libdfhooks`` itself is the **chainloader** (built from
``depends/dfhooks`` in the DFHack repo): it discovers ``dfhooks_*``/
``libdfhooks_*.so`` libraries, reads each one's optional
``dfhooks_priority`` int32 (lower runs earlier; DFHack exports 100), calls
each library's optional ``dfhooks_preinit`` at load time, and fans the
other callbacks out in priority order (shutdown in reverse). Note: despite
DFHack's comment in ``library/Hooks.cpp``, ``dfhooks_init``/
``dfhooks_preinit`` run *after* the sim thread is spawned —
``SDL_CreateThread`` precedes ``enabler.loop`` → ``hooks_init`` in
``main`` (``enabler.cpp:1289``, ``1119``).

.. list-table::
   :header-rows: 1

   * - g_src call site
     - Exported symbol
     - Runs on
     - DFHack endpoint (``library/Hooks.cpp``)
   * - ``enabler.loop``, after renderer creation, before event loop
     - ``dfhooks_init``
     - main
     - ``Core::InitMainThread`` + console/command-line checks
   * - after ``eventLoop_SDL`` exits
     - ``dfhooks_shutdown``
     - main
     - ``Core::Shutdown``
   * - every ``async_loop`` iteration
     - ``dfhooks_update``
     - **sim**
     - ``Core::Update`` (plugin ``onupdate``, ``Gui::*``, etc.)
   * - ``render_things``, before top viewscreen ``render()``
     - ``dfhooks_prerender``
     - **sim**
     - (currently a stub)
   * - every SDL event
     - ``dfhooks_sdl_event``
     - **main**
     - ``Core::DFH_SDL_Event`` → SDL events for tools
   * - every event-loop iteration, before ``do_frame``
     - ``dfhooks_sdl_loop``
     - **main**
     - ``Core::DFH_SDL_Loop`` (mouse/display side work)
   * - every ncurses key (PRINT_MODE:TEXT only)
     - ``dfhooks_ncurses_key``
     - main
     - ``Core::DFH_ncurses_key``

Practical consequences:

- Code in ``dfhooks_update`` runs on the sim thread, concurrent with the
  main thread's ``display()`` — it must not touch ``renderer.*`` buffers
  or SDL. ``CoreSuspender`` exists because even *this* does not stop the
  main thread from scanning buffers; it suspends via the DFHack-managed
  mutex.
- ``dfhooks_sdl_event``/``dfhooks_sdl_loop`` run while the sim thread is
  paused **only for the duration of the event drain** — ``unpause``
  happens before ``hooks_sdl_loop_fn``/``do_frame``, so
  ``dfhooks_sdl_loop`` runs concurrent with the sim thread. It is safe for
  SDL calls and for reading ``gps`` display-side state, not for mutating
  simulation data.
- A ``dfhooks_sdl_event`` handler that consumes an event stops DF's own
  processing of it entirely (the ``continue`` at ``enabler.cpp:932``).

``enablerst`` structure notes (``enabler.h:840``, ``df.g_src.enabler.xml``)
===========================================================================

``enablerst`` publicly inherits ``enabler_inputst`` — so
``enabler.add_input``, ``enabler.get_input``, keybinding and macro methods
all live on ``enabler``.

Field-by-field:

- ``renderer`` (``renderer*``) — the active renderer; ``renderer_2d`` in
  normal SDL builds. Virtual interface in ``enabler.h:733-852``; XML
  models all vmethods (``df.g_src.enabler.xml`` ``<class-type
  type-name='renderer'>``).
- Timing: ``fps``/``gfps`` are **floats** (target rates),
  ``outstanding_frames``/``outstanding_gframes`` are float accumulators
  drained by ``do_frame``. ``frame_timings``/``gframe_timings`` + sums
  implement the 50-frame rolling average behind
  ``calculate_fps``/``calculate_gfps`` (``enabler.cpp:1185-1213``).
- ``simticks``, ``gputicks`` — plain ``int``, incremented on sim and main
  threads respectively; also read by the *other* thread (e.g.
  ``Event::tick`` in input code) → benign-looking but technically racy.
- ``flag`` (``ENABLERFLAG_*``): ``RENDER`` = "a gframe is due; sim should
  run ``render_things``"; ``MAXFPS`` = uncapped sim; ``BASIC_TEXT`` = use
  the basic (non-antialiased) font texposes.
- Input: ``mouse_lbut``/``mouse_rbut``/``mouse_mbut`` (+``_down``/
  ``_lift`` edge flags), ``tracking_on`` (mouse inside a valid grid
  cell), ``listening_to_text``/``last_text_input`` (32-byte UTF-8
  text-input buffer, filled by ``SDL_TEXTINPUT`` on the main thread,
  cleared by sim thread each tick, read via ``get_text_input()`` by
  viewscreens that want text).
- ``clock`` — approx. ``SDL_GetTicks`` refreshed per ``do_frame``; cheap
  tick for GC-ish logic.
- ``command_line`` — the reconstructed command line string; the actual
  ``command_linest`` is ``game->command_line`` (``df.game_g.xml`` /
  ``df.g_src.command_line.xml``).
- ``fullscreen_state`` (``FullscreenState`` bitfield),
  ``overridden_grid_sizes`` (deque used by
  ``override_grid_size``/``release_grid_size``; note the FIXME at
  ``enabler.cpp:1157`` about unmatched release calls).
- ``last_message_result`` — ``std::atomic_int``; result of the last
  ``MessageBox`` call, set on the main thread in
  ``win32_compat.cpp:142``.
- ``must_do_render_things_before_display`` — when a viewport reshape is
  pending (``gps.main_thread_requesting_reshape``), ``render_things``
  leaves this true so ``do_frame`` performs the reshape on the main
  thread before the next ``display()`` (``enabler.cpp:849-858``,
  ``1253``).
- ``textures`` (``enabler_textures``/``textures`` class,
  ``enabler.h:557``) — raw ``SDL_Surface`` catalog backing ``texpos``
  values: ``raws`` vector, ``free_spaces``, ``init_texture_size``,
  ``uploaded`` flag. ``add_texture``, ``clone_texture``,
  ``grayscale_texture``, ``delete_all_post_init_textures`` etc. in
  ``textures.cpp``.
- Methods of interest: ``set_fps``/``set_gfps`` (thread-agnostic — they
  forward via ``async_frombox`` when called off the renderer thread),
  ``override_grid_size``/``release_grid_size``, ``zoom_display``,
  ``show_message_box``, ``clean_cached_tile``,
  ``get_current_interface_tile_dims``, ``main_renderer()``.

``graphicst`` and the tile pipeline (``graphics.h:5436``, ``df.g_src.graphics.xml``)
====================================================================================

``gps`` is the screen-state singleton. Everything a viewscreen draws lands
in its arrays.

The screen cell
---------------

- Indexing is **column-major**: cell ``(x, y)`` is at ``x*dimy + y``
  (confirmed throughout, e.g. ``addchar``, ``enabler.cpp:38``).
- ``screen`` is ``dimx*dimy*8`` bytes; 8 bytes per cell: ``s[0]`` =
  character byte; ``s[1..3]`` = foreground RGB; ``s[4..6]`` = background
  RGB; ``s[7]`` = type byte (``GRAPHICSTYPE_TTF``/``_TTFCONT`` —
  0xff/0xfe, currently vestigial since TTF paths are commented out).
- ``screenf``/``screenb``/``screenbright`` + ``use_old_16_colors`` are the
  "pen". ``changecolor(f,b,br)`` selects the 16-color palette
  (``uccolor[16][3]``); ``color3ub``/``bcolor3ub`` set true RGB
  (``screen_color_*`` fields). Both are written into the cell's RGB bytes.
- ``locate(y, x)`` sets the cursor ``screenx``/``screeny`` (note **y
  first**); ``addchar`` writes a cell then ``screenx += advance``.
  Clipping via ``setclipping``/``clipx``/``clipy``.

The texpos layer set
--------------------

For each cell there is a parallel set of ``long`` texpos arrays:

- ``screentexpos`` — the base graphical tile (overrides the char when
  ``INIT_DISPLAY_FLAG_USE_GRAPHICS``).
- ``screentexpos_lower`` — underlay (beneath text/tiles).
- ``screentexpos_anchored`` + ``_x``/``_y`` — multi-cell anchored tiles
  (creatures/buildings bigger than one tile). Subordinate cells get
  ``SCREENTEXPOS_FLAG_ANCHOR_SUBORDINATE`` plus packed 6-bit offsets
  (``ANCHOR_X_COORD>>6``, ``ANCHOR_Y_COORD>>12``, 64x64 limit,
  ``graphics.cpp:983-991``).
- ``screentexpos_flag`` per cell: ``GRAYSCALE``, ``ADDCOLOR``,
  ``ANCHOR_SUBORDINATE``, ``TOP_OF_TEXT``, ``BOTTOM_OF_TEXT``,
  ``ANCHOR_USE_SCREEN_COLOR`` (``graphics.h:2668-2677``).
- The same five arrays again for the **top layer** (``screen_top``,
  ``screentexpos_top*``), composited over the base layer when
  ``gps.top_in_use`` (e.g. announcements/tutorial overlays).
- Write order in ``update_all``: background → ``update_tile`` per cell →
  ``update_anchor_tile`` → top-layer ``update_top_tile``/
  ``update_top_anchor_tile``.

``texture_fullid`` (``enabler.h:646``/``671``) = ``{texpos, r,g,b,
br,bg,bb, flag}`` — the SDL renderer's cache key; ``flag`` is
``texture_fullid_flag`` (``TRANSPARENT_BACKGROUND``, ``DO_RECOLOR``,
``CONVERT``).

Viewports and map ports
-----------------------

- ``graphic_viewportst`` (``graphics.h:3319``) — a layered representation
  of the map portion of the screen: ~23 parallel ``int32_t`` arrays
  (``screentexpos``, ``_background``, ``_background_two``,
  ``_floor_flag``, ``_liquid_flag``, ``_spatter_flag``, ``_spatter``,
  ``_ramp_flag``, ``_shadow_flag``, ``_building_one``/``_two``,
  ``_top_shadow``, ``_item``, ``_vehicle``, ``_projectile``,
  ``_high_flow``, ``_vermin``, ``_signpost``, five ``*_creature``
  directions, ``_designation``, ``_interface``) plus a matching ``*_old``
  set for diffing. ``screen_x``/``screen_y`` are its private cursor;
  ``clipx``/``clipy`` clip writes; ``flag`` has
  ``GRAPHIC_VIEWPORT_FLAG_ACTIVE``.
- ``gps.main_viewport`` + ``gps.lower_viewport[LOWER_VIEWPORT_MAX]`` (=8)
  — the multilevel z-view: lower viewports draw beneath the main one.
  ``multilevel_depth_texpos[8]`` picks the depth shading tiles.
- ``graphic_map_portst`` (world-map viewports): base + 8 ``edge`` + 8
  ``edge2`` + detail/tunnel/river/road/site/army/interface +
  ``detail_to_*`` (8 dirs) + ``site_to_s`` + ``cloud_bits`` (u64 bitmask
  per cell) — plus ``_old`` twins, ``pixel_perc_x/y``,
  ``top_left_corner_*``.
- Resizing flows through ``reshape_viewports(zoom_factor)`` →
  ``shape_viewport_according_by_pixel_size`` (32px or
  ``zoom_factor*32/128`` per tile; map ports use 16px).

Per-frame lifecycle
-------------------

1. Main thread ``do_frame``: ``render`` cmd → sim thread.
2. Sim thread: ``swap_arrays()`` — ``gps.*`` ↔ ``renderer.*`` pointer
   exchange for every buffer, including each viewport's ``*_old`` arrays.
3. Sim thread: ``render_things()`` (``graphics.cpp:1199``):

   - walk ``gview.view`` ``child`` chain to topmost viewscreen;
   - ``hooks_prerender()``;
   - ``currentscreen->render(curtick)`` then
     ``currentscreen->widgets.render(curtick)`` — **widgets render last,
     deliberately "mostly DFHack"** (comment at line 1218). This is the
     ordering DFHack's overlay relies on.
   - REC/PLAY/prefix/FPS indicators.

4. Sim acks ``complete``; main thread ``async_wait`` returns.
5. Main thread ``display()`` (``enabler.cpp:218``): if
   ``force_full_display_count>0``, ``do_blank_screen_fill`` + full
   viewport/map_port rebuilds; else per-cell diff between
   ``screentexpos_*`` and ``*_old`` → ``update_viewport_tile``/
   ``update_map_port_tile`` only for changed cells. Then
   ``update_all()`` → ``render()`` → ``SDL_RenderPresent``.

The SDL renderer (``renderer_2d.hpp``)
--------------------------------------

``renderer_2d_base`` implements the ``renderer`` interface with an
``SDL_Renderer``: a ``texture_fullid → SDL_Texture*`` cache,
``update_tile`` / ``update_anchor_tile`` per-cell compositing (reading
``screen`` bytes + ``screentexpos`` + flags into a ``texture_fullid``,
looking it up in the cache, ``SDL_RenderCopy``), ``update_all`` (full
traversal), ``tidy_tile_cache`` (LRU-ish eviction), ``clean_tile_cache``
(full flush — triggered via ``gps.do_clean_tile_cache``), ``zoom``/
``resize``/``grid_resize``/``set_viewport_zoom_factor``/
``set_fullscreen``.

``screen_to_texid`` (``enabler.cpp:37``) is the canonical cell→texture
translation: char byte → font texpos (basic/large/small, or ``_top``/
``_bot`` for half-height text rows), or ``screentexpos`` +
GRAYSCALE/ADDCOLOR color mixing.

``init.font.*_font_texpos[256]`` (``init.h``, ``df.g_src.init.xml``) map
glyph bytes → font-page texposes, derived from ``curses_*.png`` font
sheets by ``prepare_graphics``.

Input pipeline (``enabler_input.cpp``, ``keybindings.h``)
=========================================================

SDL event → ``enabler.add_input`` (main thread, sim paused) →
``KeyEvent{release, EventMatch{type, mod, scancode, key|button|y}}`` →
``key_translation`` looks up ``EventMatch`` in the static ``keymap``
(``multimap<EventMatch,InterfaceKey>``) → ``Event``\ s go into the
``timeline`` (``set<Event>`` ordered by ``time`` then ``serial`` — one
physical press = one serial, possibly several ``InterfaceKey``\ s).

Sim thread consumes via ``enabler_inputst::get_input(now)`` →
``std::set<InterfaceKey>`` for one physical press; repeats are rescheduled
onto the timeline per ``repeatmap`` (``REPEAT_NOT``/``SLOW``/``FAST``,
tunable via ``init.input.hold_time``/``repeat_time``/``repeat_accel_*``).
``REPEAT_MWHEEL`` uses ``repeats`` as a countdown so one wheel notch
generates exactly N events.

``InterfaceKey`` (``keybindings.h``, ~1800 constants,
``df.g_src.keybindings.xml``) is the binding vocabulary;
``bindingNames``/``displayNames``/``sdlNames``/``sdlUnicode`` bimaps
translate between keys and human strings. Macros record
``set<InterfaceKey>`` lists (``prefs/macros/*.mak``), played back onto the
timeline.

``interface.h`` adds ``standardscrolling``/``secondaryscrolling``/
``standardstringentry`` helpers, ``INTERFACEFLAG_RETAIN_NONZERO_INPUT``,
and ``interfacest gview`` — the viewscreen stack root.

Interface layer (``ViewBase.h``, ``interface.h``)
=================================================

- ``viewscreenst`` (``ViewBase.h:1050``): ``child``/``parent`` links form
  the screen stack (``gview.view`` is the root *member*, not a pointer).
  ``breakdownlevel`` (``INTERFACE_BREAKDOWN_*``) requests teardown;
  ``feed(set<InterfaceKey>)``, ``logic()``, ``render(curtick)`` are the
  three virtuals the loop calls; ``set_port_flags()`` lets a screen claim
  viewports.
- ``interfacest::addscreen``/``removescreen`` splice the list
  (``INTERFACE_PUSH_*``); ``interfacest::loop`` (``interface.cpp:870``) is
  invoked from inside ``mainloop()`` and drives feed/logic/render per
  screen (inferred — ``mainloop`` itself is closed source).
- ``viewscreenst::widgets`` (a ``widgets::container``) is the per-screen
  widget tree — the DFHack overlay attaches widgets here. Container
  types: ``container``, ``widget_stack``, ``rows_container``,
  ``columns_container``, ``tabs``, ``scroll_rows``, ``radio_rows``,
  ``table``, ``folder``, ``filter``, ``multifilter``,
  ``params_container``; display widgets: ``text``, ``text_truncated``,
  ``text_multiline``, ``nineslice``, ``anchored_tile``,
  ``keybinding_display``, ``graphics_switcher``, ``character``; inputs:
  ``interface_*_button``, ``better_button``, ``textbox``, ``dropdown``.
  Anchoring is Godot-``Control``-style (``anchor_*`` fractions +
  ``offset_*`` + ``LayoutPreset``\ s).

Other modules DFHack touches
============================

- ``initst init`` — all init.txt state; ``init.display.grid_x``/
  ``grid_y`` are the *tile-grid* dimensions; ``init.display.flag`` has
  ``USE_GRAPHICS``, ``TEXT``, ``NOT_RESIZABLE``, ``SOFTWARE``;
  ``init.input.zoom_speed``, ``pause_zoom_no_interface_ms``.
- ``musicsoundst musicsound`` (``music_and_sound_g.h:422``) — song/
  ambience state machine (``Song``/``MusicEvent``/``MusicContext`` enums,
  ``MUSICSOUND_FLAG_*``), up to ``SOUND_CHANNELNUM`` =32 channels;
  ``update()`` is called from the main event loop; ``loading_files`` are
  async futures.
- ``command_linest`` — ``game->command_line.original``, ``arg_vect``,
  ``gen_id``/``world_seed``/``world_param``.
- ``random.h`` — ``trandom(limit)``,
  ``push_trandom_uniform_seed``/``pop_trandom_uniform_seed`` (seed stack),
  ``trandom_twist``.
- ``filest``/``stringvectst``/``textlinesst`` — path resolution across DF
  dirs (``canon_location``/``both_locations``/
  ``with_flags(FILE_FLAG_*)``), manual ``char*`` string vector, raw-file
  loader.
- ``LuaState`` (``lua.h``) — DF embeds Lua for the closed side (worldgen,
  utterances); ``add_lua_df_globals`` registers ``trandom``, ``lua_log``,
  ``capitalize_string_*``; ``data/init/globals.lua`` is run at state
  init. Not mapped in df-structures.
- ``platform.h``/``win32_compat.cpp`` — Win32 names on Linux:
  ``GetTickCount`` ≈ ``SDL_GetTicks``, ``MessageBox`` →
  ``SDL_ShowMessageBox`` (with cross-thread marshaling via
  ``show_message``), ``Sleep``, ``QueryPerformance*``, ``DWORD``,
  ``MB_*``, ``VK_*``. Values are locally defined and may differ from real
  Windows constants.

Problematic / outdated / unsafe code
====================================

Confirmed defects and latent hazards. Severity tags: **[bug]** real
defect; **[ub]** undefined/unspecified behavior risk; **[fragile]** works
today but easily broken; **[stale]** dead/outdated code.

Channels and threading
----------------------

- **[ub]** ``Chan::read`` waits ``cv.wait(lk)`` with no predicate; a
  spurious wakeup (or a wakeup for a message another consumer took) makes
  ``vals.front()``/``pop()`` run on an empty queue — UB
  (``mail.hpp:34-40``).
- **[ub]** ``Chan::try_read_for_millis`` treats *any* ``wait_for`` return
  of ``no_timeout`` as "queue non-empty" and pops unconditionally
  (``mail.hpp:23-33``). Also only one consumer can safely read a channel;
  ``try_read`` races multiple consumers by design of ``try_to_lock``.
- **[fragile]** ``Chan::write(const T &w)`` calls
  ``vals.push(std::move(w))`` — moving from a const ref copies; the API
  looks like it moves but doesn't (``mail.hpp:41-45``).
- **[fragile]** ``async_cmd``/``async_msg`` have a union + tag and their
  ctors don't initialize the union (``enabler.h:875-899``); a reader using
  the wrong member reads garbage. ``async_wait`` dispatches on ``msg`` so
  this is currently consistent — but ``async_msg(show_message)`` leaves
  ``fps``/``x``/``y`` uninitialized and vice versa.
- **[bug/ub]** ``enabler.flag``, ``enabler.async_paused``, ``simticks``,
  ``gputicks``, ``async_frames``, ``gps.mouse_*`` are non-atomic and
  cross-thread (e.g. ``flag`` write ``enabler.cpp:1005`` vs read ``784``;
  ``async_paused`` write ``enabler.cpp:1219`` vs read ``757``/``800``;
  ``simticks`` read on main in ``enabler_input.cpp:703``/``741``). Mostly
  benign on x86, formally racy.
- **[fragile]** ``async_msg::show_message`` ships raw ``const wchar_t*``
  cross-thread; lifetime is guarded only by the sender blocking on
  ``async_fromcomplete``. If anything drains ``async_frombox`` without
  servicing ``show_message``, or releases the semaphore early, UAF.
- **[fragile]** ``release_grid_size`` drains *all*
  ``overridden_grid_sizes`` then sends one ``zoom_resetgrid``; the
  in-source FIXME notes a known push/pop mismatch ("overriding once per
  movie but apparently only releasing for the last one",
  ``enabler.cpp:1157``).
- **[stale]** ``enabler_input.cpp:677`` calls ``enabler.simticks.load()``
  — but ``simticks`` is a plain ``int``. This is inside ``#ifdef CURSES``;
  the curses input path cannot compile as written. Indicates the curses
  port bit-rotted after ``simticks`` stopped being atomic.

Screen-buffer writes
--------------------

- **[bug]** ``addchar_flag(x, y, ...)`` and ``top_addchar_flag(x, y,
  ...)`` write ``screentexpos_flag[screenx*dimy + screeny]`` — the
  *cursor* cell, not the ``(x,y)`` argument cell (``graphics.h:11183``,
  ``11309``).
- **[bug]** ``addchar(x, y, c, f, b, bright)`` (and ``top_addchar``,
  ``addchar_flag``, ``top_addchar_flag`` ``(x,y)`` overloads) compute
  ``fg = f + (bright ? 8 : 0)`` and then ignore it, indexing
  ``uccolor[f]`` — the bright flag is dropped on these overloads
  (``graphics.h:11112-11118``).
- **[ub]** the ``(x,y)`` ``addchar``/``top_addchar``/``addchar_flag``/
  ``top_addchar_flag`` overloads index ``uccolor[f]``/``uccolor[b]`` with
  raw u8 parameters — any color index >15 is an out-of-bounds read
  (``graphics.h:11113-11118`` etc.).
- **[fragile]** ``locate``+``addchar`` bounds-safety relies on the clip
  rect: the pointer check ``s < screen_limit`` has no lower bound in the
  cursor-based overloads; a negative ``screenx`` passes the check and is
  only saved because ``clipx[0] >= 0``. Calling ``setclipping`` with
  negative values breaks that invariant.
- **[bug]** ``gps_locator::is_free`` reads ``gps.screen[x*dimy*8 +
  y*8]`` with no bounds check at all (``graphics.h:11433+``).
  Over/underflow reads if called with out-of-range coordinates.
- **[fragile]** ``graphicst::dim_colors``, ``rain/snow/color_square``
  operate on the 8-byte cell layout with magic offsets; several are only
  correct for the 16-color palette path.
- **[stale]** ``graphicst::gray_out_rect`` is fully commented out with a
  note that ``*4`` should be ``*8`` — the 4-byte cell era ended long ago
  (``graphics.h:11351-11367``). The TTF path in ``addst``/``top_addst``
  (``GRAPHICSTYPE_TTF``, ``ttf_manager``, ``ttfstr``) is similarly dead —
  commented out wholesale (``graphics.cpp:482-530``, ``562-610``).

Renderer
--------

- **[fragile]** ``renderer_2d_base::update_all`` uses hard-coded display
  numbers (1920/1080/907) and ``SDL_GetDisplayBounds(0, ...)`` on display
  0 for title-screen/background sizing (``renderer_2d.hpp``
  ``update_all``).
- **[fragile]** ``renderer_2d_base::zoom``: ``case zoom_reset:`` sets
  ``zoom_steps = 0`` then **falls through** into ``zoom_resetgrid`` →
  ``compute_forced_zoom()`` (``renderer_2d.hpp:4478-4481``). Probably
  intended — reset implies recompute — but there is no
  ``[[fallthrough]]`` or comment.
- **[bug]** ``renderer_2d_base`` zoom/reshape math has in-source FIXMEs:
  "perc introduces some error so at certain zooms it is way off (the -16
  is a cludge)" (``graphics.cpp:306-311``) — map-port origin math is
  approximate and known-wrong at some zoom levels.
- **[fragile]** ``clean_cached_tile``/``clean_tile_cache``/
  ``tidy_tile_cache`` call ``SDL_DestroyTexture`` on cache entries;
  entries can be null — tolerated by SDL2 but unchecked.
- **[stale]** ``renderer`` declares ``directtexcopy``/
  ``directtexcopy_old`` buffers that ``gps_allocate`` never allocates —
  dead fields still mapped in ``df.g_src.enabler.xml``
  (``enabler.h:750``/``772``).
- **[stale]** ``renderer_opengl.hpp`` is ``#include``\ d out
  (``enabler.cpp:194``) and ``loop()`` has a block of commented
  ``renderer_opengl``/``partial``/… selection logic
  (``enabler.cpp:1101-1116``); ``renderer_curses`` is ``#ifdef CURSES``
  -only. ``ttf_manager`` exists but its users are commented out.
- **[fragile]** ``renderer_2d`` ctor exits the process on init failure
  (window/renderer creation) rather than reporting.

Input
-----

- **[fragile]** ``add_input`` synthesizes release/re-press ``KeyEvent``\ s
  for all pressed keys on every modifier change
  (``enabler_input.cpp:537-555``) — quirky by design; DFH code that
  injects ``KeyEvent``\ s should use ``add_input_refined`` ("Use this one.
  It's much nicer.", ``enabler_input.h:96``).
- **[fragile]** ``load_macro_from_file`` uses fixed 100-char ``getline``
  buffers and hand-rolled parsing; malformed files print to cout and can
  partially populate ``macros`` (``enabler_input.cpp:968-1043``).
- **[bug-ish]** ``get_input``'s "3 simticks before first repeat" guard
  can drop a repeat event silently (``enabler_input.cpp:782-785``).
- **[stale]** ``GetKeyDisplay`` prints to ``cout`` on missing binding and
  returns "?" — fine for dev, useless in production.

Platform layer and misc
-----------------------

- **[bug]** ``win32_compat.cpp::MessageBox``: ``new char[64]``/
  ``new char[128]`` for title/text are **leaked on every call** — the
  heap allocation is unnecessary (``SDL_ShowMessageBox`` is synchronous
  and copies the strings; the comment claiming stack pointers are unsafe
  is wrong) (``win32_compat.cpp:125-129``). ``wcstombs`` also truncates
  at fixed sizes and loses non-ASCII.
- **[fragile]** ``platform.h`` redefines Win32 constants (``MB_*``,
  ``VK_*``, ``DWORD``, etc.) by hand; several values are explicitly noted
  not to match Windows. Code compiled against real Win32 values may
  behave differently.
- **[stale]** ``endian.h`` byteswaps only under ``__ppc__``/
  ``__ppc64__`` — dead on all modern targets.
- **[bug-ish]** ``stringvectst`` (``files.h``/``basics.h``) manually owns
  ``char*`` strings, erases from the front, and leaks if methods are used
  in unexpected order; several methods ignore allocation failure.
- **[fragile]** file/decompression helpers (``files.cpp``,
  ``basics.cpp``) read sizes and resize buffers from on-disk values
  without validation — malformed data can trigger huge allocations or
  overruns.
- **[fragile]** ``texture_handlerst``/``palettest``/``textures`` do
  manual ``new[]``/``delete[]`` and pointer bookkeeping; error paths leak
  or leave dangling pointers (e.g. ``clone_texture`` pushes a NULL
  surface into ``raws``, ``textures.cpp:59-71``).
- **[fragile]** ``lua.cpp``: ``lua_push_flags_member`` calls
  ``std::tolower`` on possibly-signed ``char`` (UB for negative chars);
  several raw ``lua_*`` stack/index manipulations assume fixed stack
  positions.
- **[stale]** ``enabler.h`` still carries OpenGL-era members/comments
  (``reset_textures`` semantics predate SDL_Renderer; "OpenGL state
  (wrappers)" comment on ``textures``), and ``enabler.cpp``/
  ``graphics.cpp`` have extensive commented-out TTF/OpenGL/partial-print
  code.
- **[fragile]** ``musicsoundst`` mixes raw ints/``DWORD``\ s,
  ``svector``\ s, and ``std::future``\ s; ``SOUND_CHANNELNUM``-indexed
  arrays in ``interfacest`` movie code are fixed at 200 entries with no
  bounds checks (``interface.h:115``).
- **[fragile]** ``emit_logs``/``errorlog_string``/``gamelog_string``
  open/close file streams per call plus keep static ``ofstream``\ s — the
  comment admits "opening/closing these too often seems to cause crashes
  on linux" (``basics.cpp:54-56``).

Quick map: g_src symbol → DFHack
=================================

.. list-table::
   :header-rows: 1

   * - g_src
     - df-structures / DFHack
   * - ``enabler`` (global ``enablerst``)
     - ``df.global.enabler`` (``df/enabler.h``)
   * - ``gps`` (``graphicst``)
     - ``df.global.gps`` (``df/graphic.h``)
   * - ``gview`` (``interfacest``)
     - ``df.global.gview`` (``df/interface.h``);
       ``Gui::getCurViewscreen``
   * - ``init`` (``initst``)
     - ``df.global.init``
   * - ``musicsound``
     - ``df.global.musicsound``
   * - ``mainloop()``
     - not mapped — closed side; reached via ``Core::Update`` inside
       ``dfhooks_update``
   * - ``hooks_*`` trampolines
     - ``dfhooks_*`` exports in ``library/Hooks.cpp``
   * - ``renderer`` vtable
     - ``df/renderer.h`` vmethods — interposable via ``VTableInterpose``;
       the old ``Renderer::renderer_wrap`` chaining code in
       ``library/modules/Renderer.cpp`` is entirely commented out
   * - ``InterfaceKey``
     - ``df.interface_key`` enum; ``Screen``/``Gui`` modules
   * - ``enabler.async_tobox``/``async_frombox``/``async_zoom``
     - modeled inline in ``df.g_src.enabler.xml`` (mutex+condvar+deque
       members)
   * - ``screentexpos*`` arrays
     - ``df.gps``/``df.renderer`` pointer fields;
       ``Screen::paintScreen``/``df.screen``
