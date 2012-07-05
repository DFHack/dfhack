##############
DFHack Lua API
##############

.. contents::

The current version of DFHack has extensive support for
the Lua scripting language, providing access to:

1. Raw data structures used by the game.
2. Many C++ functions for high-level access to these
   structures, and interaction with dfhack itself.
3. Some functions exported by C++ plugins.

Lua code can be used both for writing scripts, which
are treated by DFHack command line prompt almost as
native C++ commands, and invoked by plugins written in c++.

This document describes native API available to Lua in detail.
It does not describe all of the utility functions
implemented by Lua files located in hack/lua/...


=========================
DF data structure wrapper
=========================

DF structures described by the xml files in library/xml are exported
to lua code as a tree of objects and functions under the ``df`` global,
which broadly maps to the ``df`` namespace in C++.

**WARNING**: The wrapper provides almost raw access to the memory
of the game, so mistakes in manipulating objects are as likely to
crash the game as equivalent plain C++ code would be. E.g. NULL
pointer access is safely detected, but dangling pointers aren't.

Objects managed by the wrapper can be broadly classified into the following groups:

1. Typed object pointers (references).

   References represent objects in DF memory with a known type.

   In addition to fields and methods defined by the wrapped type,
   every reference has some built-in properties and methods.

2. Untyped pointers

   Represented as lightuserdata.

   In assignment to a pointer NULL can be represented either as
   ``nil``, or a NULL lightuserdata; reading a NULL pointer field
   returns ``nil``.

3. Named types

   Objects in the ``df`` tree that represent identity of struct, class,
   enum and bitfield types. They host nested named types, static
   methods, builtin properties & methods, and, for enums and bitfields,
   the bi-directional mapping between key names and values.

4. The ``global`` object

   ``df.global`` corresponds to the ``df::global`` namespace, and
   behaves as a mix between a named type and a reference, containing
   both nested types and fields corresponding to global symbols.

In addition to the ``global`` object and top-level types the ``df``
global also contains a few global builtin utility functions.

Typed object references
=======================

The underlying primitive lua object is userdata with a metatable.
Every structured field access produces a new userdata instance.

All typed objects have the following built-in features:

* ``ref1 == ref2``, ``tostring(ref)``

  References implement equality by type & pointer value, and string conversion.

* ``pairs(ref)``

  Returns an iterator for the sequence of actual C++ field names
  and values. Fields are enumerated in memory order. Methods and
  lua wrapper properties are not included in the iteration.

  **WARNING**: a few of the data structures (like ui_look_list)
  contain unions with pointers to different types with vtables.
  Using pairs on such structs is an almost sure way to crash with
  an access violation.

* ``ref._kind``

  Returns one of: ``primitive``, ``struct``, ``container``,
  or ``bitfield``, as appropriate for the referenced object.

* ``ref._type``

  Returns the named type object or a string that represents
  the referenced object type.

* ``ref:sizeof()``

  Returns *size, address*

* ``ref:new()``

  Allocates a new instance of the same type, and copies data
  from the current object.

* ``ref:delete()``

  Destroys the object with the C++ ``delete`` operator.
  If destructor is not available, returns *false*.

  **WARNING**: the lua reference object remains as a dangling
  pointer, like a raw C++ pointer would.

* ``ref:assign(object)``

  Assigns data from object to ref. Object must either be another
  ref of a compatible type, or a lua table; in the latter case
  special recursive assignment rules are applied.

* ``ref:_displace(index[,step])``

  Returns a new reference with the pointer adjusted by index*step.
  Step defaults to the natural object size.

Primitive references
--------------------

References of the *_kind* ``'primitive'`` are used for objects
that don't fit any of the other reference types. Such
references can only appear as a value of a pointer field,
or as a result of calling the ``_field()`` method.

They behave as structs with one field ``value`` of the right type.

To make working with numeric buffers easier, they also allow
numeric indices. Note that other than excluding negative values
no bound checking is performed, since buffer length is not available.
Index 0 is equivalent to the ``value`` field.


Struct references
-----------------

Struct references are used for class and struct objects.

They implement the following features:

* ``ref.field``, ``ref.field = value``

  Valid fields of the structure may be accessed by subscript.

  Primitive typed fields, i.e. numbers & strings, are converted
  to/from matching lua values. The value of a pointer is a reference
  to the target, or nil/NULL. Complex types are represented by
  a reference to the field within the structure; unless recursive
  lua table assignment is used, such fields can only be read.

  **NOTE:** In case of inheritance, *superclass* fields have precedence
  over the subclass, but fields shadowed in this way can still
  be accessed as ``ref['subclasstype.field']``.
  This shadowing order is necessary because vtable-based classes
  are automatically exposed in their exact type, and the reverse
  rule would make access to superclass fields unreliable.

* ``ref._field(field)``

  Returns a reference to a valid field. That is, unlike regular
  subscript, it returns a reference to the field within the structure
  even for primitive typed fields and pointers.

* ``ref:vmethod(args...)``

  Named virtual methods are also exposed, subject to the same
  shadowing rules.

* ``pairs(ref)``

  Enumerates all real fields (but not methods) in memory
  (= declaration) order.

Container references
--------------------

Containers represent vectors and arrays, possibly resizable.

A container field can associate an enum to the container
reference, which allows accessing elements using string keys
instead of numerical indices.

Implemented features:

* ``ref._enum``

  If the container has an associated enum, returns the matching
  named type object.

* ``#ref``

  Returns the *length* of the container.

* ``ref[index]``

  Accesses the container element, using either a *0-based* numerical
  index, or, if an enum is associated, a valid enum key string.

  Accessing an invalid index is an error, but some container types
  may return a default value, or auto-resize instead for convenience.
  Currently this relaxed mode is implemented by df-flagarray aka BitArray.

* ``ref._field(index)``

  Like with structs, returns a pointer to the array element, if possible.
  Flag and bit arrays cannot return such pointer, so it fails with an error.

* ``pairs(ref)``, ``ipairs(ref)``

  If the container has no associated enum, both behave identically,
  iterating over numerical indices in order. Otherwise, ipairs still
  uses numbers, while pairs tries to substitute enum keys whenever
  possible.

