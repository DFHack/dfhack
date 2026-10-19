.. _core-suspender:

===============================
CoreSuspender and the core lock
===============================

This page describes the ``DFHack::CoreSuspender`` mechanism: what it does, how
it works internally, when you must use it, and when you must not. It is aimed
at developers working on the DFHack core library or C++ plugins. Lua-side
wrappers (``dfhack.with_suspend`` etc.) are covered in `lua-api`.

.. contents:: Contents
    :local:

What it is
----------

``CoreSuspender`` is an RAII guard that gives the calling thread *exclusive,
consistent access to DF's memory and to DFHack's shared Lua state*. While a
thread holds a ``CoreSuspender``, the Dwarf Fortress simulation thread is
parked at a well-defined point in its per-frame update and cannot mutate game
state. This is the only sanctioned way for a foreign thread (console input
thread, RPC server thread, plugin-created worker thread, etc.) to read or
write ``df::global::*`` data safely.

The name is historical and slightly misleading: DFHack no longer suspends the
process's threads with OS facilities. Instead, the simulation thread
cooperatively yields at a synchronization point once per frame, and the lock
serializes who may run while it is yielded.

Threading model
---------------

Three kinds of threads matter:

**The DF simulation thread**
    Runs the game logic. DFHack hooks it via ``dfhooks_update()``, which calls
    ``Core::Update()`` once per frame. During ``Core::InitSimulationThread()``
    this thread acquires ``Core::CoreSuspendMutex`` via a ``thread_local``
    ``CoreSuspenderBase`` (``MainThread::suspend()``) and effectively holds it
    for the rest of the session. This is what "the core is running" means: the
    simulation thread owns the lock while it executes.

**The DF render thread (a.k.a. "main thread" in the hooks layer)**
    Runs SDL event handling and screen drawing. Its id is recorded in
    ``Core::df_render_thread`` by ``InitMainThread()``. This thread **can
    never acquire the lock** — see `cannot-use` below. Note that it keeps
    running while a suspender is held: suspending the core does *not* stop
    rendering, it only parks simulation. The current implementation of the DF
    render thread will generally never read or write game state — it only renders
    the frame that was just computed by the simulation thread, using a double buffer
    mechanism, and then pauses until the simulation thread signals that a new
    frame is ready — so generally it's safe to access game state (other than certain
    fields in ``df::global::enabler`` which are used by the render thread) while it's running.

    The following hooks (exported from ``library/Hooks.cpp``) are invoked on
    this thread:

    - ``dfhooks_init()`` — once, before the simulation thread is started and
      the main event loop begins.
    - ``dfhooks_sdl_event(SDL_Event*)`` — for each SDL event; returning true
      consumes the event.
    - ``dfhooks_sdl_loop()`` — once per rendered frame, after mouse state is
      set in ``gps`` and just before the screen buffer is presented.
    - ``dfhooks_ncurses_key(int)`` — for each ncurses input character
      (non-SDL builds); returning true consumes the key. (This is a legacy
      event from when DF used ncurses, and is not used in SDL builds.)
    - ``dfhooks_shutdown()`` — once, after the main event loop exits.

    ``dfhooks_preinit()`` is called even earlier by the chainloader, before
    the main thread is initialized. ``dfhooks_update()`` and
    ``dfhooks_prerender()`` run on the simulation thread instead.

**Foreign threads**
    The DFHack console/IO thread (``fIOthread``/``fInitthread``), RPC server
    connection threads, and any threads spawned by plugins or tools. These are
    the threads ``CoreSuspender`` exists for.

How it works
------------

The state lives on ``Core`` (see ``library/include/Core.h``):

- ``CoreSuspendMutex`` — a ``std::recursive_timed_mutex``. The simulation
  thread is the standing owner; suspender threads contend on it.
- ``CoreWakeup`` — a ``std::condition_variable_any`` the simulation thread
  sleeps on while tools run.
- ``ownerThread`` — atomic ``std::thread::id`` of whoever currently owns the
  suspend context. Powers ``Core::isSuspended()``. Empty when the mutex is
  unlocked.
