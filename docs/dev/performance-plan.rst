=========================
Performance plan
=========================

Working plan for improving DFHack's per-tick CPU budget. Based on
microarchitecture analysis (Intel VTune capture ``r120ue``, Jan 2026)
and code review of the Lua integration. Not user-facing documentation.

.. contents::

Baseline findings
=================

From ``r120ue`` (uarch exploration, whole-system sampling; ~24k
``CPU_CLK_UNHALTED.THREAD`` samples):

- DFHack-side code is ~21% of sampled CPU: ``lua53.dll`` 3322,
  ``dfhooks_dfhack.dll`` 1750, plugins ~110.
- DFHack owns ~11% of ``STALLS_L3_MISS`` and ~11% of DTLB
  ``WALK_ACTIVE`` samples.
- Top lua53 stallers are data-layout problems, not dispatch:
  ``luaH_getshortstr`` (hash-chain walk, 17 stall samples),
  ``luaD_precall``, ``lua_geti``, ``luaH_next`` (``pairs`` iteration),
  ``lua_getmetatable``, ``mainposition``, ``luaC_checkfinalizer``.
- Top CPU consumers are the C<->Lua transition layer:
  ``luaV_execute`` 759, ``luaD_precall`` 260, ``index2addr`` 115,
  ``luaV_finishget`` (metamethod resolution) 90, ``luaG_traceexec``
  82 (the always-armed ``LUA_MASKCOUNT`` interrupt hook),
  ``match``/``singlematch`` ~88 (Lua pattern matching per frame),
  plus ``lua_tolstring``/``luaS_hash`` churn.
- C++ side: MSVC ``std::_Hash``/``std::_Tree`` internals,
  ``std::string`` construct/destroy churn, ``matchFocusString``,
  ``Units::isActive`` — chained-node STL containers have the same
  miss problem as Lua's tables.

Working hypothesis (operator model): DF's own memory traffic evicts
essentially all DFHack data from L3 between simulation ticks, so every
tick starts cold and each pointer-chase (hash chain, GC list,
metatable walk) pays full miss latency. Goal: make the per-tick cold
walk *sequential* (prefetcher-trackable) rather than pointer-chasing,
and reduce the number of misses by shortening dependent-load chains.

Overlay processing is believed to dominate the per-tick budget; this
is consistent with the data but not yet proven — see the attribution
step below.

Constraints and rejected directions
===================================

- **No PGO/LTO (deferred).** DFHack does questionable things to DF's
  vtables and binary; whole-program optimization changes codegen
  assumptions the interpose/binpatch machinery relies on. Would need
  a dedicated validation effort before it is safe to enable. Revisit
  only if cheaper items are exhausted.
- **Hugepages: optional-only.** ``VirtualAlloc(MEM_LARGE_PAGES)``
  needs ``SeLockMemoryPrivilege`` and often fails for normal users;
  Linux needs THP/hugetlbfs. Steam Deck (Linux) is a significant
  user population and we have no Linux profiling. Any allocator work
  must function with ordinary pages; hugepages may be an opt-in
  bonus path (``madvise(MADV_HUGEPAGE)`` on Linux,
  ``MEM_LARGE_PAGES`` on Windows) with mandatory fallback.
- **No LLVM-JIT Lua (Ravi etc.).** The miss profile is in
  ``ltable``/``lgc``/``ldo`` (data layout + transitions), not
  ``lvm`` dispatch. A JIT keeps identical object layout and would
  also have to reproduce our ``lua_lock`` threading patch,
  ``LUA_MASKCOUNT`` interrupt hooks, ``lstate.h`` field access,
  yieldable pcall, and debug-hook fidelity. Wrong tool for this
  problem.
- **LuaJIT** has the best-in-class data layout but is Lua 5.1
  semantics; incompatible with our 5.3 corpus.

Action items
============

Ordered by leverage-per-effort. All are architecture-independent
unless noted.

Measurement
-----------

1. Per-tick attribution. ``perf_counters.update_lua_ms`` /
   ``update_plugin_ms`` (``Core.cpp:1668-1682``) already isolate Lua
   update time; ``script-manager.lua`` prints them. Baseline a real
   fort first.
2. Callsite-stack attribution on VTune captures: ``dd_callsite``
   parent links in ``dicer.db`` allow rebuilding stacks and splitting
   Lua-side stalls between overlay widgets, timers, and plugin
   events. (Scratch query script: ``%TEMP%/vtune_agg.py``; accepts a
   ``dicer.db`` path for other captures.)
3. Lua-level: run ``profiler.lua`` scoped to the update window, or a
   count hook aggregating by Proto, to identify which script
   functions dominate ticks.
4. Object-level (if needed): we own the Lua source — log
   ``luaC_newobj``/``luaM_realloc_`` (addr, type, size, phase) during
   a session, dump live objects by walking ``allgc`` at a tick
   boundary, cross-reference VTune miss addresses.

Cheap knobs
-----------