* ``ref:resize(new_size)``

  Resizes the container if supported, or fails with an error.

* ``ref:insert(index,item)``

  Inserts a new item at the specified index. To add at the end,
  use ``#ref``, or just ``'#'`` as index.

* ``ref:erase(index)``

  Removes the element at the given valid index.

Bitfield references
-------------------

Bitfields behave like special fixed-size containers.
Consider them to be something in between structs and
fixed-size vectors.

The ``_enum`` property points to the bitfield type.
Numerical indices correspond to the shift value,
and if a subfield occupies multiple bits, the
``ipairs`` order would have a gap.

Since currently there is no API to allocate a bitfield
object fully in GC-managed lua heap, consider using the
lua table assignment feature outlined below in order to
pass bitfield values to dfhack API functions that need
them, e.g. ``matinfo:matches{metal=true}``.


Named types
===========

Named types are exposed in the ``df`` tree with names identical
to the C++ version, except for the ``::`` vs ``.`` difference.

All types and the global object have the following features:

* ``type._kind``

  Evaluates to one of ``struct-type``, ``class-type``, ``enum-type``,
  ``bitfield-type`` or ``global``.

* ``type._identity``

  Contains a lightuserdata pointing to the underlying
  DFHack::type_instance object.

Types excluding the global object also support:

* ``type:sizeof()``

  Returns the size of an object of the type.

* ``type:new()``

  Creates a new instance of an object of the type.

* ``type:is_instance(object)``

  Returns true if object is same or subclass type, or a reference
  to an object of same or subclass type. It is permissible to pass
  nil, NULL or non-wrapper value as object; in this case the
  method returns nil.

In addition to this, enum and bitfield types contain a
bi-directional mapping between key strings and values, and
also map ``_first_item`` and ``_last_item`` to the min and
max values.

Struct and class types with instance-vector attribute in the
xml have a ``type.find(key)`` function that wraps the find
method provided in C++.

Global functions
================

The ``df`` table itself contains the following functions and values:

* ``NULL``, ``df.NULL``

  Contains the NULL lightuserdata.

* ``df.isnull(obj)``

  Evaluates to true if obj is nil or NULL; false otherwise.

* ``df.isvalid(obj[,allow_null])``

  For supported objects returns one of ``type``, ``voidptr``, ``ref``.

  If *allow_null* is true, and obj is nil or NULL, returns ``null``.

  Otherwise returns *nil*.

* ``df.sizeof(obj)``

  For types and refs identical to ``obj:sizeof()``.
  For lightuserdata returns *nil, address*

* ``df.new(obj)``, ``df.delete(obj)``, ``df.assign(obj, obj2)``

  Equivalent to using the matching methods of obj.

* ``df._displace(obj,index[,step])``

  For refs equivalent to the method, but also works with
  lightuserdata (step is mandatory then).

* ``df.is_instance(type,obj)``

  Equivalent to the method, but also allows a reference as proxy for its type.

* ``df.new(ptype[,count])``

  Allocate a new instance, or an array of built-in types.
  The ``ptype`` argument is a string from the following list:
  ``string``, ``int8_t``, ``uint8_t``, ``int16_t``, ``uint16_t``,
  ``int32_t``, ``uint32_t``, ``int64_t``, ``uint64_t``, ``bool``,
  ``float``, ``double``. All of these except ``string`` can be
  used with the count argument to allocate an array.

* ``df.reinterpret_cast(type,ptr)``

  Converts ptr to a ref of specified type. The type may be anything
  acceptable to ``df.is_instance``. Ptr may be *nil*, a ref,
  a lightuserdata, or a number.

  Returns *nil* if NULL, or a ref.


Recursive table assignment
==========================

Recursive assignment is invoked when a lua table is assigned
to a C++ object or field, i.e. one of:

* ``ref:assign{...}``
* ``ref.field = {...}``

The general mode of operation is that all fields of the table
are assigned to the fields of the target structure, roughly
emulating the following code::

    function rec_assign(ref,table)
        for key,value in pairs(table) do
            ref[key] = value
        end
    end

Since assigning a table to a field using = invokes the same
process, it is recursive.

There are however some variations to this process depending
on the type of the field being assigned to:

1. If the table contains an ``assign`` field, it is
   applied first, using the ``ref:assign(value)`` method.
   It is never assigned as a usual field.

2. When a table is assigned to a non-NULL pointer field
   using the ``ref.field = {...}`` syntax, it is applied
   to the target of the pointer instead.

   If the pointer is NULL, the table is checked for a ``new`` field:

   a. If it is *nil* or *false*, assignment fails with an error.

   b. If it is *true*, the pointer is initialized with a newly
      allocated object of the declared target type of the pointer.

   c. Otherwise, ``table.new`` must be a named type, or an
      object of a type compatible with the pointer. The pointer
      is initialized with the result of calling ``table.new:new()``.

   After this auto-vivification process, assignment proceeds
   as if the pointer wasn't NULL.

   Obviously, the ``new`` field inside the table is always skipped
   during the actual per-field assignment processing.