- ``toolCount`` — atomic count of threads that *want* the lock, incremented
  **before** blocking on the mutex. This ordering is the crux of the
  protocol: a tool that is still waiting on the mutex already counts, so the
  simulation thread can never observe "no work pending" while a tool is on
  its way in.

Once per frame, at the end of ``Core::Update()``, the simulation thread runs::

    CoreWakeup.wait(MainThread::suspend(),
                    [this] { return toolCount.load() == 0; });

``condition_variable_any::wait`` evaluates the predicate *before* releasing
the mutex, so when nothing is queued this is a single atomic load — the yield
has no measurable cost in the common case. If any tool has incremented
``toolCount``, the wait releases ``CoreSuspendMutex`` and parks the thread
until the count drains to zero.

A ``CoreSuspender`` therefore does:

- *Construct:* increment ``toolCount``, then ``lock()`` the recursive mutex.
  If the simulation thread is mid-frame, this blocks until the wait above
  releases the mutex. On successful acquisition, the thread stores itself in
  ``ownerThread``, remembering the previous owner.
- *Destruct:* restore the previous ``ownerThread``. If there was no previous
  owner (i.e. this was the outermost release), run ``Lua::Core::Reset()``,
  which verifies the shared ``lua_State`` stack is empty — every suspension
  gets a clean Lua context. Then release the mutex and decrement
  ``toolCount``; when it reaches zero, ``CoreWakeup`` is notified and the
  simulation thread resumes its frame loop.

Because the underlying mutex is recursive, nested ``CoreSuspender``
construction on the same thread is legal and cheap; only the outermost
release performs the owner restore, Lua reset, and wakeup.

Guarantees and non-guarantees
-----------------------------

What holding the lock guarantees:

- The simulation thread is not running game code. ``df::global::world``, the
  map, unit lists, viewscreen state, etc. are stable for the duration of the
  critical section.
- Only one suspender runs at a time — tools serialize on the mutex.
- Exclusive access to the shared core ``lua_State``. ``Core::getLuaState()``
  asserts ``isSuspended()``, and the ``DFHack::Lua::Core::*`` helpers assume
  it.
- All queued tools complete before the next simulation frame begins — the
  simulation thread waits for ``toolCount == 0``, not for a specific tool.

What it does *not* guarantee:

- The render thread is not suspended. It continues to draw and process SDL
  events while you hold the lock. Code running under the lock can still race
  with render-thread reads of DF memory (an accepted, deliberate asymmetry —
  the render thread never blocks, so those reads are best-effort).
- Timeliness. Acquisition can take up to a full frame; there is no priority
  or fairness among waiting tools beyond the mutex's own ordering.
- Safety during teardown. See `caveats`_.

``Core::isSuspended()`` returns true when the calling thread currently owns
the suspend context. Notably, it also returns true on the simulation thread
during normal execution (the standing owner) — so code like
``plugin_onupdate`` or viewscreen callbacks that run inside ``Core::Update``
may use the core Lua state directly, without constructing a suspender.

When the lock is required
-------------------------

You must hold the core lock when, from any thread that is not already in a
suspended context, you:

- Read or write ``df::global::*`` objects or anything reachable from them —
  game state, map blocks, units, buildings, screen/viewscreen internals.
- Touch the core ``lua_State`` — anything under ``DFHack::Lua::Core``, script
  invocation, ``Lua::Core::SafeCall``, etc.
- Perform any operation that would misbehave if the world mutated mid-read
  (iterating vectors of DF objects is the canonical example — a frame
  boundary can realloc or free them).

In practice, most entry points already arrange this for you:

- **Plugin commands** are invoked under a suspender by ``Plugin::invoke``
  unless the command is registered with ``unlocked = true``.
- **RPC methods** are invoked under a suspender unless the method is
  registered with ``SF_DONT_SUSPEND``.
- **Lua scripts** run under the lock; ``dfhack.with_suspend(f, ...)`` adds an
  explicit critical section if you need one inside script code.