5. Raise the interrupt-hook interval. ``interrupt_init`` arms
   ``LUA_MASKCOUNT, 256`` unconditionally (``LuaTools.cpp:508``) —
   ``luaG_traceexec`` shows 82 samples. Raising the count trades
   runaway-script interrupt latency for less hook dispatch. Tune and
   verify interrupt still fires acceptably.
6. GC tuning on the core state (``lua_gc`` pause/stepmul) — never
   configured today; only ``scripts/dwarf-op.lua`` calls
   ``collectgarbage``. One-line experiments; watch memory growth.
7. Audit per-frame Lua string work: ``match``/``singlematch``
   samples imply ``string.find``/patterns in a per-frame path;
   ``luaS_hash``/``lua_tolstring`` churn implies string building per
   frame. Fix at the call site once attribution identifies it.
8. Audit ``pairs()`` iteration over hash-part tables in hot loops
   (``luaH_next`` stalls): array-part iteration is contiguous and
   prefetchable; hash-part is not.

Structural: Lua heap layout
---------------------------

9. Custom ``lua_Alloc`` with type/lifetime-segregated arenas.
   - All GC objects funnel through ``luaC_newobj(L, tt, sz)`` — one
     place to tag by type; other allocs via ``luaM_realloc_`` by
     size class.
   - Permanent bump region for everything allocated during
     ``Lua::Open`` + ``require dfhack`` + script load (protos,
     interned strings, metatables, fieldtables, module tables) —
     init order approximates tick access order; GC's ``allgc`` walk
     becomes quasi-sequential. Phase flag can live in
     ``lua_extra_state`` (already used by ``dfhack_llimits.h``).
   - Separate churn region for post-init garbage.
   - Only 3 state-creation sites: ``Core.cpp:1394``,
     ``LuaTools.cpp:1793``, ``LuaTools.test.cpp``.
   - Portable: works with plain VirtualAlloc/mmap; hugepages are a
     strictly optional overlay on top (see constraints).
   - Caveat: ``lua_Alloc`` must implement realloc semantics
     (alloc+copy+free is fine); Lua GC never moves objects
     (``push_adhoc_pointer`` relies on this already).
10. Software prefetch at tick entry in ``Lua::Core::onUpdate``
    (``LuaTools.cpp:2073``): prefetch ``G(State)``, registry, and
    each due coroutine's ``stack``/``ci`` before ``lua_resume`` —
    portable via ``_mm_prefetch``/``__builtin_prefetch``. Cheap;
    converts serial cold misses into overlapped ones.

Binding layer (the userdata proxy question)
-------------------------------------------

11. Kill the double lookup in the field path.
    ``meta_struct_index`` → ``find_field`` → ``lookup_field``
    (``LuaTypes.cpp:422-454``) probes a Lua field-table *and* walks
    the enum metatable per access. Short strings are interned, so
    ``TString*`` pointer equality = name equality: a small
    direct-mapped cache keyed ``(fieldtable, TString*)``, or a
    per-type dense index assigned to each field name at metatable
    build time, collapses this to one load. Highest-confidence fix —
    hits ``luaV_finishget``/``luaH_getshortstr``/``lua_getmetatable``
    simultaneously.
12. Reduce per-access userdata churn. ``push_object_ref`` allocates
    a fresh userdata (+ ``object_ref_header``: tag_ptr, tag_identity,
    tag_attr, field_info) for every nested struct/container access —
    ``unit.pos.x`` garbage-per-hop. Options, in increasing scope:
    a) arena allocation makes the churn cheap and local (item 9);
    b) memoize refs via weak-valued cache keyed ``(ptr, identity)``
       — adds a hash probe, only worth it for expensive chains;
    c) bulk getters for hot access patterns (e.g. one call returning
       ``x,y,z`` for ``pos``) — semantic addition, avoids N
       allocations and N metamethod hops per vector;
    d) pack/shrink ``object_ref_header`` — review which of its four
       pointers are needed on the common path; union-tag fields could
       live in a side table populated only for unions.
13. Keep the proxy model; make the ref cheaper. A full move away
    from userdata proxies isn't practical (scripts rely on
    reference identity, ``__index`` polymorphism, ``_field``), but
    the identity-layer work in PR !5959 already removes one virtual
    dispatch from every primitive field read — that direction
    (more ``if constexpr`` static knowledge in the access path) is
    the right way to slim the proxy rather than replacing it.

C++-side containers
-------------------

14. MSVC STL ``unordered_map``/``unordered_set``/``map`` on per-tick
    paths are chained-node structures with the same miss signature
    (``_Fnv1a_append_value``, ``_Hash`` loops appear in the stall
    list — e.g. inside ``Units::isActive``). Swap hot-path instances
    for flat maps / sorted vectors / precomputed indices.
15. ``matchFocusString`` allocates ``std::string`` per call per
    frame — make it allocation-free (``string_view``, cached split).

Overlay / script layer
----------------------