3. If the target of the assignment is a container, a separate
   rule set is used:

   a. If the table contains neither ``assign`` nor ``resize``
      fields, it is interpreted as an ordinary *1-based* lua
      array. The container is resized to the #-size of the
      table, and elements are assigned in numeric order::

        ref:resize(#table);
        for i=1,#table do ref[i-1] = table[i] end

   b. Otherwise, ``resize`` must be *true*, *false*, or
      an explicit number. If it is not false, the container
      is resized. After that the usual struct-like 'pairs'
      assignment is performed.

      In case ``resize`` is *true*, the size is computed
      by scanning the table for the largest numeric key.

   This means that in order to reassign only one element of
   a container using this system, it is necessary to use::

      { resize=false, [idx]=value }

Since nil inside a table is indistinguishable from missing key,
it is necessary to use ``df.NULL`` as a null pointer value.

This system is intended as a way to define a nested object
tree using pure lua data structures, and then materialize it in
C++ memory in one go. Note that if pointer auto-vivification
is used, an error in the middle of the recursive walk would
not destroy any objects allocated in this way, so the user
should be prepared to catch the error and do the necessary
cleanup.

==========
DFHack API
==========

DFHack utility functions are placed in the ``dfhack`` global tree.

Native utilities
================

Input & Output
--------------

* ``dfhack.print(args...)``

  Output tab-separated args as standard lua print would do,
  but without a newline.

* ``print(args...)``, ``dfhack.println(args...)``

  A replacement of the standard library print function that
  works with DFHack output infrastructure.

* ``dfhack.printerr(args...)``

  Same as println; intended for errors. Uses red color and logs to stderr.log.

* ``dfhack.color([color])``

  Sets the current output color. If color is *nil* or *-1*, resets to default.
  Returns the previous color value.

* ``dfhack.is_interactive()``

  Checks if the thread can access the interactive console and returns *true* or *false*.

* ``dfhack.lineedit([prompt[,history_filename]])``

  If the thread owns the interactive console, shows a prompt
  and returns the entered string. Otherwise returns *nil, error*.

  Depending on the context, this function may actually yield the
  running coroutine and let the C++ code release the core suspend
  lock. Using an explicit ``dfhack.with_suspend`` will prevent
  this, forcing the function to block on input with lock held.

* ``dfhack.interpreter([prompt[,env[,history_filename]]])``

  Starts an interactive lua interpreter, using the specified prompt
  string, global environment and command-line history file.

  If the interactive console is not accessible, returns *nil, error*.


Exception handling
------------------

* ``dfhack.error(msg[,level[,verbose]])``

  Throws a dfhack exception object with location and stack trace.
  The verbose parameter controls whether the trace is printed by default.

* ``qerror(msg[,level])``

  Calls ``dfhack.error()`` with ``verbose`` being *false*. Intended to
  be used for user-caused errors in scripts, where stack traces are not
  desirable.

* ``dfhack.pcall(f[,args...])``

  Invokes f via xpcall, using an error function that attaches
  a stack trace to the error. The same function is used by SafeCall
  in C++, and dfhack.safecall.

* ``safecall(f[,args...])``, ``dfhack.safecall(f[,args...])``

  Just like pcall, but also prints the error using printerr before
  returning. Intended as a convenience function.

* ``dfhack.saferesume(coroutine[,args...])``

  Compares to coroutine.resume like dfhack.safecall vs pcall.

* ``dfhack.exception``

  Metatable of error objects used by dfhack. The objects have the
  following properties:

  ``err.where``
    The location prefix string, or *nil*.
  ``err.message``
    The base message string.
  ``err.stacktrace``
    The stack trace string, or *nil*.
  ``err.cause``
    A different exception object, or *nil*.
  ``err.thread``
    The coroutine that has thrown the exception.
  ``err.verbose``
    Boolean, or *nil*; specifies if where and stacktrace should be printed.
  ``tostring(err)``, or ``err:tostring([verbose])``
    Converts the exception to string.

* ``dfhack.exception.verbose``

  The default value of the ``verbose`` argument of ``err:tostring()``.


Locking and finalization
------------------------

* ``dfhack.with_suspend(f[,args...])``

  Calls ``f`` with arguments after grabbing the DF core suspend lock.
  Suspending is necessary for accessing a consistent state of DF memory.

  Returned values and errors are propagated through after releasing
  the lock. It is safe to nest suspends.

  Every thread is allowed only one suspend per DF frame, so it is best
  to group operations together in one big critical section. A plugin
  can choose to run all lua code inside a C++-side suspend lock.

* ``dfhack.call_with_finalizer(num_cleanup_args,always,cleanup_fn[,cleanup_args...],fn[,args...])``

  Invokes ``fn`` with ``args``, and after it returns or throws an
  error calls ``cleanup_fn`` with ``cleanup_args``. Any return values from
  ``fn`` are propagated, and errors are re-thrown.

  The ``num_cleanup_args`` integer specifies the number of ``cleanup_args``,
  and the ``always`` boolean specifies if cleanup should be called in any case,
  or only in case of an error.

* ``dfhack.with_finalize(cleanup_fn,fn[,args...])``

  Calls ``fn`` with arguments, then finalizes with ``cleanup_fn``.
  Implemented using ``call_with_finalizer(0,true,...)``.

* ``dfhack.with_onerror(cleanup_fn,fn[,args...])``

  Calls ``fn`` with arguments, then finalizes with ``cleanup_fn`` on any thrown error.
  Implemented using ``call_with_finalizer(0,false,...)``.

* ``dfhack.with_temp_object(obj,fn[,args...])``

  Calls ``fn(obj,args...)``, then finalizes with ``obj:delete()``.


Persistent configuration storage
--------------------------------

This api is intended for storing configuration options in the world itself.
It probably should be restricted to data that is world-dependent.

Entries are identified by a string ``key``, but it is also possible to manage
multiple entries with the same key; their identity is determined by ``entry_id``.
Every entry has a mutable string ``value``, and an array of 7 mutable ``ints``.

* ``dfhack.persistent.get(key)``, ``entry:get()``

  Retrieves a persistent config record with the given string key,
  or refreshes an already retrieved entry. If there are multiple
  entries with the same key, it is undefined which one is retrieved
  by the first version of the call.

  Returns entry, or *nil* if not found.

* ``dfhack.persistent.delete(key)``, ``entry:delete()``

  Removes an existing entry. Returns *true* if succeeded.

* ``dfhack.persistent.get_all(key[,match_prefix])``

  Retrieves all entries with the same key, or starting with key..'/'.
  Calling ``get_all('',true)`` will match all entries.

  If none found, returns nil; otherwise returns an array of entries.

* ``dfhack.persistent.save({key=str1, ...}[,new])``, ``entry:save([new])``

  Saves changes in an entry, or creates a new one. Passing true as
  new forces creation of a new entry even if one already exists;
  otherwise the existing one is simply updated.
  Returns *entry, did_create_new*

Since the data is hidden in data structures owned by the DF world,
and automatically stored in the save game, these save and retrieval
functions can just copy values in memory without doing any actual I/O.
However, currently every entry has a 180+-byte dead-weight overhead.

Material info lookup
--------------------

A material info record has fields:

* ``type``, ``index``, ``material``

  DF material code pair, and a reference to the material object.

* ``mode``

  One of ``'builtin'``, ``'inorganic'``, ``'plant'``, ``'creature'``.

* ``inorganic``, ``plant``, ``creature``

  If the material is of the matching type, contains a reference to the raw object.

* ``figure``

  For a specific creature material contains a ref to the historical figure.

Functions:

* ``dfhack.matinfo.decode(type,index)``

  Looks up material info for the given number pair; if not found, returs *nil*.

* ``....decode(matinfo)``, ``....decode(item)``, ``....decode(obj)``

  Uses ``matinfo.type``/``matinfo.index``, item getter vmethods,
  or ``obj.mat_type``/``obj.mat_index`` to get the code pair.

* ``dfhack.matinfo.find(token[,token...])``

  Looks up material by a token string, or a pre-split string token sequence.

* ``dfhack.matinfo.getToken(...)``, ``info:getToken()``

  Applies ``decode`` and constructs a string token.

* ``info:toString([temperature[,named]])``

  Returns the human-readable name at the given temperature.

* ``info:getCraftClass()``

  Returns the classification used for craft skills.

* ``info:matches(obj)``

  Checks if the material matches job_material_category or job_item.
  Accept dfhack_material_category auto-assign table.

C++ function wrappers
=====================

Thin wrappers around C++ functions, similar to the ones for virtual methods.
One notable difference is that these explicit wrappers allow argument count
adjustment according to the usual lua rules, so trailing false/nil arguments
can be omitted.

* ``dfhack.getOSType()``

  Returns the OS type string from ``symbols.xml``.

* ``dfhack.getDFVersion()``

  Returns the DF version string from ``symbols.xml``.

* ``dfhack.getDFPath()``

  Returns the DF directory path.

* ``dfhack.getHackPath()``

  Returns the dfhack directory path, i.e. ``".../df/hack/"``.

* ``dfhack.isWorldLoaded()``

  Checks if the world is loaded.

* ``dfhack.isMapLoaded()``

  Checks if the world and map are loaded.

* ``dfhack.TranslateName(name[,in_english,only_last_name])``

  Convert a language_name or only the last name part to string.

Gui module
----------

* ``dfhack.gui.getCurViewscreen()``

  Returns the viewscreen that is current in the core.

* ``dfhack.gui.getFocusString(viewscreen)``

  Returns a string representation of the current focus position
  in the ui. The string has a "screen/foo/bar/baz..." format.

* ``dfhack.gui.getSelectedWorkshopJob([silent])``

  When a job is selected in *'q'* mode, returns the job, else
  prints error unless silent and returns *nil*.

* ``dfhack.gui.getSelectedJob([silent])``

  Returns the job selected in a workshop or unit/jobs screen.

* ``dfhack.gui.getSelectedUnit([silent])``

  Returns the unit selected via *'v'*, *'k'*, unit/jobs, or
  a full-screen item view of a cage or suchlike.

* ``dfhack.gui.getSelectedItem([silent])``

  Returns the item selected via *'v'* ->inventory, *'k'*, *'t'*, or
  a full-screen item view of a container. Note that in the
  last case, the highlighted *contained item* is returned, not
  the container itself.

* ``dfhack.gui.showAnnouncement(text,color[,is_bright])``

  Adds a regular announcement with given text, color, and brightness.
  The is_bright boolean actually seems to invert the brightness.

* ``dfhack.gui.showPopupAnnouncement(text,color[,is_bright])``

  Pops up a titan-style modal announcement window.

Job module
----------

* ``dfhack.job.cloneJobStruct(job)``

  Creates a deep copy of the given job.

* ``dfhack.job.printJobDetails(job)``

  Prints info about the job.

* ``dfhack.job.printItemDetails(jobitem,idx)``

  Prints info about the job item.

* ``dfhack.job.getHolder(job)``

  Returns the building holding the job.

* ``dfhack.job.getWorker(job)``

  Returns the unit performing the job.

* ``dfhack.job.checkBuildingsNow()``

  Instructs the game to check buildings for jobs next frame and assign workers.

* ``dfhack.job.checkDesignationsNow()``

  Instructs the game to check designations for jobs next frame and assign workers.

* ``dfhack.job.is_equal(job1,job2)``

  Compares important fields in the job and nested item structures.

* ``dfhack.job.is_item_equal(job_item1,job_item2)``

  Compares important fields in the job item structures.

* ``dfhack.job.listNewlyCreated(first_id)``

  Returns the current value of ``df.global.job_next_id``, and
  if there are any jobs with ``first_id <= id < job_next_id``,
  a lua list containing them.

Units module
------------

* ``dfhack.units.getPosition(unit)``

  Returns true *x,y,z* of the unit, or *nil* if invalid; may be not equal to unit.pos if caged.

* ``dfhack.units.getContainer(unit)``

  Returns the container (cage) item or *nil*.

* ``dfhack.units.setNickname(unit,nick)``

  Sets the unit's nickname properly.

* ``dfhack.units.getVisibleName(unit)``

  Returns the language_name object visible in game, accounting for false identities.

* ``dfhack.units.getIdentity(unit)``

  Returns the false identity of the unit if it has one, or *nil*.

* ``dfhack.units.getNemesis(unit)``

  Returns the nemesis record of the unit if it has one, or *nil*.

* ``dfhack.units.isDead(unit)``

  The unit is completely dead and passive, or a ghost.

* ``dfhack.units.isAlive(unit)``

  The unit isn't dead or undead.

* ``dfhack.units.isSane(unit)``

  The unit is capable of rational action, i.e. not dead, insane, zombie, or active werewolf.

* ``dfhack.units.isDwarf(unit)``

  The unit is of the correct race of the fortress.

* ``dfhack.units.isCitizen(unit)``

  The unit is an alive sane citizen of the fortress; wraps the
  same checks the game uses to decide game-over by extinction.

* ``dfhack.units.getAge(unit[,true_age])``

  Returns the age of the unit in years as a floating-point value.
  If ``true_age`` is true, ignores false identities.

* ``dfhack.units.getNoblePositions(unit)``

  Returns a list of tables describing noble position assignments, or *nil*.
  Every table has fields ``entity``, ``assignment`` and ``position``.

* ``dfhack.units.getProfessionName(unit[,ignore_noble,plural])``

  Retrieves the profession name using custom profession, noble assignments
  or raws. The ``ignore_noble`` boolean disables the use of noble positions.

* ``dfhack.units.getCasteProfessionName(race,caste,prof_id[,plural])``

  Retrieves the profession name for the given race/caste using raws.


Items module
------------

* ``dfhack.items.getPosition(item)``

  Returns true *x,y,z* of the item, or *nil* if invalid; may be not equal to item.pos if in inventory.

* ``dfhack.items.getDescription(item, type[, decorate])``

  Returns the string description of the item, as produced by the getItemDescription
  method. If decorate is true, also adds markings for quality and improvements.

* ``dfhack.items.getGeneralRef(item, type)``

  Searches for a general_ref with the given type.

* ``dfhack.items.getSpecificRef(item, type)``

  Searches for a specific_ref with the given type.

* ``dfhack.items.getOwner(item)``

  Returns the owner unit or *nil*.

* ``dfhack.items.setOwner(item,unit)``

  Replaces the owner of the item. If unit is *nil*, removes ownership.
  Returns *false* in case of error.

* ``dfhack.items.getContainer(item)``

  Returns the container item or *nil*.

* ``dfhack.items.getContainedItems(item)``

  Returns a list of items contained in this one.

* ``dfhack.items.moveToGround(item,pos)``

  Move the item to the ground at position. Returns *false* if impossible.

* ``dfhack.items.moveToContainer(item,container)``

  Move the item to the container. Returns *false* if impossible.

* ``dfhack.items.moveToBuilding(item,building,use_mode)``

  Move the item to the building. Returns *false* if impossible.

* ``dfhack.items.moveToInventory(item,unit,use_mode,body_part)``

  Move the item to the unit inventory. Returns *false* if impossible.


Maps module
-----------

* ``dfhack.maps.getSize()``

  Returns map size in blocks: *x, y, z*

* ``dfhack.maps.getTileSize()``

  Returns map size in tiles: *x, y, z*

* ``dfhack.maps.getBlock(x,y,z)``

  Returns a map block object for given x,y,z in local block coordinates.

* ``dfhack.maps.getTileBlock(coords)``, or ``getTileBlock(x,y,z)``

  Returns a map block object for given df::coord or x,y,z in local tile coordinates.

* ``dfhack.maps.getRegionBiome(region_coord2d)``, or ``getRegionBiome(x,y)``

  Returns the biome info struct for the given global map region.

* ``dfhack.maps.enableBlockUpdates(block[,flow,temperature])``

  Enables updates for liquid flow or temperature, unless already active.

* ``dfhack.maps.getGlobalInitFeature(index)``

  Returns the global feature object with the given index.

* ``dfhack.maps.getLocalInitFeature(region_coord2d,index)``

  Returns the local feature object with the given region coords and index.

* ``dfhack.maps.getTileBiomeRgn(coords)``, or ``getTileBiomeRgn(x,y,z)``

  Returns *x, y* for use with ``getRegionBiome``.

* ``dfhack.maps.canWalkBetween(pos1, pos2)``

  Checks if a dwarf may be able to walk between the two tiles,
  using a pathfinding cache maintained by the game. Note that
  this cache is only updated when the game is unpaused, and thus
  can get out of date if doors are forbidden or unforbidden, or
  tools like liquids or tiletypes are used. It also cannot possibly
  take into account anything that depends on the actual units, like
  burrows, or the presence of invaders.


Burrows module
--------------

* ``dfhack.burrows.findByName(name)``

  Returns the burrow pointer or *nil*.

* ``dfhack.burrows.clearUnits(burrow)``

  Removes all units from the burrow.

* ``dfhack.burrows.isAssignedUnit(burrow,unit)``

  Checks if the unit is in the burrow.

* ``dfhack.burrows.setAssignedUnit(burrow,unit,enable)``

  Adds or removes the unit from the burrow.

* ``dfhack.burrows.clearTiles(burrow)``

  Removes all tiles from the burrow.

* ``dfhack.burrows.listBlocks(burrow)``

  Returns a table of map block pointers.

* ``dfhack.burrows.isAssignedTile(burrow,tile_coord)``

  Checks if the tile is in burrow.

* ``dfhack.burrows.setAssignedTile(burrow,tile_coord,enable)``

  Adds or removes the tile from the burrow. Returns *false* if invalid coords.

* ``dfhack.burrows.isAssignedBlockTile(burrow,block,x,y)``

  Checks if the tile within the block is in burrow.

* ``dfhack.burrows.setAssignedBlockTile(burrow,block,x,y,enable)``

  Adds or removes the tile from the burrow. Returns *false* if invalid coords.


Buildings module
----------------

* ``dfhack.buildings.getSize(building)``

  Returns *width, height, centerx, centery*.

* ``dfhack.buildings.findAtTile(pos)``, or ``findAtTile(x,y,z)``

  Scans the buildings for the one located at the given tile.
  Does not work on civzones. Warning: linear scan if the map
  tile indicates there are buildings at it.

* ``dfhack.buildings.findCivzonesAt(pos)``, or ``findCivzonesAt(x,y,z)``

  Scans civzones, and returns a lua sequence of those that touch
  the given tile, or *nil* if none.

* ``dfhack.buildings.getCorrectSize(width, height, type, subtype, custom, direction)``

  Computes correct dimensions for the specified building type and orientation,
  using width and height for flexible dimensions.
  Returns *is_flexible, width, height, center_x, center_y*.

* ``dfhack.buildings.checkFreeTiles(pos,size[,extents,change_extents,allow_occupied])``

  Checks if the rectangle defined by ``pos`` and ``size``, and possibly extents,
  can be used for placing a building. If ``change_extents`` is true, bad tiles
  are removed from extents. If ``allow_occupied``, the occupancy test is skipped.

* ``dfhack.buildings.countExtentTiles(extents,defval)``

  Returns the number of tiles included by extents, or defval.

* ``dfhack.buildings.containsTile(building, x, y[, room])``

  Checks if the building contains the specified tile, either directly, or as room.

* ``dfhack.buildings.hasSupport(pos,size)``

  Checks if a bridge constructed at specified position would have
  support from terrain, and thus won't collapse if retracted.

Low-level building creation functions;

* ``dfhack.buildings.allocInstance(pos, type, subtype, custom)``

  Creates a new building instance of given type, subtype and custom type,
  at specified position. Returns the object, or *nil* in case of an error.

* ``dfhack.buildings.setSize(building, width, height, direction)``

  Configures an object returned by ``allocInstance``, using specified
  parameters wherever appropriate. If the building has fixed size along
  any dimension, the corresponding input parameter will be ignored.
  Returns *false* if the building cannot be placed, or *true, width,
  height, rect_area, true_area*. Returned width and height are the
  final values used by the building; true_area is less than rect_area
  if any tiles were removed from designation.

* ``dfhack.buildings.constructAbstract(building)``

  Links a fully configured object created by ``allocInstance`` into the
  world. The object must be an abstract building, i.e. a stockpile or civzone.
  Returns *true*, or *false* if impossible.

* ``dfhack.buildings.constructWithItems(building, items)``

  Links a fully configured object created by ``allocInstance`` into the
  world for construction, using a list of specific items as material.
  Returns *true*, or *false* if impossible.

* ``dfhack.buildings.constructWithFilters(building, job_items)``

  Links a fully configured object created by ``allocInstance`` into the
  world for construction, using a list of job_item filters as inputs.
  Returns *true*, or *false* if impossible. Filter objects are claimed
  and possibly destroyed in any case.
  Use a negative ``quantity`` field value to auto-compute the amount
  from the size of the building.

* ``dfhack.buildings.deconstruct(building)``

  Destroys the building, or queues a deconstruction job.
  Returns *true* if the building was destroyed and deallocated immediately.

More high-level functions are implemented in lua and can be loaded by
``require('dfhack.buildings')``. See ``hack/lua/dfhack/buildings.lua``.

Among them are:

* ``dfhack.buildings.getFiltersByType(argtable,type,subtype,custom)``

  Returns a sequence of lua structures, describing input item filters
  suitable for the specified building type, or *nil* if unknown or invalid.
  The returned sequence is suitable for use as the ``job_items`` argument
  of ``constructWithFilters``.
  Uses tables defined in ``buildings.lua``.

  Argtable members ``material`` (the default name), ``bucket``, ``barrel``,
  ``chain``, ``mechanism``, ``screw``, ``pipe``, ``anvil``, ``weapon`` are used to
  augment the basic attributes with more detailed information if the
  building has input items with the matching name (see the tables for naming details).
  Note that it is impossible to *override* any properties this way, only supply those that
  are not mentioned otherwise; one exception is that flags2.non_economic
  is automatically cleared if an explicit material is specified.

* ``dfhack.buildings.constructBuilding{...}``

  Creates a building in one call, using options contained
  in the argument table. Returns the building, or *nil, error*.

  **NOTE:** Despite the name, unless the building is abstract,
  the function creates it in an 'unconstructed' stage, with
  a queued in-game job that will actually construct it. I.e.
  the function replicates programmatically what can be done
  through the construct building menu in the game ui, except
  that it does less environment constraint checking.

  The following options can be used:

  - ``pos = coordinates``, or ``x = ..., y = ..., z = ...``

    Mandatory. Specifies the left upper corner of the building.

  - ``type = df.building_type.FOO, subtype = ..., custom = ...``

    Mandatory. Specifies the type of the building. Obviously, subtype
    and custom are only expected if the type requires them.

  - ``fields = { ... }``

    Initializes fields of the building object after creation with ``df.assign``.

  - ``width = ..., height = ..., direction = ...``

    Sets size and orientation of the building. If it is
    fixed-size, specified dimensions are ignored.

  - ``full_rectangle = true``

    For buildings like stockpiles or farm plots that can normally
    accomodate individual tile exclusion, forces an error if any
    tiles within the specified width*height are obstructed.

  - ``items = { item, item ... }``, or ``filters = { {...}, {...}... }``

    Specifies explicit items or item filters to use in construction.
    It is the job of the user to ensure they are correct for the building type.

  - ``abstract = true``

    Specifies that the building is abstract and does not require construction.
    Required for stockpiles and civzones; an error otherwise.

  - ``material = {...}, mechanism = {...}, ...``

    If none of ``items``, ``filter``, or ``abstract`` is used,
    the function uses ``getFiltersByType`` to compute the input
    item filters, and passes the argument table through. If no filters
    can be determined this way, ``constructBuilding`` throws an error.


Constructions module
--------------------

* ``dfhack.constructions.designateNew(pos,type,item_type,mat_index)``

  Designates a new construction at given position. If there already is
  a planned but not completed construction there, changes its type.
  Returns *true*, or *false* if obstructed.
  Note that designated constructions are technically buildings.

* ``dfhack.constructions.designateRemove(pos)``, or ``designateRemove(x,y,z)``

  If there is a construction or a planned construction at the specified
  coordinates, designates it for removal, or instantly cancels the planned one.
  Returns *true, was_only_planned* if removed; or *false* if none found.


Internal API
------------

These functions are intended for the use by dfhack developers,
and are only documented here for completeness:

* ``dfhack.internal.scripts``

  The table used by ``dfhack.run_script()`` to give every script its own
  global environment, persistent between calls to the script.

* ``dfhack.internal.getAddress(name)``

  Returns the global address ``name``, or *nil*.

* ``dfhack.internal.setAddress(name, value)``

  Sets the global address ``name``. Returns the value of ``getAddress`` before the change.

* ``dfhack.internal.getVTable(name)``

  Returns the pre-extracted vtable address ``name``, or *nil*.

* ``dfhack.internal.getRebaseDelta()``

  Returns the ASLR rebase offset of the DF executable.

* ``dfhack.internal.getMemRanges()``

  Returns a sequence of tables describing virtual memory ranges of the process.

* ``dfhack.internal.memmove(dest,src,count)``

  Wraps the standard memmove function. Accepts both numbers and refs as pointers.

* ``dfhack.internal.memcmp(ptr1,ptr2,count)``

  Wraps the standard memcmp function.

* ``dfhack.internal.memscan(haystack,count,step,needle,nsize)``

  Searches for ``needle`` of ``nsize`` bytes in ``haystack``,
  using ``count`` steps of ``step`` bytes.
  Returns: *step_idx, sum_idx, found_ptr*, or *nil* if not found.

* ``dfhack.internal.diffscan(old_data, new_data, start_idx, end_idx, eltsize[, oldval, newval, delta])``

  Searches for differences between buffers at ptr1 and ptr2, as integers of size eltsize.
  The oldval, newval or delta arguments may be used to specify additional constraints.
  Returns: *found_index*, or *nil* if end reached.


Core interpreter context
========================

While plugins can create any number of interpreter instances,
there is one special context managed by dfhack core. It is the
only context that can receive events from DF and plugins.

Core context specific functions:

* ``dfhack.is_core_context``

  Boolean value; *true* in the core context.

* ``dfhack.timeout(time,mode,callback)``

  Arranges for the callback to be called once the specified
  period of time passes. The ``mode`` argument specifies the
  unit of time used, and may be one of ``'frames'`` (raw FPS),
  ``'ticks'`` (unpaused FPS), ``'days'``, ``'months'``,
  ``'years'`` (in-game time). All timers other than
  ``'frames'`` are cancelled when the world is unloaded,
  and cannot be queued until it is loaded again.
  Returns the timer id, or *nil* if unsuccessful due to
  world being unloaded.

* ``dfhack.timeout_active(id[,new_callback])``

  Returns the active callback with the given id, or *nil*
  if inactive or nil id. If called with 2 arguments, replaces
  the current callback with the given value, if still active.
  Using ``timeout_active(id,nil)`` cancels the timer.

* ``dfhack.onStateChange.foo = function(code)``

  Event. Receives the same codes as plugin_onstatechange in C++.


Event type
----------

An event is just a lua table with a predefined metatable that
contains a __call metamethod. When it is invoked, it loops
through the table with next and calls all contained values.
This is intended as an extensible way to add listeners.

This type itself is available in any context, but only the
core context has the actual events defined by C++ code.

Features:

* ``dfhack.event.new()``

  Creates a new instance of an event.

* ``event[key] = function``

  Sets the function as one of the listeners.

  **NOTE**: The ``df.NULL`` key is reserved for the use by
  the C++ owner of the event, and has some special semantics.

* ``event(args...)``

  Invokes all listeners contained in the event in an arbitrary
  order using ``dfhack.safecall``.


===========
Lua Modules
===========

DFHack sets up the lua interpreter so that the built-in ``require``
function can be used to load shared lua code from hack/lua/.
The ``dfhack`` namespace reference itself may be obtained via
``require('dfhack')``, although it is initially created as a
global by C++ bootstrap code.

The following module management functions are provided:

* ``mkmodule(name)``

  Creates an environment table for the module. Intended to be used as::

    local _ENV = mkmodule('foo')
    ...
    return _ENV

  If called the second time, returns the same table; thus providing reload support.

* ``reload(name)``

  Reloads a previously ``require``-d module *"name"* from the file.
  Intended as a help for module development.

* ``dfhack.BASE_G``

  This variable contains the root global environment table, which is
  used as a base for all module and script environments. Its contents
  should be kept limited to the standard Lua library and API described
  in this document.

Global environment
==================

A number of variables and functions are provided in the base global
environment by the mandatory init file dfhack.lua:

* Color constants

  These are applicable both for ``dfhack.color()`` and color fields
  in DF functions or structures:

  COLOR_RESET, COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
  COLOR_RED, COLOR_MAGENTA, COLOR_BROWN, COLOR_GREY, COLOR_DARKGREY,
  COLOR_LIGHTBLUE, COLOR_LIGHTGREEN, COLOR_LIGHTCYAN, COLOR_LIGHTRED,
  COLOR_LIGHTMAGENTA, COLOR_YELLOW, COLOR_WHITE

* ``dfhack.onStateChange`` event codes

  Available only in the core context, as is the event itself:

  SC_WORLD_LOADED, SC_WORLD_UNLOADED, SC_MAP_LOADED,
  SC_MAP_UNLOADED, SC_VIEWSCREEN_CHANGED, SC_CORE_INITIALIZED

* Functions already described above

  safecall, qerror, mkmodule, reload

* ``printall(obj)``

  If the argument is a lua table or DF object reference, prints all fields.

* ``copyall(obj)``

  Returns a shallow copy of the table or reference as a lua table.

* ``pos2xyz(obj)``

  The object must have fields x, y and z. Returns them as 3 values.
  If obj is *nil*, or x is -30000 (the usual marker for undefined
  coordinates), returns *nil*.

* ``xyz2pos(x,y,z)``

  Returns a table with x, y and z as fields.

* ``safe_index(obj,index...)``

  Walks a sequence of dereferences, which may be represented by numbers or strings.
  Returns *nil* if any of obj or indices is *nil*, or a numeric index is out of array bounds.

utils
=====

* ``utils.compare(a,b)``

  Comparator function; returns *-1* if a<b, *1* if a>b, *0* otherwise.

* ``utils.compare_name(a,b)``

  Comparator for names; compares empty string last.

* ``utils.is_container(obj)``

  Checks if obj is a container ref.

* ``utils.make_index_sequence(start,end)``

  Returns a lua sequence of numbers in start..end.

* ``utils.make_sort_order(data, ordering)``

  Computes a sorted permutation of objects in data, as a table of integer
  indices into the data sequence. Uses ``data.n`` as input length
  if present.

  The ordering argument is a sequence of ordering specs, represented
  as lua tables with following possible fields:

  ord.key = *function(value)*
    Computes comparison key from input data value. Not called on nil.
    If omitted, the comparison key is the value itself.
  ord.key_table = *function(data)*
    Computes a key table from the data table in one go.
  ord.compare = *function(a,b)*
    Comparison function. Defaults to ``utils.compare`` above.
    Called on non-nil keys; nil sorts last.
  ord.nil_first = *true/false*
    If true, nil keys are sorted first instead of last.
  ord.reverse = *true/false*
    If true, sort non-nil keys in descending order.

  For every comparison during sorting the specs are applied in
  order until an unambiguous decision is reached. Sorting is stable.

  Example of sorting a sequence by field foo::

    local spec = { key = function(v) return v.foo end }
    local order = utils.make_sort_order(data, { spec })
    local output = {}
    for i = 1,#order do output[i] = data[order[i]] end

  Separating the actual reordering of the sequence in this
  way enables applying the same permutation to multiple arrays.
  This function is used by the sort plugin.

* ``utils.assign(tgt, src)``

  Does a recursive assignment of src into tgt.
  Uses ``df.assign`` if tgt is a native object ref; otherwise
  recurses into lua tables.

* ``utils.clone(obj, deep)``

  Performs a shallow, or semi-deep copy of the object as a lua table tree.
  The deep mode recurses into lua tables and subobjects, except pointers
  to other heap objects.
  Null pointers are represented as df.NULL. Zero-based native containers
  are converted to 1-based lua sequences.

* ``utils.clone_with_default(obj, default, force)``

  Copies the object, using the ``default`` lua table tree
  as a guide to which values should be skipped as uninteresting.
  The ``force`` argument makes it always return a non-*nil* value.

* ``utils.sort_vector(vector,field,cmpfun)``

  Sorts a native vector or lua sequence using the comparator function.
  If ``field`` is not *nil*, applies the comparator to the field instead
  of the whole object.

* ``utils.binsearch(vector,key,field,cmpfun,min,max)``

  Does a binary search in a native vector or lua sequence for
  ``key``, using ``cmpfun`` and ``field`` like sort_vector.
  If ``min`` and ``max`` are specified, they are used as the
  search subrange bounds.

  If found, returns *item, true, idx*. Otherwise returns
  *nil, false, insert_idx*, where *insert_idx* is the correct
  insertion point.

* ``utils.insert_sorted(vector,item,field,cmpfun)``

  Does a binary search, and inserts item if not found.
  Returns *did_insert, vector[idx], idx*.

* ``utils.insert_or_update(vector,item,field,cmpfun)``

  Like ``insert_sorted``, but also assigns the item into
  the vector cell if insertion didn't happen.

  As an example, you can use this to set skill values::

    utils.insert_or_update(soul.skills, {new=true, id=..., rating=...}, 'id')

  (For an explanation of ``new=true``, see table assignment in the wrapper section)

* ``utils.prompt_yes_no(prompt, default)``

  Presents a yes/no prompt to the user. If ``default`` is not *nil*,
  allows just pressing Enter to submit the default choice.
  If the user enters ``'abort'``, throws an error.

* ``utils.prompt_input(prompt, checkfun, quit_str)``

  Presents a prompt to input data, until a valid string is entered.
  Once ``checkfun(input)`` returns *true, ...*, passes the values
  through. If the user enters the quit_str (defaults to ``'~~~'``),
  throws an error.

* ``utils.check_number(text)``

  A ``prompt_input`` ``checkfun`` that verifies a number input.

dumper
======

A third-party lua table dumper module from
http://lua-users.org/wiki/DataDumper. Defines one
function:

* ``dumper.DataDumper(value, varname, fastmode, ident, indent_step)``

  Returns ``value`` converted to a string. The ``indent_step``
  argument specifies the indentation step size in spaces. For
  the other arguments see the original documentation link above.


=======
Plugins
=======

DFHack plugins may export native functions and events
to lua contexts. They are automatically imported by
``mkmodule('plugins.<name>')``; this means that a lua
module file is still necessary for ``require`` to read.

The following plugins have lua support.

burrows
=======

Implements extended burrow manipulations.

Events:

* ``onBurrowRename.foo = function(burrow)``

  Emitted when a burrow might have been renamed either through
  the game UI, or ``renameBurrow()``.

* ``onDigComplete.foo = function(job_type,pos,old_tiletype,new_tiletype,worker)``

  Emitted when a tile might have been dug out. Only tracked if the
  auto-growing burrows feature is enabled.

Native functions:

* ``renameBurrow(burrow,name)``

  Renames the burrow, emitting ``onBurrowRename`` and updating auto-grow state properly.

* ``findByName(burrow,name)``

  Finds a burrow by name, using the same rules as the plugin command line interface.
  Namely, trailing ``'+'`` characters marking auto-grow burrows are ignored.

* ``copyUnits(target,source,enable)``

  Applies units from ``source`` burrow to ``target``. The ``enable``
  parameter specifies if they are to be added or removed.

* ``copyTiles(target,source,enable)``

  Applies tiles from ``source`` burrow to ``target``. The ``enable``
  parameter specifies if they are to be added or removed.

* ``setTilesByKeyword(target,keyword,enable)``

  Adds or removes tiles matching a predefined keyword. The keyword
  set is the same as used by the command line.

The lua module file also re-exports functions from ``dfhack.burrows``.

sort
====

Does not export any native functions as of now. Instead, it
calls lua code to perform the actual ordering of list items.


=======
Scripts
=======

Any files with the .lua extension placed into hack/scripts/*
are automatically used by the DFHack core as commands. The
matching command name consists of the name of the file sans
the extension.

If the first line of the script is a one-line comment, it is
used by the built-in ``ls`` and ``help`` commands.

**NOTE:** Scripts placed in subdirectories still can be accessed, but
do not clutter the ``ls`` command list; thus it is preferred
for obscure developer-oriented scripts and scripts used by tools.
When calling such scripts, always use '/' as the separator for
directories, e.g. ``devel/lua-example``.

Scripts are re-read from disk every time they are used
(this may be changed later to check the file change time); however
the global variable values persist in memory between calls.
Every script gets its own separate environment for global
variables.

Arguments are passed in to the scripts via the **...** built-in
quasi-variable; when the script is called by the DFHack core,
they are all guaranteed to be non-nil strings.

DFHack core invokes the scripts in the *core context* (see above);
however it is possible to call them from any lua code (including
from other scripts) in any context, via the same function the core uses:

* ``dfhack.run_script(name[,args...])``

  Run a lua script in hack/scripts/, as if it was started from dfhack command-line.
  The ``name`` argument should be the name stem, as would be used on the command line.

Note that this function lets errors propagate to the caller.