- ``plugin_onupdate``, ``plugin_onstatechange``, EventManager handlers, and
  viewscreen ``feed``/``render`` callbacks execute on the simulation thread
  inside ``Core::Update`` and are implicitly in a suspended context. A
  ``CoreSuspender`` there is redundant but harmless (the mutex is recursive;
  ``EventManager::manageEvents`` does exactly this defensively).

So the lock is *your* responsibility mainly when you create your own thread,
register an ``unlocked`` command or ``SF_DONT_SUSPEND`` RPC method, or write
code reachable from foreign-thread callbacks that touches DF memory.

.. _cannot-use:

When the lock cannot be used
----------------------------

**On the render thread.** The ``CoreSuspenderBase`` constructor asserts
``df_render_thread != std::this_thread::get_id()``. This includes
``ConditionalCoreSuspender`` — you cannot even *attempt* acquisition there.
Beyond the assert, blocking the render thread on the mutex would stall frame
presentation and can deadlock against the parked simulation thread. This is
why hotkey guards and SDL event handling (which run on the render thread via
``dfhooks_sdl_event``) must inspect only native viewscreen state and must not
enter Lua — see the workaround in ``Gui.cpp``'s ``dfscreen_selection`` and
``Core::isRenderThread()``. To move work the other way, use
``DFHack::runOnRenderThread()``.

**Before the core is initialized.** The same constructor asserts
``df_render_thread`` is set, which only happens in ``InitMainThread()``. A
suspender cannot be constructed in static initializers or in code that runs
before ``dfhooks_init`` completes.

**Where blocking until the next frame boundary is unacceptable.** The mutex
is only released at the ``CoreWakeup.wait`` at the end of ``Core::Update``.
During long stretches where DF does not reach that hook (heavy save/load or
world-gen work, a frozen/backgrounded game), acquisition stalls arbitrarily.
If you only want to *opportunistically* take the lock, use
``ConditionalCoreSuspender``.

**While holding locks that suspended-context code also needs.** Classic
lock-ordering deadlock: the code comment in ``Plugin::unload`` exists because
holding the plugin ``access`` lock while the ``CoreSuspender`` destructs can
deadlock against the simulation thread's bookkeeping. Establish and follow a
lock order; when in doubt, release other locks before acquiring the suspend.

**While waiting on input or on another thread.** Holding the lock across
``lineedit``-style input waits, thread joins, or blocking IPC freezes the
simulation for the duration — and deadlocks outright if the thing you're
waiting on needs the simulation thread. ``Lua::RunCoreQueryLoop`` shows the
correct shape: it releases the suspend while waiting for input and
re-acquires it for each execution. On the Lua side, ``dfhack.lineedit`` may
yield the coroutine so the C++ side can drop the lock; wrapping such a call in
``dfhack.with_suspend`` forces it to block with the lock held instead.

``A note about vmethod interposes``
-----------------------------------

VMethod interposes normally run in the context of the simulation thread,
or alternatively in the context of another thread that has already acquired
a ``CoreSuspender``, so they are effectively already in a suspended context.
You should therefore not construct a ``CoreSuspender`` in the implementation
of a vmethod interpose, because there is a small chance that a vmethod interpose will
instead be run on one of DF's worker threads; attempting to acquire a lock in that
context is likely to cause a deadlock.

In the event that a vmethod interpose is called from a DF worker thread, we assume
that Bay12 has already implemented synchronization for the data that the
interposed method would have accessed. Mainly, this means that implementations
for interposed methods should avoid accessing data that is "not related" to the data
the method being interposed would normally access, as such data might not be
protected by synchronization. Reverse engineering may be required to determine
what accesses are safe from interposed methods. At the current time, we do not
know of any interposable methods that are called from DF worker threads, but this
does not mean there aren't any.

Note also that there are a handful of interposable methods that will be called by
DF from its render (main) thread. Acquiring a lock in these methods will also
likely cause a deadlock. Mainly, these are vmethods of the ``renderer`` class and
its subclasses. Exercise caution when writing interposes for these methods: they
must not attempt to acquire the core suspend lock or directly access DF game
data other than the renderer state itself.

``ConditionalCoreSuspender``
----------------------------