16. Once attribution lands (item 2/3): batch the overlay render
    path. ``dfhack.penarray`` bulk tile ops already exist
    (``LuaApi.cpp:997+``). Per-cell ``Pen`` writes through
    metamethods per frame should move to bulk APIs or C++ rendering.
    If overlay is indeed the budget owner, this is likely the
    largest single user-visible win.

Verification methodology
========================

For each change: measure ``update_lua_ms`` / ``update_plugin_ms``
before/after on the same save; VTune Hotspots (not uarch exploration
— less noise) filtered to the sim thread; compare
``MEMORY_ACTIVITY.STALLS_L3_MISS`` and ``MEM_LOAD_RETIRED.L3_MISS``
inside ``lua53.dll``/``dfhooks_dfhack.dll``. Each item above is
independently revertable.

Lua 5.4/5.5 upgrade assessment
==============================

The ``__ipairs`` blocker is narrower than it appears:

- ``__ipairs`` is registered on all generated metatables
  (``LuaTypes.cpp`` ``SetPairsMethod`` x4; ``LuaWrapper.cpp``
  ``wtype_ipairs``/``complex_enum_ipairs``). It exists because some
  proxy ``__index`` implementations never return nil for integer
  keys (enum attrs — DFHack/dfhack#1860), so plain ``ipairs`` would
  not terminate.
- Under 5.4, ``__ipairs`` is ignored entirely; ``ipairs`` indexes
  ``t[i]`` until nil. Migration path: make every proxy ``__index``
  bounded for integer keys (containers already bound by item_count;
  enum attrs need an explicit bound — the enum count is known), then
  audit direct ``__ipairs`` users (one script:
  ``scripts/test/fix/stuck-written-materials.lua``; plus
  ``test/structures/enum_attrs.lua`` semantics).
- Real benefits for our profile: **generational GC** (young-gen
  collections over small dense sets directly attack the
  cold-sweep problem), ~10-25% faster VM, ``lua_newuserdatauv``
  uservalues (could fold ``object_ref_header`` into a uservalue —
  see item 12d).
- Costs: semantic audit of ~500 scripts (integer/string coercion
  strictness, ``math.random`` algorithm change breaks seeded
  determinism, ``lua_resume`` signature change — we call it
  directly in ``LuaTools.cpp:880``), bytecode format change
  (``dumper.lua`` ``string.dump`` roundtrips). Moderate project.
- Recommendation: pursue items 9-12 first (larger wins per risk);
  revisit 5.4 as a GC-locality lever afterward. 5.5 is not yet a
  stable target; track it but don't plan against it.

PR !5959 review (performance notes)
===================================

Net-positive for the hot path, with watch-items:

- **Win:** ``type_identity_for<T>::lua_read``/``lua_write`` inline
  ``*(T*)ptr`` via ``if constexpr`` — removes the second virtual
  call (old ``integer_identity_base::lua_read`` → virtual
  ``read()``) on every primitive field access. This is exactly the
  ``read_field``/``write_field`` hot path.
- **Watch:** ``mapped<T>`` containers (``std::map``/
  ``unordered_map``) now get identities with O(n) ``item_pointer``
  iteration. Any struct field of map type newly exposed to Lua
  (check the ``library/xml`` submodule bump) becomes O(n^2) under
  ``ipairs``-style iteration — audit which types gain this.
- **Watch:** ``container_storage<C<E*,A...>>`` generalizes the
  "vector<T*> == vector<void*>" layout assumption to *any*
  random-access pointer container (deque etc.). The assumption
  holds for same-width pointer instantiations in practice, but the
  blast radius widened.
- **Note:** removing ``BUILD_DFHACK_LIB`` guards lets plugins
  instantiate ``container_impl``/``type_identity_for`` locally,
  creating non-canonical identity objects — ``is_type_compatible``
  compares identity *pointers*, so a plugin-side local instance of
  an existing identity could yield false type mismatches. Document
  that identities must come from ``identity_traits``/canonical
  sources only.
- ``field_error``/``get_object_internal`` newly ``DFHACK_EXPORT`` —
  required by header-side template instantiation; fine.
- ``LuaTools.h`` now includes ``DataIdentity.h`` — heavier include
  in every consumer TU; compile-time only.
- ``bool`` moved to ``NUMBER_IDENTITY_TRAITS`` with correct
  ``isInteger()==false`` and bool-specific read/write — semantics
  preserved (``type()`` still ``IDTYPE_PRIMITIVE``, name "bool").
- ``c_string`` → ``primitive_identity_base`` preserves
  "raw pointer string" write error and push-string read semantics.
- ``push_adhoc_pointer`` retains the "GC never moves objects" hack —
  reinforces that any allocator work must stay non-moving.

Suggested order
===============

1. Callsite attribution (who owns the Lua stalls).
2. Cheap knobs: hook count, GC tuning, string-work audit.
3. Field-lookup inline cache (item 11).
4. Arena allocator (items 9-10).
5. C++ container swaps + ``matchFocusString`` (items 14-15).
6. Overlay batching per attribution (item 16).
7. Lua 5.4 evaluation (generational GC) once 1-6 land.
