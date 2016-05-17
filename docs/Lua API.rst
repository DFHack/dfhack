.. _lua-api:

##############
DFHack Lua API
##############

DFHack has extensive support for
the Lua_ scripting language, providing access to:

.. _Lua: http://www.lua.org

1. Raw data structures used by the game.
2. Many C++ functions for high-level access to these
   structures, and interaction with dfhack itself.
3. Some functions exported by C++ plugins.

Lua code can be used both for writing scripts, which
are treated by DFHack command line prompt almost as
native C++ commands, and invoked by plugins written in C++.

This document describes native API available to Lua in detail.
It does not describe all of the utility functions
implemented by Lua files located in :file:`hack/lua/*`
(:file:`library/lua/*` in the git repo).


.. contents::
   :depth: 3


=========================
DF data structure wrapper
=========================

.. contents::
   :local:

Data structures of the game are defined in XML files located in :file:`library/xml`
(and `online <http://github.com/DFHack/df-structures>`_, and automatically exported
to lua code as a tree of objects and functions under the ``df`` global, which
also broadly maps to the ``df`` namespace in the headers generated for C++.

.. warning::
    The wrapper provides almost raw access to the memory
    of the game, so mistakes in manipulating objects are as likely to
    crash the game as equivalent plain C++ code would be.

    eg. NULL pointer access is safely detected, but dangling pointers aren't.

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

  .. warning::
    a few of the data structures (like ui_look_list)
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

  .. warning::
    the lua reference object remains as a dangling
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
  to the target, or ``nil``/NULL. Complex types are represented by
  a reference to the field within the structure; unless recursive
  lua table assignment is used, such fields can only be read.

  .. note::
    In case of inheritance, *superclass* fields have precedence
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
  order, which is the same as declaration order.

Container references
--------------------

Containers represent vectors and arrays, possibly resizable.

A container field can associate an enum to the container
reference, which allows accessing elements using string keys
instead of numerical indices.

Note that two-dimensional arrays in C++ (ie pointers to pointers)
are exposed to lua as one-dimensional.  The best way to handle this
is probably ``array[x].value:_displace(y)``.

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
  ``DFHack::type_instance`` object.

Types excluding the global object also support:

* ``type:sizeof()``

  Returns the size of an object of the type.

* ``type:new()``

  Creates a new instance of an object of the type.

* ``type:is_instance(object)``

  Returns true if object is same or subclass type, or a reference
  to an object of same or subclass type. It is permissible to pass
  ``nil``, NULL or non-wrapper value as object; in this case the
  method returns ``nil``.

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

.. _lua-api-table-assignment:

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

Since ``nil`` inside a table is indistinguishable from missing key,
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

.. contents::
   :local:

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

* ``dfhack.interpreter([prompt[,history_filename[,env]]])``

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


Miscellaneous
-------------

* ``dfhack.VERSION``

  DFHack version string constant.

* ``dfhack.curry(func,args...)``, or ``curry(func,args...)``

  Returns a closure that invokes the function with args combined
  both from the curry call and the closure call itself. I.e.
  ``curry(func,a,b)(c,d)`` equals ``func(a,b,c,d)``.


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

This section describes the old way of storing persistent data in the game. It is inefficient because it has overheads of over a hundred bytes per persistent data element. The new system is to use ``persist-table``, which will allow DFHack to store persistent data in a json file.

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

It is also possible to associate one bit per map tile with an entry,
using these two methods:

* ``entry:getTilemask(block[, create])``

  Retrieves the tile bitmask associated with this entry in the given map
  block. If ``create`` is *true*, an empty mask is created if none exists;
  otherwise the function returns *nil*, which must be assumed to be the same
  as an all-zero mask.

* ``entry:deleteTilemask(block)``

  Deletes the associated tile mask from the given map block.

Note that these masks are only saved in fortress mode, and also that deleting
the persistent entry will **NOT** delete the associated masks.


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

.. _lua_api_random:

Random number generation
------------------------

* ``dfhack.random.new([seed[,perturb_count]])``

  Creates a new random number generator object. Without any
  arguments, the object is initialized using current time.
  Otherwise, the seed must be either a non-negative integer,
  or a list of such integers. The second argument may specify
  the number of additional randomization steps performed to
  improve the initial state.

* ``rng:init([seed[,perturb_count]])``

  Re-initializes an already existing random number generator object.

* ``rng:random([limit])``

  Returns a random integer. If ``limit`` is specified, the value
  is in the range [0, limit); otherwise it uses the whole 32-bit
  unsigned integer range.

* ``rng:drandom()``

  Returns a random floating-point number in the range [0,1).

* ``rng:drandom0()``

  Returns a random floating-point number in the range (0,1).

* ``rng:drandom1()``

  Returns a random floating-point number in the range [0,1].

* ``rng:unitrandom()``

  Returns a random floating-point number in the range [-1,1].

* ``rng:unitvector([size])``

  Returns multiple values that form a random vector of length 1,
  uniformly distributed over the corresponding sphere surface.
  The default size is 3.

* ``fn = rng:perlin([dim]); fn(x[,y[,z]])``

  Returns a closure that computes a classical Perlin noise function
  of dimension *dim*, initialized from this random generator.
  Dimension may be 1, 2 or 3 (default).


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

* ``dfhack.getDFHackVersion()``
* ``dfhack.getDFHackRelease()``
* ``dfhack.getCompiledDFVersion()``
* ``dfhack.getGitDescription()``
* ``dfhack.getGitCommit()``
* ``dfhack.isRelease()``

  Return information about the DFHack build in use.

  .. note::
    ``getCompiledDFVersion()`` returns the DF version specified at compile time,
    while ``getDFVersion()`` returns the version and typically the OS as well.
    These do not necessarily match - for example, DFHack 0.34.11-r5 worked with
    DF 0.34.10 and 0.34.11, so the former function would always return ``0.34.11``
    while the latter would return ``v0.34.10 <platform>`` or ``v0.34.11 <platform>``.

* ``dfhack.getDFPath()``

  Returns the DF directory path.

* ``dfhack.getHackPath()``

  Returns the dfhack directory path, i.e. ``".../df/hack/"``.

* ``dfhack.getSavePath()``

  Returns the path to the current save directory, or *nil* if no save loaded.

* ``dfhack.getTickCount()``

  Returns the tick count in ms, exactly as DF ui uses.

* ``dfhack.isWorldLoaded()``

  Checks if the world is loaded.

* ``dfhack.isMapLoaded()``

  Checks if the world and map are loaded.

* ``dfhack.TranslateName(name[,in_english,only_last_name])``

  Convert a language_name or only the last name part to string.

* ``dfhack.df2utf(string)``

  Convert a string from DF's CP437 encoding to UTF-8.

* ``dfhack.df2console()``

  Convert a string from DF's CP437 encoding to the correct encoding for the
  DFHack console.

* ``dfhack.utf2df(string)``

  Convert a string from UTF-8 to DF's CP437 encoding.

**Note:** When printing CP437-encoded text to the console (for example, names
returned from TranslateName()), use ``print(dfhack.df2console(text)`` to ensure
proper display on all platforms.


Gui module
----------

* ``dfhack.gui.getCurViewscreen([skip_dismissed])``

  Returns the topmost viewscreen. If ``skip_dismissed`` is *true*,
  ignores screens already marked to be removed.

* ``dfhack.gui.getFocusString(viewscreen)``

  Returns a string representation of the current focus position
  in the ui. The string has a "screen/foo/bar/baz..." format.

* ``dfhack.gui.getCurFocus([skip_dismissed])``

  Returns the focus string of the current viewscreen.

* ``dfhack.gui.getViewscreenByType(type [, depth])``

  Returns the topmost viewscreen out of the top ``depth`` viewscreens with
  the specified type (e.g. ``df.viewscreen_titlest``), or ``nil`` if none match.
  If ``depth`` is not specified or is less than 1, all viewscreens are checked.

* ``dfhack.gui.getSelectedWorkshopJob([silent])``

  When a job is selected in :kbd:`q` mode, returns the job, else
  prints error unless silent and returns *nil*.

* ``dfhack.gui.getSelectedJob([silent])``

  Returns the job selected in a workshop or unit/jobs screen.

* ``dfhack.gui.getSelectedUnit([silent])``

  Returns the unit selected via :kbd:`v`, :kbd:`k`, unit/jobs, or
  a full-screen item view of a cage or suchlike.

* ``dfhack.gui.getSelectedItem([silent])``

  Returns the item selected via :kbd:`v` ->inventory, :kbd:`k`, :kbd:`t`, or
  a full-screen item view of a container. Note that in the
  last case, the highlighted *contained item* is returned, not
  the container itself.

* ``dfhack.gui.getSelectedBuilding([silent])``

  Returns the building selected via :kbd:`q`, :kbd:`t`, :kbd:`k` or :kbd:`i`.

* ``dfhack.gui.writeToGamelog(text)``

  Writes a string to :file:`gamelog.txt` without doing an announcement.

* ``dfhack.gui.makeAnnouncement(type,flags,pos,text,color[,is_bright])``

  Adds an announcement with given announcement_type, text, color, and brightness.
  The is_bright boolean actually seems to invert the brightness.

  The announcement is written to :file:`gamelog.txt`. The announcement_flags
  argument provides a custom set of :file:`announcements.txt` options,
  which specify if the message should actually be displayed in the
  announcement list, and whether to recenter or show a popup.

  Returns the index of the new announcement in ``df.global.world.status.reports``, or -1.

* ``dfhack.gui.addCombatReport(unit,slot,report_index)``

  Adds the report with the given index (returned by makeAnnouncement)
  to the specified group of the given unit. Returns *true* on success.

* ``dfhack.gui.addCombatReportAuto(unit,flags,report_index)``

  Adds the report with the given index to the appropriate group(s)
  of the given unit, as requested by the flags.

* ``dfhack.gui.showAnnouncement(text,color[,is_bright])``

  Adds a regular announcement with given text, color, and brightness.
  The is_bright boolean actually seems to invert the brightness.

* ``dfhack.gui.showZoomAnnouncement(type,pos,text,color[,is_bright])``

  Like above, but also specifies a position you can zoom to from the announcement menu.

* ``dfhack.gui.showPopupAnnouncement(text,color[,is_bright])``

  Pops up a titan-style modal announcement window.

* ``dfhack.gui.showAutoAnnouncement(type,pos,text,color[,is_bright,unit1,unit2])``

  Uses the type to look up options from announcements.txt, and calls the above
  operations accordingly. The units are used to call ``addCombatReportAuto``.


Job module
----------

* ``dfhack.job.cloneJobStruct(job)``

  Creates a deep copy of the given job.

* ``dfhack.job.printJobDetails(job)``

  Prints info about the job.

* ``dfhack.job.printItemDetails(jobitem,idx)``

  Prints info about the job item.

* ``dfhack.job.getGeneralRef(job, type)``

  Searches for a general_ref with the given type.

* ``dfhack.job.getSpecificRef(job, type)``

  Searches for a specific_ref with the given type.

* ``dfhack.job.getHolder(job)``

  Returns the building holding the job.

* ``dfhack.job.getWorker(job)``

  Returns the unit performing the job.

* ``dfhack.job.setJobCooldown(building,worker,timeout)``

  Prevent the worker from taking jobs at the specified workshop for the specified time.
  This doesn't decrease the timeout in any circumstances.

* ``dfhack.job.removeWorker(job,timeout)``

  Removes the worker from the specified workshop job, and sets the cooldown.
  Returns *true* on success.

* ``dfhack.job.checkBuildingsNow()``

  Instructs the game to check buildings for jobs next frame and assign workers.

* ``dfhack.job.checkDesignationsNow()``

  Instructs the game to check designations for jobs next frame and assign workers.

* ``dfhack.job.is_equal(job1,job2)``

  Compares important fields in the job and nested item structures.

* ``dfhack.job.is_item_equal(job_item1,job_item2)``

  Compares important fields in the job item structures.

* ``dfhack.job.linkIntoWorld(job,new_id)``

  Adds job into ``df.global.job_list``, and if new_id
  is true, then also sets its id and increases
  ``df.global.job_next_id``

* ``dfhack.job.listNewlyCreated(first_id)``

  Returns the current value of ``df.global.job_next_id``, and
  if there are any jobs with ``first_id <= id < job_next_id``,
  a lua list containing them.

* ``dfhack.job.isSuitableItem(job_item, item_type, item_subtype)``

  Does basic sanity checks to verify if the suggested item type matches
  the flags in the job item.

* ``dfhack.job.isSuitableMaterial(job_item, mat_type, mat_index)``

  Likewise, if replacing material.

* ``dfhack.job.getName(job)``

  Returns the job's description, as seen in the Units and Jobs screens.

Units module
------------

* ``dfhack.units.getPosition(unit)``

  Returns true *x,y,z* of the unit, or *nil* if invalid; may be not equal to unit.pos if caged.

* ``dfhack.units.getGeneralRef(unit, type)``

  Searches for a general_ref with the given type.

* ``dfhack.units.getSpecificRef(unit, type)``

  Searches for a specific_ref with the given type.

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

* ``dfhack.units.isHidingCurse(unit)``

  Checks if the unit hides improved attributes from its curse.

* ``dfhack.units.getPhysicalAttrValue(unit, attr_type)``
* ``dfhack.units.getMentalAttrValue(unit, attr_type)``

  Computes the effective attribute value, including curse effect.

* ``dfhack.units.isCrazed(unit)``
* ``dfhack.units.isOpposedToLife(unit)``
* ``dfhack.units.hasExtravision(unit)``
* ``dfhack.units.isBloodsucker(unit)``

  Simple checks of caste attributes that can be modified by curses.

* ``dfhack.units.getMiscTrait(unit, type[, create])``

  Finds (or creates if requested) a misc trait object with the given id.

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

* ``dfhack.units.getNominalSkill(unit, skill[, use_rust])``

  Retrieves the nominal skill level for the given unit. If ``use_rust``
  is *true*, subtracts the rust penalty.

* ``dfhack.units.getEffectiveSkill(unit, skill)``

  Computes the effective rating for the given skill, taking into account exhaustion, pain etc.

* ``dfhack.units.getExperience(unit, skill[, total])``

  Returns the experience value for the given skill. If ``total`` is true, adds experience implied by the current rating.

* ``dfhack.units.computeMovementSpeed(unit)``

  Computes number of frames * 100 it takes the unit to move in its current state of mind and body.

* ``dfhack.units.computeSlowdownFactor(unit)``

  Meandering and floundering in liquid introduces additional slowdown. It is
  random, but the function computes and returns the expected mean factor as a float.

* ``dfhack.units.getNoblePositions(unit)``

  Returns a list of tables describing noble position assignments, or *nil*.
  Every table has fields ``entity``, ``assignment`` and ``position``.

* ``dfhack.units.getProfessionName(unit[,ignore_noble,plural])``

  Retrieves the profession name using custom profession, noble assignments
  or raws. The ``ignore_noble`` boolean disables the use of noble positions.

* ``dfhack.units.getCasteProfessionName(race,caste,prof_id[,plural])``

  Retrieves the profession name for the given race/caste using raws.

* ``dfhack.units.getProfessionColor(unit[,ignore_noble])``

  Retrieves the color associated with the profession, using noble assignments
  or raws. The ``ignore_noble`` boolean disables the use of noble positions.

* ``dfhack.units.getCasteProfessionColor(race,caste,prof_id)``

  Retrieves the profession color for the given race/caste using raws.


Items module
------------

* ``dfhack.items.getPosition(item)``

  Returns true *x,y,z* of the item, or *nil* if invalid; may be not equal to item.pos if in inventory.

* ``dfhack.items.getDescription(item, type[, decorate])``

  Returns the string description of the item, as produced by the ``getItemDescription``
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

* ``dfhack.items.getHolderBuilding(item)``

  Returns the holder building or *nil*.

* ``dfhack.items.getHolderUnit(item)``

  Returns the holder unit or *nil*.

* ``dfhack.items.moveToGround(item,pos)``

  Move the item to the ground at position. Returns *false* if impossible.

* ``dfhack.items.moveToContainer(item,container)``

  Move the item to the container. Returns *false* if impossible.

* ``dfhack.items.moveToBuilding(item,building[,use_mode[,force_in_building])``

  Move the item to the building. Returns *false* if impossible.

  ``use_mode`` defaults to 0. If set to 2, the item will be treated as part of the building.

  If ``force_in_building`` is true, the item will be considered to be stored by the building
  (used for items temporarily used in traps in vanilla DF)

* ``dfhack.items.moveToInventory(item,unit,use_mode,body_part)``

  Move the item to the unit inventory. Returns *false* if impossible.

* ``dfhack.items.remove(item[, no_uncat])``

  Removes the item, and marks it for garbage collection unless ``no_uncat`` is true.

* ``dfhack.items.makeProjectile(item)``

  Turns the item into a projectile, and returns the new object, or *nil* if impossible.

* ``dfhack.items.isCasteMaterial(item_type)``

  Returns *true* if this item type uses a creature/caste pair as its material.

* ``dfhack.items.getSubtypeCount(item_type)``

  Returns the number of raw-defined subtypes of the given item type, or *-1* if not applicable.

* ``dfhack.items.getSubtypeDef(item_type, subtype)``

  Returns the raw definition for the given item type and subtype, or *nil* if invalid.

* ``dfhack.items.getItemBaseValue(item_type, subtype, material, mat_index)``

  Calculates the base value for an item of the specified type and material.

* ``dfhack.items.getValue(item)``

  Calculates the Basic Value of an item, as seen in the View Item screen.


Maps module
-----------

* ``dfhack.maps.getSize()``

  Returns map size in blocks: *x, y, z*

* ``dfhack.maps.getTileSize()``

  Returns map size in tiles: *x, y, z*

* ``dfhack.maps.getBlock(x,y,z)``

  Returns a map block object for given x,y,z in local block coordinates.

* ``dfhack.maps.isValidTilePos(coords)``, or ``isValidTilePos(x,y,z)``

  Checks if the given df::coord or x,y,z in local tile coordinates are valid.

* ``dfhack.maps.getTileBlock(coords)``, or ``getTileBlock(x,y,z)``

  Returns a map block object for given df::coord or x,y,z in local tile coordinates.

* ``dfhack.maps.ensureTileBlock(coords)``, or ``ensureTileBlock(x,y,z)``

  Like ``getTileBlock``, but if the block is not allocated, try creating it.

* ``dfhack.maps.getTileType(coords)``, or ``getTileType(x,y,z)``

  Returns the tile type at the given coordinates, or *nil* if invalid.

* ``dfhack.maps.getTileFlags(coords)``, or ``getTileFlags(x,y,z)``

  Returns designation and occupancy references for the given coordinates, or *nil, nil* if invalid.

* ``dfhack.maps.getRegionBiome(region_coord2d)``, or ``getRegionBiome(x,y)``

  Returns the biome info struct for the given global map region.

* ``dfhack.maps.enableBlockUpdates(block[,flow,temperature])``

  Enables updates for liquid flow or temperature, unless already active.

* ``dfhack.maps.spawnFlow(pos,type,mat_type,mat_index,dimension)``

  Spawns a new flow (i.e. steam/mist/dust/etc) at the given pos, and with
  the given parameters. Returns it, or *nil* if unsuccessful.

* ``dfhack.maps.getGlobalInitFeature(index)``

  Returns the global feature object with the given index.

* ``dfhack.maps.getLocalInitFeature(region_coord2d,index)``

  Returns the local feature object with the given region coords and index.

* ``dfhack.maps.getTileBiomeRgn(coords)``, or ``getTileBiomeRgn(x,y,z)``

  Returns *x, y* for use with ``getRegionBiome``.

* ``dfhack.maps.canWalkBetween(pos1, pos2)``

  Checks if a dwarf may be able to walk between the two tiles,
  using a pathfinding cache maintained by the game.

  .. note::
    This cache is only updated when the game is unpaused, and thus
    can get out of date if doors are forbidden or unforbidden, or
    tools like `liquids` or `tiletypes` are used. It also cannot possibly
    take into account anything that depends on the actual units, like
    burrows, or the presence of invaders.

* ``dfhack.maps.hasTileAssignment(tilemask)``

  Checks if the tile_bitmask object is not *nil* and contains any set bits; returns *true* or *false*.

* ``dfhack.maps.getTileAssignment(tilemask,x,y)``

  Checks if the tile_bitmask object is not *nil* and has the relevant bit set; returns *true* or *false*.

* ``dfhack.maps.setTileAssignment(tilemask,x,y,enable)``

  Sets the relevant bit in the tile_bitmask object to the *enable* argument.

* ``dfhack.maps.resetTileAssignment(tilemask[,enable])``

  Sets all bits in the mask to the *enable* argument.


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

General
~~~~~~~

* ``dfhack.buildings.getGeneralRef(building, type)``

  Searches for a general_ref with the given type.

* ``dfhack.buildings.getSpecificRef(building, type)``

  Searches for a specific_ref with the given type.

* ``dfhack.buildings.setOwner(item,unit)``

  Replaces the owner of the building. If unit is *nil*, removes ownership.
  Returns *false* in case of error.

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

* ``dfhack.buildings.getStockpileContents(stockpile)``

  Returns a list of items stored on the given stockpile.
  Ignores empty bins, barrels, and wheelbarrows assigned as storage and transport for that stockpile.

Low-level
~~~~~~~~~
Low-level building creation functions:

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

High-level
~~~~~~~~~~
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

  .. note::
    Despite the name, unless the building is abstract,
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


Screen API
----------

The screen module implements support for drawing to the tiled screen of the game.
Note that drawing only has any effect when done from callbacks, so it can only
be feasibly used in the core context.

Basic painting functions:

* ``dfhack.screen.getWindowSize()``

  Returns *width, height* of the screen.

* ``dfhack.screen.getMousePos()``

  Returns *x,y* of the tile the mouse is over.

* ``dfhack.screen.inGraphicsMode()``

  Checks if [GRAPHICS:YES] was specified in init.

* ``dfhack.screen.paintTile(pen,x,y[,char,tile])``

  Paints a tile using given parameters. See below for a description of pen.

  Returns *false* if coordinates out of bounds, or other error.

* ``dfhack.screen.readTile(x,y)``

  Retrieves the contents of the specified tile from the screen buffers.
  Returns a pen object, or *nil* if invalid or TrueType.

* ``dfhack.screen.paintString(pen,x,y,text)``

  Paints the string starting at *x,y*. Uses the string characters
  in sequence to override the ``ch`` field of pen.

  Returns *true* if painting at least one character succeeded.

* ``dfhack.screen.fillRect(pen,x1,y1,x2,y2)``

  Fills the rectangle specified by the coordinates with the given pen.
  Returns *true* if painting at least one character succeeded.

* ``dfhack.screen.findGraphicsTile(pagename,x,y)``

  Finds a tile from a graphics set (i.e. the raws used for creatures),
  if in graphics mode and loaded.

  Returns: *tile, tile_grayscale*, or *nil* if not found.
  The values can then be used for the *tile* field of *pen* structures.

* ``dfhack.screen.clear()``

  Fills the screen with blank background.

* ``dfhack.screen.invalidate()``

  Requests repaint of the screen by setting a flag. Unlike other
  functions in this section, this may be used at any time.

* ``dfhack.screen.getKeyDisplay(key)``

  Returns the string that should be used to represent the given
  logical keybinding on the screen in texts like "press Key to ...".

* ``dfhack.screen.keyToChar(key)``

  Returns the integer character code of the string input
  character represented by the given logical keybinding,
  or *nil* if not a string input key.

* ``dfhack.screen.charToKey(charcode)``

  Returns the keybinding representing the given string input
  character, or *nil* if impossible.

The "pen" argument used by functions above may be represented by
a table with the following possible fields:

  ``ch``
    Provides the ordinary tile character, as either a 1-character string or a number.
    Can be overridden with the ``char`` function parameter.
  ``fg``
    Foreground color for the ordinary tile. Defaults to COLOR_GREY (7).
  ``bg``
    Background color for the ordinary tile. Defaults to COLOR_BLACK (0).
  ``bold``
    Bright/bold text flag. If *nil*, computed based on (fg & 8); fg is masked to 3 bits.
    Otherwise should be *true/false*.
  ``tile``
    Graphical tile id. Ignored unless [GRAPHICS:YES] was in init.txt.
  ``tile_color = true``
    Specifies that the tile should be shaded with *fg/bg*.
  ``tile_fg, tile_bg``
    If specified, overrides *tile_color* and supplies shading colors directly.

Alternatively, it may be a pre-parsed native object with the following API:

* ``dfhack.pen.make(base[,pen_or_fg,bg,bold])``

  Creates a new pre-parsed pen by combining its arguments according to the
  following rules:

  1. The ``base`` argument may be a pen object, a pen table as specified above,
     or a single color value. In the single value case, it is split into
     ``fg`` and ``bold`` properties, and others are initialized to 0.
     This argument will be converted to a pre-parsed object and returned
     if there are no other arguments.

  2. If the ``pen_or_fg`` argument is specified as a table or object, it
     completely replaces the base, and is returned instead of it.

  3. Otherwise, the non-nil subset of the optional arguments is used
     to update the ``fg``, ``bg`` and ``bold`` properties of the base.
     If the ``bold`` flag is *nil*, but *pen_or_fg* is a number, ``bold``
     is deduced from it like in the simple base case.

  This function always returns a new pre-parsed pen, or *nil*.

* ``dfhack.pen.parse(base[,pen_or_fg,bg,bold])``

  Exactly like the above function, but returns ``base`` or ``pen_or_fg``
  directly if they are already a pre-parsed native object.

* ``pen.property``, ``pen.property = value``, ``pairs(pen)``

  Pre-parsed pens support reading and setting their properties,
  but don't behave exactly like a simple table would; for instance,
  assigning to ``pen.tile_color`` also resets ``pen.tile_fg`` and
  ``pen.tile_bg`` to *nil*.

In order to actually be able to paint to the screen, it is necessary
to create and register a viewscreen (basically a modal dialog) with
the game.

.. warning::
    As a matter of policy, in order to avoid user confusion, all
    interface screens added by dfhack should bear the "DFHack" signature.

Screens are managed with the following functions:

* ``dfhack.screen.show(screen[,below])``

  Displays the given screen, possibly placing it below a different one.
  The screen must not be already shown. Returns *true* if success.

* ``dfhack.screen.dismiss(screen[,to_first])``

  Marks the screen to be removed when the game enters its event loop.
  If ``to_first`` is *true*, all screens up to the first one will be deleted.

* ``dfhack.screen.isDismissed(screen)``

  Checks if the screen is already marked for removal.

Apart from a native viewscreen object, these functions accept a table
as a screen. In this case, ``show`` creates a new native viewscreen
that delegates all processing to methods stored in that table.

.. note:: Lua-implemented screens are only supported in the core context.

Supported callbacks and fields are:

* ``screen._native``

  Initialized by ``show`` with a reference to the backing viewscreen
  object, and removed again when the object is deleted.

* ``function screen:onShow()``

  Called by ``dfhack.screen.show`` if successful.

* ``function screen:onDismiss()``

  Called by ``dfhack.screen.dismiss`` if successful.

* ``function screen:onDestroy()``

  Called from the destructor when the viewscreen is deleted.

* ``function screen:onResize(w, h)``

  Called before ``onRender`` or ``onIdle`` when the window size has changed.

* ``function screen:onRender()``

  Called when the viewscreen should paint itself. This is the only context
  where the above painting functions work correctly.

  If omitted, the screen is cleared; otherwise it should do that itself.
  In order to make a see-through dialog, call ``self._native.parent:render()``.

* ``function screen:onIdle()``

  Called every frame when the screen is on top of the stack.

* ``function screen:onHelp()``

  Called when the help keybinding is activated (usually '?').

* ``function screen:onInput(keys)``

  Called when keyboard or mouse events are available.
  If any keys are pressed, the keys argument is a table mapping them to *true*.
  Note that this refers to logical keybingings computed from real keys via
  options; if multiple interpretations exist, the table will contain multiple keys.

  The table also may contain special keys:

  ``_STRING``
    Maps to an integer in range 0-255. Duplicates a separate "STRING_A???" code for convenience.

  ``_MOUSE_L, _MOUSE_R``
    If the left or right mouse button is being pressed.

  ``_MOUSE_L_DOWN, _MOUSE_R_DOWN``
    If the left or right mouse button was just pressed.

  If this method is omitted, the screen is dismissed on receival of the ``LEAVESCREEN`` key.

* ``function screen:onGetSelectedUnit()``
* ``function screen:onGetSelectedItem()``
* ``function screen:onGetSelectedJob()``
* ``function screen:onGetSelectedBuilding()``

  Implement these to provide a return value for the matching
  ``dfhack.gui.getSelected...`` function.


PenArray class
--------------

Screens that require significant computation in their onRender() method can use
a ``dfhack.penarray`` instance to cache their output.

* ``dfhack.penarray.new(w, h)``

  Creates a new penarray instance with an internal buffer of ``w * h`` tiles.
  These dimensions currently cannot be changed after a penarray is instantiated.

* ``penarray:clear()``

  Clears the internal buffer, similar to ``dfhack.screen.clear()``.

* ``penarray:get_dims()``

  Returns the x and y dimensions of the internal buffer.

* ``penarray:get_tile(x, y)``

  Returns a pen corresponding to the tile at (``x``, ``y``) in the internal buffer.
  Note that indices are 0-based.

* ``penarray:set_tile(x, y, pen)``

  Sets the tile at (``x``, ``y``) in the internal buffer to the pen given.

* ``penarray:draw(x, y, w, h, bufferx, buffery)``

  Draws the contents of the internal buffer, beginning at
  (``bufferx``, ``buffery``) and spanning ``w`` columns and ``h`` rows, to the
  screen starting at (``x``, ``y``). Any invalid screen and buffer coordinates
  are skipped.

  ``bufferx`` and ``buffery`` default to 0.

Filesystem module
-----------------

Most of these functions return ``true`` on success and ``false`` on failure,
unless otherwise noted.

* ``dfhack.filesystem.exists(path)``

  Returns ``true`` if ``path`` exists.

* ``dfhack.filesystem.isfile(path)``

  Returns ``true`` if ``path`` exists and is a file.

* ``dfhack.filesystem.isdir(path)``

  Returns ``true`` if ``path`` exists and is a directory.

* ``dfhack.filesystem.getcwd()``

  Returns the current working directory. To retrieve the DF path, use ``dfhack.getDFPath()`` instead.

* ``dfhack.filesystem.chdir(path)``

  Changes the current directory to ``path``. Use with caution.

* ``dfhack.filesystem.mkdir(path)``

  Creates a new directory. Returns ``false`` if unsuccessful, including if ``path`` already exists.

* ``dfhack.filesystem.rmdir(path)``

  Removes a directory. Only works if the directory is already empty.

* ``dfhack.filesystem.mtime(path)``

  Returns the modification time (in seconds) of the file or directory specified by ``path``,
  or -1 if ``path`` does not exist. This depends on the system clock and should only be used locally.

* ``dfhack.filesystem.atime(path)``
* ``dfhack.filesystem.ctime(path)``

  Return values vary across operating systems - return the ``st_atime`` and ``st_ctime``
  fields of a C++ stat struct, respectively.

* ``dfhack.filesystem.listdir(path)``

  Lists files/directories in a directory.  Returns ``{}`` if ``path`` does not exist.

* ``dfhack.filesystem.listdir_recursive(path [, depth = 10])``

  Lists all files/directories in a directory and its subdirectories. All directories
  are listed before their contents. Returns a table with subtables of the format::

    {path: 'path to file', isdir: true|false}

  Note that ``listdir()`` returns only the base name of each directory entry, while
  ``listdir_recursive()`` returns the initial path and all components following it
  for each entry.

Internal API
------------

These functions are intended for the use by dfhack developers,
and are only documented here for completeness:

* ``dfhack.internal.scripts``

  The table used by ``dfhack.run_script()`` to give every script its own
  global environment, persistent between calls to the script.

* ``dfhack.internal.getPE()``

  Returns the PE timestamp of the DF executable (only on Windows)

* ``dfhack.internal.getMD5()``

  Returns the MD5 of the DF executable (only on OS X and Linux)

* ``dfhack.internal.getAddress(name)``

  Returns the global address ``name``, or *nil*.

* ``dfhack.internal.setAddress(name, value)``

  Sets the global address ``name``. Returns the value of ``getAddress`` before the change.

* ``dfhack.internal.getVTable(name)``

  Returns the pre-extracted vtable address ``name``, or *nil*.

* ``dfhack.internal.getImageBase()``

  Returns the mmap base of the executable.

* ``dfhack.internal.getRebaseDelta()``

  Returns the ASLR rebase offset of the DF executable.

* ``dfhack.internal.adjustOffset(offset[,to_file])``

  Returns the re-aligned offset, or *nil* if invalid.
  If ``to_file`` is true, the offset is adjusted from memory to file.
  This function returns the original value everywhere except windows.

* ``dfhack.internal.getMemRanges()``

  Returns a sequence of tables describing virtual memory ranges of the process.

* ``dfhack.internal.patchMemory(dest,src,count)``

  Like memmove below, but works even if dest is read-only memory, e.g. code.
  If destination overlaps a completely invalid memory region, or another error
  occurs, returns false.

* ``dfhack.internal.patchBytes(write_table[, verify_table])``

  The first argument must be a lua table, which is interpreted as a mapping from
  memory addresses to byte values that should be stored there. The second argument
  may be a similar table of values that need to be checked before writing anything.

  The function takes care to either apply all of ``write_table``, or none of it.
  An empty ``write_table`` with a nonempty ``verify_table`` can be used to reasonably
  safely check if the memory contains certain values.

  Returns *true* if successful, or *nil, error_msg, address* if not.

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

* ``dfhack.internal.getDir(path)``

  Lists files/directories in a directory.
  Returns: *file_names* or empty table if not found. Identical to ``dfhack.filesystem.listdir(path)``.

* ``dfhack.internal.strerror(errno)``

  Wraps strerror() - returns a string describing a platform-specific error code

* ``dfhack.internal.addScriptPath(path, search_before)``

  Adds ``path`` to the list of paths searched for scripts (both in Lua and Ruby).
  If ``search_before`` is passed and ``true``, the path will be searched before
  the default paths (e.g. ``raw/scripts``, ``hack/scripts``); otherwise, it will
  be searched after.

  Returns ``true`` if successful or ``false`` otherwise (e.g. if the path does
  not exist or has already been registered).

* ``dfhack.internal.removeScriptPath(path)``

  Removes ``path`` from the script search paths and returns ``true`` if successful.

* ``dfhack.internal.getScriptPaths()``

  Returns the list of script paths in the order they are searched, including defaults.
  (This can change if a world is loaded.)

* ``dfhack.internal.findScript(name)``

  Searches script paths for the script ``name`` and returns the path of the first
  file found, or ``nil`` on failure.

  .. note::
    This requires an extension to be specified (``.lua`` or ``.rb``) - use
    ``dfhack.findScript()`` to include the ``.lua`` extension automatically.

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

An event is a native object transparently wrapping a lua table,
and implementing a __call metamethod. When it is invoked, it loops
through the table with next and calls all contained values.
This is intended as an extensible way to add listeners.

This type itself is available in any context, but only the
core context has the actual events defined by C++ code.

Features:

* ``dfhack.event.new()``

  Creates a new instance of an event.

* ``event[key] = function``

  Sets the function as one of the listeners. Assign *nil* to remove it.

  .. note::
    The ``df.NULL`` key is reserved for the use by
    the C++ owner of the event; it is an error to try setting it.

* ``#event``

  Returns the number of non-nil listeners.

* ``pairs(event)``

  Iterates over all listeners in the table.

* ``event(args...)``

  Invokes all listeners contained in the event in an arbitrary
  order using ``dfhack.safecall``.


===========
Lua Modules
===========

.. contents::
   :local:

DFHack sets up the lua interpreter so that the built-in ``require``
function can be used to load shared lua code from :file:`hack/lua/`.
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
  in DF functions or structures::

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

* Miscellaneous constants

  ``NEWLINE``, ``COMMA``, ``PERIOD``
    evaluate to the relevant character strings.
  ``DEFAULT_NIL``
    is an unspecified unique token used by the class module below.

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

* ``same_xyz(a,b)``

  Checks if ``a`` and ``b`` have the same x, y and z fields.

* ``get_path_xyz(path,i)``

  Returns ``path.x[i], path.y[i], path.z[i]``.

* ``pos2xy(obj)``, ``xy2pos(x,y)``, ``same_xy(a,b)``, ``get_path_xy(a,b)``

  Same as above, but for 2D coordinates.

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

* ``for link,item in utils.listpairs(list)``

  Iterates a df-list structure, for example ``df.global.world.job_list``.

* ``utils.assign(tgt, src)``

  Does a recursive assignment of src into tgt.
  Uses ``df.assign`` if tgt is a native object ref; otherwise
  recurses into lua tables.

* ``utils.clone(obj, deep)``

  Performs a shallow, or semi-deep copy of the object as a lua table tree.
  The deep mode recurses into lua tables and subobjects, except pointers
  to other heap objects.
  Null pointers are represented as ``df.NULL``. Zero-based native containers
  are converted to 1-based lua sequences.

* ``utils.clone_with_default(obj, default, force)``

  Copies the object, using the ``default`` lua table tree
  as a guide to which values should be skipped as uninteresting.
  The ``force`` argument makes it always return a non-*nil* value.

* ``utils.parse_bitfield_int(value, type_ref)``

  Given an int ``value``, and a bitfield type in the ``df`` tree,
  it returns a lua table mapping the enabled bit keys to *true*,
  unless value is 0, in which case it returns *nil*.

* ``utils.list_bitfield_flags(bitfield[, list])``

  Adds all enabled bitfield keys to ``list`` or a newly-allocated
  empty sequence, and returns it. The ``bitfield`` argument may
  be *nil*.

* ``utils.sort_vector(vector,field,cmpfun)``

  Sorts a native vector or lua sequence using the comparator function.
  If ``field`` is not *nil*, applies the comparator to the field instead
  of the whole object.

* ``utils.linear_index(vector,key[,field])``

  Searches for ``key`` in the vector, and returns *index, found_value*,
  or *nil* if none found.

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

  (For an explanation of ``new=true``, see `lua-api-table-assignment`)

* ``utils.erase_sorted_key(vector,key,field,cmpfun)``

  Removes the item with the given key from the list. Returns: *did_erase, vector[idx], idx*.

* ``utils.erase_sorted(vector,item,field,cmpfun)``

  Exactly like ``erase_sorted_key``, but if field is specified, takes the key from ``item[field]``.

* ``utils.call_with_string(obj,methodname,...)``

  Allocates a temporary string object, calls ``obj:method(tmp,...)``, and
  returns the value written into the temporary after deleting it.

* ``utils.getBuildingName(building)``

  Returns the string description of the given building.

* ``utils.getBuildingCenter(building)``

  Returns an x/y/z table pointing at the building center.

* ``utils.split_string(string, delimiter)``

  Splits the string by the given delimiter, and returns a sequence of results.

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

class
=====

Implements a trivial single-inheritance class system.

* ``Foo = defclass(Foo[, ParentClass])``

  Defines or updates class Foo. The ``Foo = defclass(Foo)`` syntax
  is needed so that when the module or script is reloaded, the
  class identity will be preserved through the preservation of
  global variable values.

  The ``defclass`` function is defined as a stub in the global
  namespace, and using it will auto-load the class module.

* ``Class.super``

  This class field is set by defclass to the parent class, and
  allows a readable ``Class.super.method(self, ...)`` syntax for
  calling superclass methods.

* ``Class.ATTRS { foo = xxx, bar = yyy }``

  Declares certain instance fields to be attributes, i.e. auto-initialized
  from fields in the table used as the constructor argument. If omitted,
  they are initialized with the default values specified in this declaration.

  If the default value should be *nil*, use ``ATTRS { foo = DEFAULT_NIL }``.

  Declaring an attribute is mostly the same as defining your ``init`` method like this::

    function Class.init(args)
        self.attr1 = args.attr1 or default1
        self.attr2 = args.attr2 or default2
        ...
    end

  The main difference is that attributes are processed as a separate
  initialization step, before any ``init`` methods are called. They
  also make the directy relation between instance fields and constructor
  arguments more explicit.

* ``new_obj = Class{ foo = arg, bar = arg, ... }``

  Calling the class as a function creates and initializes a new instance.
  Initialization happens in this order:

  1. An empty instance table is created, and its metatable set.
  2. The ``preinit`` methods are called via ``invoke_before`` (see below)
     with the table used as argument to the class. These methods are intended
     for validating and tweaking that argument table.
  3. Declared ATTRS are initialized from the argument table or their default values.
  4. The ``init`` methods are called via ``invoke_after`` with the argument table.
     This is the main constructor method.
  5. The ``postinit`` methods are called via ``invoke_after`` with the argument table.
     Place code that should be called after the object is fully constructed here.

Predefined instance methods:

* ``instance:assign{ foo = xxx }``

  Assigns all values in the input table to the matching instance fields.

* ``instance:callback(method_name, [args...])``

  Returns a closure that invokes the specified method of the class,
  properly passing in self, and optionally a number of initial arguments too.
  The arguments given to the closure are appended to these.

* ``instance:cb_getfield(field_name)``

  Returns a closure that returns the specified field of the object when called.

* ``instance:cb_setfield(field_name)``

  Returns a closure that sets the specified field to its argument when called.

* ``instance:invoke_before(method_name, args...)``

  Navigates the inheritance chain of the instance starting from the most specific
  class, and invokes the specified method with the arguments if it is defined in
  that specific class. Equivalent to the following definition in every class::

    function Class:invoke_before(method, ...)
      if rawget(Class, method) then
        rawget(Class, method)(self, ...)
      end
      Class.super.invoke_before(method, ...)
    end

* ``instance:invoke_after(method_name, args...)``

  Like invoke_before, only the method is called after the recursive call to super,
  i.e. invocations happen in the parent to child order.

  These two methods are inspired by the Common Lisp before and after methods, and
  are intended for implementing similar protocols for certain things. The class
  library itself uses them for constructors.

To avoid confusion, these methods cannot be redefined.

==================
In-game UI Library
==================

.. contents::
   :local:

A number of lua modules with names starting with ``gui`` are dedicated
to wrapping the natives of the ``dfhack.screen`` module in a way that
is easy to use. This allows relatively easily and naturally creating
dialogs that integrate in the main game UI window.

These modules make extensive use of the ``class`` module, and define
things ranging from the basic ``Painter``, ``View`` and ``Screen``
classes, to fully functional predefined dialogs.

gui
===

This module defines the most important classes and functions for
implementing interfaces. This documents those of them that are
considered stable.


Misc
----

* ``USE_GRAPHICS``

  Contains the value of ``dfhack.screen.inGraphicsMode()``, which cannot be
  changed without restarting the game and thus is constant during the session.

* ``CLEAR_PEN``

  The black pen used to clear the screen.

* ``simulateInput(screen, keys...)``

  This function wraps an undocumented native function that passes a set of
  keycodes to a screen, and is the official way to do that.

  Every argument after the initial screen may be *nil*, a numeric keycode,
  a string keycode, a sequence of numeric or string keycodes, or a mapping
  of keycodes to *true* or *false*. For instance, it is possible to use the
  table passed as argument to ``onInput``.

* ``mkdims_xy(x1,y1,x2,y2)``

  Returns a table containing the arguments as fields, and also ``width`` and
  ``height`` that contains the rectangle dimensions.

* ``mkdims_wh(x1,y1,width,height)``

  Returns the same kind of table as ``mkdims_xy``, only this time it computes
  ``x2`` and ``y2``.

* ``is_in_rect(rect,x,y)``

  Checks if the given point is within a rectangle, represented by a table produced
  by one of the ``mkdims`` functions.

* ``blink_visible(delay)``

  Returns *true* or *false*, with the value switching to the opposite every ``delay``
  msec. This is intended for rendering blinking interface objects.

* ``getKeyDisplay(keycode)``

  Wraps ``dfhack.screen.getKeyDisplay`` in order to allow using strings for the keycode argument.


ViewRect class
--------------

This class represents an on-screen rectangle with an associated independent
clip area rectangle. It is the base of the ``Painter`` class, and is used by
``Views`` to track their client area.

* ``ViewRect{ rect = ..., clip_rect = ..., view_rect = ..., clip_view = ... }``

  The constructor has the following arguments:

  :rect: The ``mkdims`` rectangle in screen coordinates of the logical viewport.
         Defaults to the whole screen.
  :clip_rect: The clip rectangle in screen coordinates. Defaults to ``rect``.
  :view_rect: A ViewRect object to copy from; overrides both ``rect`` and ``clip_rect``.
  :clip_view: A ViewRect object to intersect the specified clip area with.

* ``rect:isDefunct()``

  Returns *true* if the clip area is empty, i.e. no painting is possible.

* ``rect:inClipGlobalXY(x,y)``

  Checks if these global coordinates are within the clip rectangle.

* ``rect:inClipLocalXY(x,y)``

  Checks if these coordinates (specified relative to ``x1,y1``) are within the clip rectangle.

* ``rect:localXY(x,y)``

  Converts a pair of global coordinates to local; returns *x_local,y_local*.

* ``rect:globalXY(x,y)``

  Converts a pair of local coordinates to global; returns *x_global,y_global*.

* ``rect:viewport(x,y,w,h)`` or ``rect:viewport(subrect)``

  Returns a ViewRect representing a sub-rectangle of the current one.
  The arguments are specified in local coordinates; the ``subrect``
  argument must be a ``mkdims`` table. The returned object consists of
  the exact specified rectangle, and a clip area produced by intersecting
  it with the clip area of the original object.


Painter class
-------------

The painting natives in ``dfhack.screen`` apply to the whole screen, are
completely stateless and don't implement clipping.

The Painter class inherits from ViewRect to provide clipping and local
coordinates, and tracks current cursor position and current pen.

* ``Painter{ ..., pen = ..., key_pen = ... }``

  In addition to ViewRect arguments, Painter accepts a suggestion of
  the initial value for the main pen, and the keybinding pen. They
  default to COLOR_GREY and COLOR_LIGHTGREEN otherwise.

  There are also some convenience functions that wrap this constructor:

  - ``Painter.new(rect,pen)``
  - ``Painter.new_view(view_rect,pen)``
  - ``Painter.new_xy(x1,y1,x2,y2,pen)``
  - ``Painter.new_wh(x1,y1,width,height,pen)``

* ``painter:isValidPos()``

  Checks if the current cursor position is within the clip area.

* ``painter:viewport(x,y,w,h)``

  Like the superclass method, but returns a Painter object.

* ``painter:cursor()``

  Returns the current cursor *x,y* in local coordinates.

* ``painter:seek(x,y)``

  Sets the current cursor position, and returns *self*.
  Either of the arguments may be *nil* to keep the current value.

* ``painter:advance(dx,dy)``

  Adds the given offsets to the cursor position, and returns *self*.
  Either of the arguments may be *nil* to keep the current value.

* ``painter:newline([dx])``

  Advances the cursor to the start of the next line plus the given x offset, and returns *self*.

* ``painter:pen(...)``

  Sets the current pen to ``dfhack.pen.parse(old_pen,...)``, and returns *self*.

* ``painter:key_pen(...)``

  Sets the current keybinding pen to ``dfhack.pen.parse(old_pen,...)``, and returns *self*.

* ``painter:clear()``

  Fills the whole clip rectangle with ``CLEAR_PEN``, and returns *self*.

* ``painter:fill(x1,y1,x2,y2[,...])`` or ``painter:fill(rect[,...])``

  Fills the specified local coordinate rectangle with ``dfhack.pen.parse(cur_pen,...)``,
  and returns *self*.

* ``painter:char([char[, ...]])``

  Paints one character using ``char`` and ``dfhack.pen.parse(cur_pen,...)``; returns *self*.
  The ``char`` argument, if not nil, is used to override the ``ch`` property of the pen.

* ``painter:tile([char, tile[, ...]])``

  Like above, but also allows overriding the ``tile`` property on ad-hoc basis.

* ``painter:string(text[, ...])``

  Paints the string with ``dfhack.pen.parse(cur_pen,...)``; returns *self*.

* ``painter:key(keycode[, ...])``

  Paints the description of the keycode using ``dfhack.pen.parse(cur_key_pen,...)``; returns *self*.

As noted above, all painting methods return *self*, in order to allow chaining them like this::

  painter:pen(foo):seek(x,y):char(1):advance(1):string('bar')...


View class
----------

This class is the common abstract base of both the stand-alone screens
and common widgets to be used inside them. It defines the basic layout,
rendering and event handling framework.

The class defines the following attributes:

:visible: Specifies that the view should be painted.
:active: Specifies that the view should receive events, if also visible.
:view_id: Specifies an identifier to easily identify the view among subviews.
          This is reserved for implementation of top-level views, and should
          not be used by widgets for their internal subviews.

It also always has the following fields:

:subviews: Contains a table of all subviews. The sequence part of the
           table is used for iteration. In addition, subviews are also
           indexed under their *view_id*, if any; see ``addviews()`` below.

These fields are computed by the layout process:

:frame_parent_rect: The ViewRect represeting the client area of the parent view.
:frame_rect: The ``mkdims`` rect of the outer frame in parent-local coordinates.
:frame_body: The ViewRect representing the body part of the View's own frame.

The class has the following methods:

* ``view:addviews(list)``

  Adds the views in the list to the ``subviews`` sequence. If any of the views
  in the list have ``view_id`` attributes that don't conflict with existing keys
  in ``subviews``, also stores them under the string keys. Finally, copies any
  non-conflicting string keys from the ``subviews`` tables of the listed views.

  Thus, doing something like this::

    self:addviews{
        Panel{
            view_id = 'panel',
            subviews = {
                Label{ view_id = 'label' }
            }
        }
    }

  Would make the label accessible as both ``self.subviews.label`` and
  ``self.subviews.panel.subviews.label``.

* ``view:getWindowSize()``

  Returns the dimensions of the ``frame_body`` rectangle.

* ``view:getMousePos()``

  Returns the mouse *x,y* in coordinates local to the ``frame_body``
  rectangle if it is within its clip area, or nothing otherwise.

* ``view:updateLayout([parent_rect])``

  Recomputes layout of the view and its subviews. If no argument is
  given, re-uses the previous parent rect. The process goes as follows:

  1. Calls ``preUpdateLayout(parent_rect)`` via ``invoke_before``.
  2. Uses ``computeFrame(parent_rect)`` to compute the desired frame.
  3. Calls ``postComputeFrame(frame_body)`` via ``invoke_after``.
  4. Calls ``updateSubviewLayout(frame_body)`` to update children.
  5. Calls ``postUpdateLayout(frame_body)`` via ``invoke_after``.

* ``view:computeFrame(parent_rect)`` *(for overriding)*

  Called by ``updateLayout`` in order to compute the frame rectangle(s).
  Should return the ``mkdims`` rectangle for the outer frame, and optionally
  also for the body frame. If only one rectangle is returned, it is used
  for both frames, and the margin becomes zero.

* ``view:updateSubviewLayout(frame_body)``

  Calls ``updateLayout`` on all children.

* ``view:render(painter)``

  Given the parent's painter, renders the view via the following process:

  1. Calls ``onRenderFrame(painter, frame_rect)`` to paint the outer frame.
  2. Creates a new painter using the ``frame_body`` rect.
  3. Calls ``onRenderBody(new_painter)`` to paint the client area.
  4. Calls ``renderSubviews(new_painter)`` to paint visible children.

* ``view:renderSubviews(painter)``

  Calls ``render`` on all ``visible`` subviews in the order they
  appear in the ``subviews`` sequence.

* ``view:onRenderFrame(painter, rect)`` *(for overriding)*

  Called by ``render`` to paint the outer frame; by default does nothing.

* ``view:onRenderBody(painter)`` *(for overriding)*

  Called by ``render`` to paint the client area; by default does nothing.

* ``view:onInput(keys)`` *(for overriding)*

  Override this to handle events. By default directly calls ``inputToSubviews``.
  Return a true value from this method to signal that the event has been handled
  and should not be passed on to more views.

* ``view:inputToSubviews(keys)``

  Calls ``onInput`` on all visible active subviews, iterating the ``subviews``
  sequence in *reverse order*, so that topmost subviews get events first.
  Returns *true* if any of the subviews handled the event.


Screen class
------------

This is a View subclass intended for use as a stand-alone dialog or screen.
It adds the following methods:

* ``screen:isShown()``

  Returns *true* if the screen is currently in the game engine's display stack.

* ``screen:isDismissed()``

  Returns *true* if the screen is dismissed.

* ``screen:isActive()``

  Returns *true* if the screen is shown and not dismissed.

* ``screen:invalidate()``

  Requests a repaint. Note that currently using it is not necessary, because
  repaints are constantly requested automatically, due to issues with native
  screens happening otherwise.

* ``screen:renderParent()``

  Asks the parent native screen to render itself, or clears the screen if impossible.

* ``screen:sendInputToParent(...)``

  Uses ``simulateInput`` to send keypresses to the native parent screen.

* ``screen:show([parent])``

  Adds the screen to the display stack with the given screen as the parent;
  if parent is not specified, places this one one topmost. Before calling
  ``dfhack.screen.show``, calls ``self:onAboutToShow(parent)``.

* ``screen:onAboutToShow(parent)`` *(for overriding)*

  Called when ``dfhack.screen.show`` is about to be called.

* ``screen:onShow()``

  Called by ``dfhack.screen.show`` once the screen is successfully shown.

* ``screen:dismiss()``

  Dismisses the screen. A dismissed screen does not receive any more
  events or paint requests, but may remain in the display stack for
  a short time until the game removes it.

* ``screen:onDismiss()`` *(for overriding)*

  Called by ``dfhack.screen.dismiss()``.

* ``screen:onDestroy()`` *(for overriding)*

  Called by the native code when the screen is fully destroyed and removed
  from the display stack. Place code that absolutely must be called whenever
  the screen is removed by any means here.

* ``screen:onResize``, ``screen:onRender``

  Defined as callbacks for native code.


FramedScreen class
------------------

A Screen subclass that paints a visible frame around its body.
Most dialogs should inherit from this class.

A framed screen has the following attributes:

:frame_style: A table that defines a set of pens to draw various parts of the frame.
:frame_title: A string to display in the middle of the top of the frame.
:frame_width: Desired width of the client area. If *nil*, the screen will occupy the whole width.
:frame_height: Likewise, for height.
:frame_inset: The gap between the frame and the client area. Defaults to 0.
:frame_background: The pen to fill in the frame with. Defaults to CLEAR_PEN.

There are the following predefined frame style tables:

* ``GREY_FRAME``

  A plain grey-colored frame.

* ``BOUNDARY_FRAME``

  The same frame as used by the usual full-screen DF views, like dwarfmode.

* ``GREY_LINE_FRAME``

  A frame consisting of grey lines, similar to the one used by titan announcements.


gui.widgets
===========

This module implements some basic widgets based on the View infrastructure.

Widget class
------------

Base of all the widgets. Inherits from View and has the following attributes:

* ``frame = {...}``

  Specifies the constraints on the outer frame of the widget.
  If omitted, the widget will occupy the whole parent rectangle.

  The frame is specified as a table with the following possible fields:

  :l: gap between the left edges of the frame and the parent.
  :t: gap between the top edges of the frame and the parent.
  :r: gap between the right edges of the frame and the parent.
  :b: gap between the bottom edges of the frame and the parent.
  :w: maximum width of the frame.
  :h: maximum heigth of the frame.
  :xalign: X alignment of the frame.
  :yalign: Y alignment of the frame.

  First the ``l,t,r,b`` fields restrict the available area for
  placing the frame. If ``w`` and ``h`` are not specified or
  larger then the computed area, it becomes the frame. Otherwise
  the smaller frame is placed within the are based on the
  ``xalign/yalign`` fields. If the align hints are omitted, they
  are assumed to be 0, 1, or 0.5 based on which of the ``l/r/t/b``
  fields are set.

* ``frame_inset = {...}``

  Specifies the gap between the outer frame, and the client area.
  The attribute may be a simple integer value to specify a uniform
  inset, or a table with the following fields:

  :l: left margin.
  :t: top margin.
  :r: right margin.
  :b: bottom margin.
  :x: left/right margin, if ``l`` and/or ``r`` are omitted.
  :y: top/bottom margin, if ``t`` and/or ``b`` are omitted.

* ``frame_background = pen``

  The pen to fill the outer frame with. Defaults to no fill.

Panel class
-----------

Inherits from Widget, and intended for grouping a number of subviews.

Has attributes:

* ``subviews = {}``

  Used to initialize the subview list in the constructor.

* ``on_render = function(painter)``

  Called from ``onRenderBody``.

Pages class
-----------

Subclass of Panel; keeps exactly one child visible.

* ``Pages{ ..., selected = ... }``

  Specifies which child to select initially; defaults to the first one.

* ``pages:getSelected()``

  Returns the selected *index, child*.

* ``pages:setSelected(index)``

  Selects the specified child, hiding the previous selected one.
  It is permitted to use the subview object, or its ``view_id`` as index.

EditField class
---------------

Subclass of Widget; implements a simple edit field.

Attributes:

:text: The current contents of the field.
:text_pen: The pen to draw the text with.
:on_char: Input validation callback; used as ``on_char(new_char,text)``.
          If it returns false, the character is ignored.
:on_change: Change notification callback; used as ``on_change(new_text,old_text)``.
:on_submit: Enter key callback; if set the field will handle the key and call ``on_submit(text)``.

Label class
-----------

This Widget subclass implements flowing semi-static text.

It has the following attributes:

:text_pen: Specifies the pen for active text.
:text_dpen: Specifies the pen for disabled text.
:text_hpen: Specifies the pen for text hovered over by the mouse, if a click handler is registered.
:disabled: Boolean or a callback; if true, the label is disabled.
:enabled: Boolean or a callback; if false, the label is disabled.
:auto_height: Sets self.frame.h from the text height.
:auto_width: Sets self.frame.w from the text width.
:on_click: A callback called when the label is clicked (optional)
:on_rclick: A callback called when the label is right-clicked (optional)

The text itself is represented as a complex structure, and passed
to the object via the ``text`` argument of the constructor, or via
the ``setText`` method, as one of:

* A simple string, possibly containing newlines.
* A sequence of tokens.

Every token in the sequence in turn may be either a string, possibly
containing newlines, or a table with the following possible fields:

* ``token.text = ...``

  Specifies the main text content of a token, and may be a string, or
  a callback returning a string.

* ``token.gap = ...``

  Specifies the number of character positions to advance on the line
  before rendering the token.

* ``token.tile = pen``

  Specifies a pen to paint as one tile before the main part of the token.

* ``token.width = ...``

  If specified either as a value or a callback, the text field is padded
  or truncated to the specified number.

* ``token.pad_char = '?'``

  If specified together with ``width``, the padding area is filled with
  this character instead of just being skipped over.

* ``token.key = '...'``

  Specifies the keycode associated with the token. The string description
  of the key binding is added to the text content of the token.

* ``token.key_sep = '...'``

  Specifies the separator to place between the keybinding label produced
  by ``token.key``, and the main text of the token. If the separator is
  '()', the token is formatted as ``text..' ('..binding..')'``. Otherwise
  it is simply ``binding..sep..text``.

* ``token.enabled``, ``token.disabled``

  Same as the attributes of the label itself, but applies only to the token.

* ``token.pen``, ``token.dpen``

  Specify the pen and disabled pen to be used for the token's text.
  The field may be either the pen itself, or a callback that returns it.

* ``token.on_activate``

  If this field is not nil, and ``token.key`` is set, the token will actually
  respond to that key binding unless disabled, and call this callback. Eventually
  this may be extended with mouse click support.

* ``token.id``

  Specifies a unique identifier for the token.

* ``token.line``, ``token.x1``, ``token.x2``

  Reserved for internal use.

The Label widget implements the following methods:

* ``label:setText(new_text)``

  Replaces the text currently contained in the widget.

* ``label:itemById(id)``

  Finds a token by its ``id`` field.

* ``label:getTextHeight()``

  Computes the height of the text.

* ``label:getTextWidth()``

  Computes the width of the text.

List class
----------

The List widget implements a simple list with paging.

It has the following attributes:

:text_pen: Specifies the pen for deselected list entries.
:cursor_pen: Specifies the pen for the selected entry.
:inactive_pen: If specified, used for the cursor when the widget is not active.
:icon_pen: Default pen for icons.
:on_select: Selection change callback; called as ``on_select(index,choice)``.
            This is also called with *nil* arguments if ``setChoices`` is called
            with an empty list.
:on_submit: Enter key callback; if specified, the list reacts to the key
            and calls it as ``on_submit(index,choice)``.
:on_submit2: Shift-Enter key callback; if specified, the list reacts to the key
             and calls it as ``on_submit2(index,choice)``.
:row_height: Height of every row in text lines.
:icon_width: If not *nil*, the specified number of character columns
             are reserved to the left of the list item for the icons.
:scroll_keys: Specifies which keys the list should react to as a table.

Every list item may be specified either as a string, or as a lua table
with the following fields:

:text: Specifies the label text in the same format as the Label text.
:caption, [1]: Deprecated legacy aliases for **text**.
:text_*: Reserved for internal use.
:key: Specifies a keybinding that acts as a shortcut for the specified item.
:icon: Specifies an icon string, or a pen to paint a single character. May be a callback.
:icon_pen: When the icon is a string, used to paint it.

The list supports the following methods:

* ``List{ ..., choices = ..., selected = ... }``

  Same as calling ``setChoices`` after construction.

* ``list:setChoices(choices[, selected])``

  Replaces the list of choices, possibly also setting the currently selected index.

* ``list:setSelected(selected)``

  Sets the currently selected index. Returns the index after validation.

* ``list:getChoices()``

  Returns the list of choices.

* ``list:getSelected()``

  Returns the selected *index, choice*, or nothing if the list is empty.

* ``list:getContentWidth()``

  Returns the minimal width to draw all choices without clipping.

* ``list:getContentHeight()``

  Returns the minimal width to draw all choices without scrolling.

* ``list:submit()``

  Call the ``on_submit`` callback, as if the Enter key was handled.

* ``list:submit2()``

  Call the ``on_submit2`` callback, as if the Shift-Enter key was handled.

FilteredList class
------------------

This widget combines List, EditField and Label into a combo-box like
construction that allows filtering the list by subwords of its items.

In addition to passing through all attributes supported by List, it
supports:

:edit_pen: If specified, used instead of ``cursor_pen`` for the edit field.
:edit_below: If true, the edit field is placed below the list instead of above.
:not_found_label: Specifies the text of the label shown when no items match the filter.

The list choices may include the following attributes:

:search_key: If specified, used instead of **text** to match against the filter.

The widget implements:

* ``list:setChoices(choices[, selected])``

  Resets the filter, and passes through to the inner list.

* ``list:getChoices()``

  Returns the list of *all* choices.

* ``list:getFilter()``

  Returns the current filter string, and the *filtered* list of choices.

* ``list:setFilter(filter[,pos])``

  Sets the new filter string, filters the list, and selects the item at
  index ``pos`` in the *unfiltered* list if possible.

* ``list:canSubmit()``

  Checks if there are currently any choices in the filtered list.

* ``list:getSelected()``, ``list:getContentWidth()``, ``list:getContentHeight()``, ``list:submit()``

  Same as with an ordinary list.


=======
Plugins
=======

.. contents::
   :local:

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

.. _eventful:

Eventful
========

This plugin exports some events to lua thus allowing to run lua functions
on DF world events.

List of events
--------------

1. ``onReactionComplete(reaction,reaction_product,unit,input_items,input_reagents,output_items,call_native)``

   Auto activates if detects reactions starting with ``LUA_HOOK_``. Is called when reaction finishes.

2. ``onItemContaminateWound(item,unit,wound,number1,number2)``

   Is called when item tries to contaminate wound (e.g. stuck in).

3. ``onProjItemCheckMovement(projectile)``

   Is called when projectile moves.

4. ``onProjItemCheckImpact(projectile,somebool)``

   Is called when projectile hits something.

5. ``onProjUnitCheckMovement(projectile)``

   Is called when projectile moves.

6. ``onProjUnitCheckImpact(projectile,somebool)``

   Is called when projectile hits something.

7. ``onWorkshopFillSidebarMenu(workshop,callnative)``

   Is called when viewing a workshop in 'q' mode, to populate reactions, useful for custom viewscreens for shops.

8. ``postWorkshopFillSidebarMenu(workshop)``

   Is called after calling (or not) native fillSidebarMenu(). Useful for job button
   tweaking (e.g. adding custom reactions)

.. _EventManager:

Events from EventManager
------------------------
These events are straight from EventManager module. Each of them first needs to be enabled. See functions for more info. If you register a listener before the game is loaded, be aware that no events will be triggered immediately after loading, so you might need to add another event listener for when the game first loads in some cases.

1. ``onBuildingCreatedDestroyed(building_id)``

   Gets called when building is created or destroyed.

2. ``onConstructionCreatedDestroyed(building_id)``

   Gets called when construction is created or destroyed.

3. ``onJobInitiated(job)``

   Gets called when job is issued.

4. ``onJobCompleted(job)``

   Gets called when job is finished. The job that is passed to this function is a copy. Requires a frequency of 0 in order to distinguish between workshop jobs that were cancelled by the user and workshop jobs that completed successfully.

5. ``onUnitDeath(unit_id)``

   Gets called on unit death.

6. ``onItemCreated(item_id)``

   Gets called when item is created (except due to traders, migrants, invaders and spider webs).

7. ``onSyndrome(unit_id,syndrome_index)``

   Gets called when new syndrome appears on a unit.

8. ``onInvasion(invasion_id)``

   Gets called when new invasion happens.

9. ``onInventoryChange(unit_id,item_id,old_equip,new_equip)``

   Gets called when someone picks up an item, puts one down, or changes the way they are holding it. If an item is picked up, old_equip will be null. If an item is dropped, new_equip will be null. If an item is re-equipped in a new way, then neither will be null. You absolutely must NOT alter either old_equip or new_equip or you might break other plugins.

10. ``onReport(reportId)``

    Gets called when a report happens. This happens more often than you probably think, even if it doesn't show up in the announcements.

11. ``onUnitAttack(attackerId, defenderId, woundId)``

    Called when a unit wounds another with a weapon. Is NOT called if blocked, dodged, deflected, or parried.

12. ``onUnload()``

    A convenience event in case you don't want to register for every onStateChange event.

13. ``onInteraction(attackVerb, defendVerb, attackerId, defenderId, attackReportId, defendReportId)``

    Called when a unit uses an interaction on another.

Functions
---------

1. ``registerReaction(reaction_name,callback)``

   Simplified way of using onReactionComplete; the callback is function (same params as event).

2. ``removeNative(shop_name)``

   Removes native choice list from the building.

3. ``addReactionToShop(reaction_name,shop_name)``

   Add a custom reaction to the building.

4. ``enableEvent(evType,frequency)``

   Enable event checking for EventManager events. For event types use ``eventType`` table. Note that different types of events require different frequencies to be effective. The frequency is how many ticks EventManager will wait before checking if that type of event has happened. If multiple scripts or plugins use the same event type, the smallest frequency is the one that is used, so you might get events triggered more often than the frequency you use here.

5. ``registerSidebar(shop_name,callback)``

   Enable callback when sidebar for ``shop_name`` is drawn. Usefull for custom workshop views e.g. using gui.dwarfmode lib. Also accepts a ``class`` instead of function
   as callback. Best used with ``gui.dwarfmode`` class ``WorkshopOverlay``.

Examples
--------
Spawn dragon breath on each item attempt to contaminate wound::

    b=require "plugins.eventful"
    b.onItemContaminateWound.one=function(item,unit,un_wound,x,y)
        local flw=dfhack.maps.spawnFlow(unit.pos,6,0,0,50000)
    end

Reaction complete example::

  b=require "plugins.eventful"

  b.registerReaction("LUA_HOOK_LAY_BOMB",function(reaction,unit,in_items,in_reag,out_items,call_native)
    local pos=copyall(unit.pos)
    -- spawn dragonbreath after 100 ticks
    dfhack.timeout(100,"ticks",function() dfhack.maps.spawnFlow(pos,6,0,0,50000) end)
    --do not call real item creation code
    call_native.value=false
  end)

Grenade example::

  b=require "plugins.eventful"
  b.onProjItemCheckImpact.one=function(projectile)
    -- you can check if projectile.item e.g. has correct material
    dfhack.maps.spawnFlow(projectile.cur_pos,6,0,0,50000)
  end

Integrated tannery::

  b=require "plugins.eventful"
  b.addReactionToShop("TAN_A_HIDE","LEATHERWORKS")

.. _building-hacks:

Building-hacks
==============

This plugin overwrites some methods in workshop df class so that mechanical workshops are possible. Although
plugin export a function it's recommended to use lua decorated function.

Functions
---------

``registerBuilding(table)`` where table must contain name, as a workshop raw name, the rest are optional:

    :name:
        custom workshop id e.g. ``SOAPMAKER``

        .. note:: this is the only mandatory field.

    :fix_impassible:
        if true make impassible tiles impassible to liquids too
    :consume:
        how much machine power is needed to work.
        Disables reactions if not supplied enough and ``needs_power==1``
    :produce:
        how much machine power is produced.
    :needs_power:
        if produced in network < consumed stop working, default true
    :gears:
        a table or ``{x=?,y=?}`` of connection points for machines.
    :action:
        a table of number (how much ticks to skip) and a function which
        gets called on shop update
    :animate:
        a table of frames which can be a table of:

        a. tables of 4 numbers ``{tile,fore,back,bright}`` OR
        b. empty table (tile not modified) OR
        c. ``{x=<number> y=<number> + 4 numbers like in first case}``,
           this generates full frame useful for animations that change little (1-2 tiles)

    :canBeRoomSubset:
        a flag if this building can be counted in room. 1 means it can, 0 means it can't and -1 default building behaviour
    :auto_gears:
        a flag that automatically fills up gears and animate. It looks over building definition for gear icons and maps them.

    Animate table also might contain:

    :frameLength:
        how many ticks does one frame take OR
    :isMechanical:
        a bool that says to try to match to mechanical system (i.e. how gears are turning)

``getPower(building)`` returns two number - produced and consumed power if building can be modified and returns nothing otherwise

``setPower(building,produced,consumed)`` sets current productiona and consumption for a building.

Examples
--------

Simple mechanical workshop::

  require('plugins.building-hacks').registerBuilding{name="BONE_GRINDER",
    consume=15,
    gears={x=0,y=0}, --connection point
    animate={
      isMechanical=true, --animate the same conn. point as vanilla gear
      frames={
      {{x=0,y=0,42,7,0,0}}, --first frame, 1 changed tile
      {{x=0,y=0,15,7,0,0}} -- second frame, same
      }
    }

Or with auto_gears::

  require('plugins.building-hacks').registerBuilding{name="BONE_GRINDER",
    consume=15,
    auto_gears=true
    }

Luasocket
=========

A way to access csocket from lua. The usage is made similar to luasocket in vanilla lua distributions. Currently
only subset of functions exist and only tcp mode is implemented.

Socket class
------------

This is a base class for ``client`` and ``server`` sockets. You can not create it - it's like a virtual
base class in c++.


* ``socket:close()``

  Closes the connection.

* ``socket:setTimeout(sec,msec)``

  Sets the operation timeout for this socket. It's possible to set timeout to 0. Then it performs like
  a non-blocking socket.

Client class
------------

Client is a connection socket to a server. You can get this object either from ``tcp:connect(address,port)`` or
from ``server:accept()``. It's a subclass of ``socket``.

* ``client:receive(pattern)``

  Receives data.  Pattern is one of:

  :``*l``:      read one line (default, if pattern is *nil*)
  :<number>:    read specified number of bytes
  :``*a``:      read all available data

* ``client:send(data)``

  Sends data. Data is a string.


Server class
------------

Server is a socket that is waiting for clients.
You can get this object from ``tcp:bind(address,port)``.

* ``server:accept()``

  Accepts an incoming connection if it exists.
  Returns a ``client`` object representing that socket.

Tcp class
---------

A class with all the tcp functionality.

* ``tcp:bind(address,port)``

  Starts listening on that port for incoming connections. Returns ``server`` object.

* ``tcp:connect(address,port)``

  Tries connecting to that address and port. Returns ``client`` object.


=======
Scripts
=======

.. contents::
   :local:

Any files with the .lua extension placed into :file:`hack/scripts/*`
are automatically used by the DFHack core as commands. The
matching command name consists of the name of the file without
the extension. First DFHack searches for the script in the :file:`<save_folder>/raw/scripts/` folder. If it is not found there, it searches in the :file:`<DF>/raw/scripts/` folder. If it is not there, it searches in
:file:`<DF>/hack/scripts/`. If it is not there, it gives up.

If the first line of the script is a one-line comment, it is
used by the built-in ``ls`` and ``help`` commands.
Such a comment is required for every script in the official DFHack repository.

.. note::
    Scripts placed in subdirectories still can be accessed, but
    do not clutter the `ls` command list (unless ``ls -a``; thus it is preferred
    for obscure developer-oriented scripts and scripts used by tools.
    When calling such scripts, always use '/' as the separator for
    directories, e.g. ``devel/lua-example``.

Scripts are re-read from disk if they have changed since the last time they were read.
Global variable values persist in memory between calls, unless the file has changed.
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

* ``dfhack.script_environment(name)``

  Run an Lua script and return its environment.
  This command allows you to use scripts like modules for increased portability.
  It is highly recommended that if you are a modder you put your custom modules in ``raw/scripts`` and use ``script_environment`` instead of ``require`` so that saves with your mod installed will be self-contained and can be transferred to people who do have DFHack but do not have your mod installed.

  You can say ``dfhack.script_environment('add-thought').addEmotionToUnit([arguments go here])`` and it will have the desired effect.
  It will call the script in question with the global ``moduleMode`` set to ``true`` so that the script can return early.
  This is useful because if the script is called from the console it should deal with its console arguments and if it is called by ``script_environment`` it should only create its global functions and return.
  You can also access global variables with, for example ``print(dfhack.script_environment('add-thought').validArgs)``

  The function ``script_environment`` is fast enough that it is recommended that you not store its result in a nonlocal variable, because your script might need to load a different version of that script if the save is unloaded and a save with a different mod that overrides the same script with a slightly different functionality is loaded.
  This will not be an issue in most cases.

  This function also permits circular dependencies of scripts.

* ``dfhack.reqscript(name)`` or ``reqscript(name)``

  Nearly identical to script_environment() but requires scripts being loaded to
  include a line similar to::

      --@ module = true

  This is intended to only allow scripts that take appropriate action when used
  as a module to be loaded.

Enabling and disabling scripts
==============================

Scripts can choose to recognize the built-in ``enable`` and ``disable`` commands
by including the following line anywhere in their file::

    --@ enable = true

When the ``enable`` and ``disable`` commands are invoked, a ``dfhack_flags``
table will be passed to the script with the following fields set:

* ``enable``: Always true if the script is being enabled *or* disabled
* ``enable_state``: True if the script is being enabled, false otherwise

Save init script
================

If a save directory contains a file called ``raw/init.lua``, it is
automatically loaded and executed every time the save is loaded.
The same applies to any files called ``raw/init.d/*.lua``. Every
such script can define the following functions to be called by dfhack:

* ``function onStateChange(op) ... end``

  Automatically called from the regular onStateChange event as long
  as the save is still loaded. This avoids the need to install a hook
  into the global ``dfhack.onStateChange`` table, with associated
  cleanup concerns.

* ``function onUnload() ... end``

  Called when the save containing the script is unloaded. This function
  should clean up any global hooks installed by the script. Note that
  when this is called, the world is already completely unloaded.

Within the init script, the path to the save directory is available as ``SAVE_PATH``.