``ConditionalCoreSuspender`` attempts acquisition and is testable as a bool::

    DFHack::ConditionalCoreSuspender suspend;
    if (suspend) {
        // owns the lock
    } else {
        // lock not acquired; skip or degrade
    }

Despite its doc comment, acquisition is not strictly non-blocking: it uses
``try_lock_for(100ms)``, i.e. it will wait up to ~100 ms for the simulation
thread to reach the yield point. It fails only if the mutex is still
unavailable after that — typically because a very long frame is in progress
or another tool is hogging the lock. Use it for latency-sensitive paths where
a dropped update is better than a stall.

Note that ``toolCount`` is incremented before the attempt and decremented on
failure, so even a *failed* conditional suspender briefly makes the
simulation thread believe a tool is queued.

RPC batching
------------

``CoreService::CoreSuspend``/``CoreResume`` let a remote client pin the lock
across multiple RPC calls for atomic batch operations: ``CoreSuspend``
allocates a ``CoreSuspender`` on the connection thread and keeps it alive
until the matching ``CoreResume``. While a client holds it, the game is
frozen — this is a sharp tool; a misbehaving or disconnected client can stall
the session.

.. _caveats:

Caveats and pitfalls
--------------------

- **Suspension is observable.** Holding the lock stalls the simulation
  thread, so game time literally stops: long critical sections manifest as
  hitching or a frozen UI. Batch what you need, keep sections short, and
  never do I/O or sleeps inside the lock.

- **One yield point per frame.** ``CoreSuspender`` construction can block for
  up to a whole frame even when the game is running smoothly. Releasing and
  immediately re-acquiring in a loop can starve the simulation thread into
  running one frame per re-acquisition.

- **Frame-boundary visibility.** The yield happens at the *end* of
  ``Core::Update``, after ``doUpdate`` (state-change events, per-frame
  plugin/Lua handlers) has run. A tool's critical section therefore observes
  a world that is between frames: internally consistent, but already fully
  processed for the previous tick.

- **Lua state is reset on release.** The outermost unlock runs
  ``Lua::Core::Reset``, which clears the shared lua_State stack (logging
  "Common lua context stack top left at N after suspend" if it wasn't
  empty). Do not expect anything you left on the core Lua stack to survive
  the end of your critical section.

- **Shutdown is not synchronized.** When ``Core::Shutdown`` sets
  ``shutdown``, the next ``Core::Update`` releases the standing lock and the
  simulation thread stops waiting on ``toolCount``. A suspender acquired
  during or after teardown still acquires the mutex but the world is going
  away; core teardown code (e.g. ``Plugin::unload``) checks
  ``Core::isValid()``/``isMapLoaded()`` before entering the lock for this
  reason.

- **The render thread races with you.** If you mutate data that drawing code
  reads asynchronously, the render thread may observe intermediate states.
  Usually cosmetic, but know that "the game is frozen" is not literally true.
  Mainly, this applies to accessing objects reachable from ``df::global::enabler``,
  which is the control object for the renderer.

- **``MemAccess::isSuspended()`` is unrelated.** It's a legacy stub that
  always returns true; the real check is ``Core::isSuspended()``.

- **Stale advice.** The Lua API docs state "every thread is allowed only one
  suspend per DF frame" — a leftover from the pre-mutex implementation. Under
  the current mechanism repeated acquisition is legal; each one just risks a
  frame-length wait. Grouping operations into one critical section is still
  good advice for latency, not correctness.

Reference
---------

All of the following are in ``library/include/Core.h``:

``DFHack::CoreSuspender``
    RAII guard. Constructor blocks until the lock is acquired; destructor
    releases. Publicly, construction and destruction are the entire
    interface — ``lock``/``unlock``/``try_lock`` are deliberately protected.

``DFHack::ConditionalCoreSuspender``
    As above, but acquisition is attempted with a 100 ms bound and the
    object converts to ``bool`` to report success.

``Core::isSuspended()``
    True if the calling thread owns the suspend context.

``Core::isRenderThread()``
    True on the DF render thread, where the lock is forbidden.
