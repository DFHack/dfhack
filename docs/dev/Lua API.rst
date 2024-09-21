.. highlight:: lua

.. _lua-api:

########################
DFHack Lua API Reference
########################

DFHack has extensive support for
the Lua_ scripting language, providing access to:

.. _Lua: https://www.lua.org

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


.. contents:: Contents
  :local:
  :depth: 2

.. _lua-df:

=========================
DF data structure wrapper
=========================

.. contents::
   :local:

Data structures of the game are defined in XML files located in :file:`library/xml`
(and `online <https://github.com/DFHack/df-structures>`_, and automatically exported
to lua code as a tree of objects and functions under the ``df`` global, which
also broadly maps to the ``df`` namespace in the headers generated for C++.

.. warning::

    The wrapper provides almost raw access to the memory of the game, so
    mistakes in manipulating objects are as likely to crash the game as
    equivalent plain C++ code would be - e.g., null pointer access is safely
    detected, but dangling pointers aren't.

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

  Destroys the object with the C++ ``delete`` operator. If the destructor is not
  available, returns *false*. (This typically only occurs when trying to delete
  an instance of a DF class with virtual methods whose vtable address has not
  been found; it is impossible for ``delete()`` to determine the validity of
  ``ref``.)

  .. warning::
    ``ref`` **must** be an object allocated with ``new``, like in C++. Calling
    ``obj.field:delete()`` where ``obj`` was allocated with ``new`` will not
    work. After ``delete()`` returns, ``ref`` remains as a dangling pointer,
    like a raw C++ pointer would. Any accesses to ``ref`` after ``ref:delete()``
    has been called are undefined behavior.

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

They behave as structs with a ``value`` field of the right type. If the
object's XML definition has a ``ref-target`` attribute, they will also have
a read-only ``ref_target`` field set to the corresponding type object.

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

  Primitive typed fields, i.e., numbers & strings, are converted
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

* ``ref:_field(field)``

  Returns a reference to a valid field. That is, unlike regular
  subscript, it returns a reference to the field within the structure
  even for primitive typed fields and pointers. Fails with an error
  if the field is not found.

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

* ``ref:_field(index)``

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

Additionally, bitfields have a ``whole`` property,
which returns the value of the bitfield as an
integer.

Since currently there is no API to allocate a bitfield
object fully in GC-managed lua heap, consider using the
lua table assignment feature outlined below in order to
pass bitfield values to dfhack API functions that need
them, e.g., ``matinfo:matches{metal=true}``.


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
  ``DFHack::type_identity`` object.

All compound types (structs, classes, unions, and the global object) support:

* ``type._fields``

  Contains a table mapping field names to descriptions of the type's fields,
  including data members and functions. Iterating with ``pairs()`` returns data
  fields in the order they are defined in the type. Functions and globals may
  appear in an arbitrary order.

  Each entry contains the following fields:

  * ``name``: the name of the field (matches the ``_fields`` table key)
  * ``offset``: for data members, the position of the field relative to the start of the type, in bytes
  * ``count``: for arrays, the number of elements
  * ``mode``: implementation detail. See ``struct_field_info::Mode`` in ``DataDefs.h``.

  Each entry may also contain the following fields, depending on its type:

  * ``type_name``: present for most fields; a string representation of the field's type
  * ``type``: the type object matching the field's type; present if such an object exists
    (e.g., present for DF types, absent for primitive types)
  * ``type_identity``: present for most fields; a lightuserdata pointing to the field's underlying ``DFHack::type_identity`` object
  * ``index_enum``, ``ref_target``: the type object corresponding to the field's similarly-named XML attribute, if present
  * ``union_tag_field``, ``union_tag_attr``, ``original_name``: the string value of the field's similarly-named XML attribute, if present

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

Enum types support the following:

* ``type.next_item(index)``

  Returns the next valid numeric value of the enum. It  returns the
  first enum value if ``index`` is greater than or equal to the max
  enum value.

* ``type.attrs``

  A mapping of enum keys (usually integers) and values (usually strings) to
  their attributes. e.g ``df.goal_type.attrs.STAY_ALIVE`` returns
  ``{ short_name: "Stay Alive", achieved_short_name: "Stayed Alive" } }``

* ``type._attr_entry_type``

  Returns the named ``struct-type`` type representing the table returned
  by ``type.attrs``.

In addition to this, enum and bitfield types contain a
bi-directional mapping between key strings and values, and
also map ``_first_item`` and ``_last_item`` to the min and
max values.

Struct and class types with an instance-vector attribute in the XML also support:

* ``type.find(key)``

  Returns an object from the instance vector that matches the key, where the
  field is determined by the 'key-field' specified in the XML.

* ``type.get_vector()``

  Returns the instance vector e.g ``df.item.get_vector() == df.global.world.items.all``

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
to a C++ object or field, i.e., one of:

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

* ``dfhack.getCommandHistory(history_id, history_filename)``

  Returns the list of strings in the specified history. Intended to be used by
  GUI scripts that don't have access to a console and so can't use
  ``dfhack.lineedit``. The ``history_id`` parameter is some unique string that
  the script uses to identify its command history, such as the script's name. If
  this is the first time the history with the given ``history_id`` is being
  accessed, it is initialized from the given file.

* ``dfhack.addCommandToHistory(history_id, history_filename, command)``

  Adds a command to the specified history and saves the updated history to the
  specified file.

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
  both from the curry call and the closure call itself. I.e.,
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

.. _persistent-api:

Persistent configuration storage
--------------------------------

This api is intended for storing tool state in the world savegame directory. It
is intended for data that is world-dependent. Global state that is independent
of the loaded world should be saved into a separate file named after the tool
in the ``dfhack-config/`` directory.

Entries are associated with the current loaded site (fortress) and are
identified by a string ``key``. The data will still be associated with a fort
if the fort is retired and then later unretired. Entries are stored as
serialized strings, but there are convenience functions for working with
arbitrary Lua tables.

* ``dfhack.persistent.getSiteData(key[, default])``

  Retrieves the Lua table associated with the current site and the given string
  ``key``. If ``default`` is supplied, then it is returned if the key isn't
  found in the current site's persistent data.

  Example usage::

    local state = dfhack.persistent.getSiteData('my-script-name', {somedata={}})

* ``dfhack.persistent.getSiteDataString(key)``

  Retrieves the underlying serialized string associated with the current site
  and the given string ``key``. Returns *nil* if the key isn't found in the
  current site's persistent data. Most scripts will want to use ``getSiteData``
  instead.

* ``dfhack.persistent.saveSiteData(key, data)``

  Persists the given ``data`` (usually a table; can be of arbitrary complexity and depth)
  in the world save, associated with the current site and the given ``key``.

* ``dfhack.persistent.saveSiteDataString(key, data_str)``

  Persists the given string in the world save, associated with the current site
  and the given ``key``.

* ``dfhack.persistent.deleteSiteData(key)``

  Removes the existing entry associated with the current site and the given
  ``key``. Returns *true* if succeeded.

* ``dfhack.persistent.getWorldData(key[, default])``
* ``dfhack.persistent.getWorldDataString(key)``
* ``dfhack.persistent.saveWorldData(key, data)``
* ``dfhack.persistent.saveWorldDataString(key, data_str)``
* ``dfhack.persistent.deleteWorldData(key)``

  Same semantics as for the ``Site`` functions, but will associated the data
  with the global world context.

The data is kept in memory, so no I/O occurs when getting or saving keys. It is
all written to a json file in the game save directory when the game is saved.

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

  Looks up material info for the given number pair; if not found, returns *nil*.

* ``....decode(matinfo|item|plant|obj)``

  Uses type-specific methods for retrieving the code pair.

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


.. _lua-cpp-func-wrappers:

C++ function wrappers
=====================

.. contents::
   :local:

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
* ``dfhack.getDFHackBuildID()``
* ``dfhack.getCompiledDFVersion()``
* ``dfhack.getGitDescription()``
* ``dfhack.getGitCommit()``
* ``dfhack.getGitXmlCommit()``
* ``dfhack.getGitXmlExpectedCommit()``
* ``dfhack.gitXmlMatch()``
* ``dfhack.isRelease()``
* ``dfhack.isPrerelease()``

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

  Returns the dfhack directory path, i.e., ``".../df/hack/"``.

* ``dfhack.getSavePath()``

  Returns the path to the current save directory, or *nil* if no save loaded.

* ``dfhack.getTickCount()``

  Returns the tick count in ms, exactly as DF ui uses.

* ``dfhack.isWorldLoaded()``

  Checks if the world is loaded.

* ``dfhack.isMapLoaded()``

  Checks if the world and map are loaded.

* ``dfhack.isSiteLoaded()``

  Checks if a site (e.g., a player fort) is loaded.

* ``dfhack.TranslateName(name[,in_english[,only_last_name]])``

  Convert a language_name or only the last name part to string.

* ``dfhack.df2utf(string)``

  Convert a string from DF's CP437 encoding to UTF-8.

* ``dfhack.df2console()``

  Convert a string from DF's CP437 encoding to the correct encoding for the
  DFHack console.

.. warning::

  When printing CP437-encoded text to the console (for example, names returned
  from ``dfhack.TranslateName()``), use ``print(dfhack.df2console(text))`` to
  ensure proper display on all platforms.

* ``dfhack.utf2df(string)``

  Convert a string from UTF-8 to DF's CP437 encoding.

* ``dfhack.upperCp437(string)``

  Return a version of the string with all letters capitalized.
  Non-ASCII CP437 characters are capitalized if a CP437 version exists.
  For example, ``ä`` is replaced by ``Ä``, but ``â`` is never capitalized.


* ``dfhack.lowerCp437(string)``

  Return a version of the string with all letters in lower case.
  Non-ASCII CP437 characters are downcased. For example, ``Ä`` is replaced by ``ä``.

* ``dfhack.toSearchNormalized(string)``

  Replace non-ASCII alphabetic characters in a CP437-encoded string with their
  nearest ASCII equivalents, if possible, and returns a CP437-encoded string.
  Note that the returned string may be longer than the input string. For
  example, ``ä`` is replaced with ``a``, and ``æ`` is replaced with ``ae``.

* ``dfhack.capitalizeStringWords(string)``

  Return a version of the string with the first letter of each word capitalized.
  The beginning of a word is determined by a space or quote ``"``. It is also
  determined by an apostrophe ``'`` when preceded by a space or comma.
  Non-ASCII CP437 characters will be capitalized if a CP437 version exists.
  This function does not downcase characters. Use ``dfhack.lowerCp437``
  first, if desired.

* ``dfhack.formatInt(num)``

  Formats an integer value as a string according to the current system locale.
  E.g., for American English, it would transform like: ``12345`` ->
  ``'12,345'``

* ``dfhack.formatFloat(num)``

  Formats a floating point value as a string according to the current system
  locale. E.g., for American English, it would transform like: ``-12345.6789``
  -> ``'-12,345.678711'`` (because float imprecision).

* ``dfhack.run_command(command[, ...])``

  Run an arbitrary DFHack command, with the core suspended, and send output to
  the DFHack console. The command can be passed as a table, multiple string
  arguments, or a single string argument (not recommended - in this case, the
  usual DFHack console tokenization is used).

  A ``command_result`` constant starting with ``CR_`` is returned, where ``CR_OK``
  indicates success.

  The following examples are equivalent::

    dfhack.run_command({'ls', 'quick'})
    dfhack.run_command('ls', 'quick')
    dfhack.run_command('ls quick')  -- not recommended

* ``dfhack.run_command_silent(command[, ...])``

  Similar to ``run_command()``, but instead of printing to the console,
  returns an ``output, command_result`` pair. ``output`` is a single string -
  see ``dfhack.internal.runCommand()`` to obtain colors as well.

Gui module
----------

Screens
~~~~~~~

* ``dfhack.gui.getCurViewscreen([skip_dismissed])``

  Returns the topmost viewscreen. If ``skip_dismissed`` is *true*,
  ignores screens already marked to be removed.

* ``dfhack.gui.getFocusStrings(viewscreen)``

  Returns a table of string representations of the current UI focuses.
  The strings have a "screen/foo/bar/baz..." format e.g.::

    [1] = "dwarfmode/Info/CREATURES/CITIZEN"
    [2] = "dwarfmode/Squads"

* ``dfhack.gui.matchFocusString(focus_string[, viewscreen])``

  Returns ``true`` if the given ``focus_string`` is found in the current
  focus strings, or as a prefix to any of the focus strings, or ``false``
  if no match is found. Matching is case insensitive. If ``viewscreen`` is
  specified, gets the focus strings to match from the given viewscreen.

* ``dfhack.gui.getCurFocus([skip_dismissed])``

  Returns the focus string of the current viewscreen.

* ``dfhack.gui.getViewscreenByType(type[, depth])``

  Returns the topmost viewscreen out of the top ``depth`` viewscreens with
  the specified type (e.g., ``df.viewscreen_titlest``), or ``nil`` if none match.
  If ``depth`` is not specified or is less than 1, all viewscreens are checked.

* ``dfhack.gui.getDFViewscreen([skip_dismissed[, viewscreen]])``

  Returns the topmost viewscreen not owned by DFHack. If ``skip_dismissed`` is
  ``true``, ignores screens already marked to be removed. If ``viewscreen`` is
  specified, starts the scan at the given viewscreen.

* ``dfhack.gui.getWidget(container, <name or index>[, <name or index>...])``

  Returns the DF widget in the given widget container with the given name or
  (zero-based) numeric index. You can follow a chain of widget containers by
  passing additional names or indices. For example:
  ``:lua ~dfhack.gui.getWidget(game.main_interface.info.labor, "Tabs", 0)``

* ``dfhack.gui.getWidgetChildren(container)``

  Returns all the DF widgets in the given widget container.

General-purpose selections
~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``dfhack.gui.getSelectedWorkshopJob([silent])``
* ``dfhack.gui.getSelectedJob([silent])``
* ``dfhack.gui.getSelectedUnit([silent])``
* ``dfhack.gui.getSelectedItem([silent])``
* ``dfhack.gui.getSelectedBuilding([silent])``
* ``dfhack.gui.getSelectedCivZone([silent])``
* ``dfhack.gui.getSelectedStockpile([silent])``
* ``dfhack.gui.getSelectedPlant([silent])``

  Returns the currently selected in-game object or the indicated thing
  associated with the selected in-game object. For example, Calling
  ``getSelectedJob`` when a building is selected will return the job associated
  with the building (e.g., the ``ConstructBuilding`` job). If ``silent`` is
  omitted or set to ``false`` and a selected object cannot be found, then an
  error is printed to the console.

* ``dfhack.gui.getAnyWorkshopJob(screen)``
* ``dfhack.gui.getAnyJob(screen)``
* ``dfhack.gui.getAnyUnit(screen)``
* ``dfhack.gui.getAnyItem(screen)``
* ``dfhack.gui.getAnyBuilding(screen)``
* ``dfhack.gui.getAnyCivZone(screen)``
* ``dfhack.gui.getAnyStockpile(screen)``
* ``dfhack.gui.getAnyPlant(screen)``

  Similar to the corresponding ``getSelected`` functions, but operate on the
  given screen instead of the current screen and always return ``nil`` silently
  on failure.

Fortress mode
~~~~~~~~~~~~~

* ``dfhack.gui.getDwarfmodeViewDims()``

  Returns dimensions of the displayed map viewport. See ``getPanelLayout()``
  in the ``gui.dwarfmode`` module for a more Lua-friendly version.

* ``dfhack.gui.resetDwarfmodeView([pause])``

  Resets the fortress mode sidebar menus and cursors to their default state. If
  ``pause`` is true, also pauses the game.

* ``dfhack.gui.pauseRecenter(pos[,pause])``
  ``dfhack.gui.pauseRecenter(x,y,z[,pause])``

  Same as ``resetDwarfmodeView``, but also recenter if position is valid. If
  ``pause`` is false, skip pausing. Respects ``RECENTER_INTERFACE_SHUTDOWN_MS``
  in DF's ``init.txt`` (the delay before input is recognized when a recenter
  occurs.)

* ``dfhack.gui.revealInDwarfmodeMap(pos[,center[,highlight]])``
  ``dfhack.gui.revealInDwarfmodeMap(x,y,z[,center[,highlight]])``

  Centers the view on the given coordinates. If ``center`` is true, make sure
  the position is in the exact center of the view, else just bring it on screen.
  If ``highlight`` is true, then mark the target tile with a pulsing highlight
  until the player clicks somewhere else.

  ``pos`` can be a ``df.coord`` instance or a table assignable to a ``df.coord``
  (see `lua-api-table-assignment`),
  e.g.::

    {x = 5, y = 7, z = 11}
    getSelectedUnit().pos
    copyall(df.global.cursor)

  If the position is invalid, the function will simply ensure the current
  window position is clamped between valid values.

* ``dfhack.gui.refreshSidebar()``

  Refreshes the fortress mode sidebar. This can be useful when making changes to
  the map, for example, because DF only updates the sidebar when the cursor
  position changes.

* ``dfhack.gui.inRenameBuilding()``

  Returns ``true`` if a building is being renamed.

Announcements
~~~~~~~~~~~~~

* ``dfhack.gui.writeToGamelog(text)``

  Writes a string to :file:`gamelog.txt` without doing an announcement.

* ``dfhack.gui.makeAnnouncement(type,flags,pos,text[,color[,is_bright]])``

  Adds an announcement with given announcement_type, text, color, and brightness.

  The announcement is written to :file:`gamelog.txt`. The announcement_flags
  argument provides a custom set of :file:`announcements.txt` options,
  which specify if the message should actually be displayed in the
  announcement list, and whether to recenter or show a popup.

  Returns the index of the new announcement in ``df.global.world.status.reports``, or -1.

* ``dfhack.gui.addCombatReport(unit,slot,report_index[,update_alert])``

  Adds the report with the given index (returned by makeAnnouncement)
  to the specified group of the given unit. If ``update_alert`` is ``true``,
  an alert badge will appear on the left side of the screen if not already visible.
  Returns ``true`` on success.

* ``dfhack.gui.addCombatReportAuto(unit,flags,report_index)``

  Adds the report with the given index to the appropriate group(s) of the given unit
  based on the unit's current job and as requested by the flags.
  Always updates alert badges. Returns ``true`` on any success.

* ``dfhack.gui.showAnnouncement(text[,color[,is_bright]])``

  Adds a regular announcement with given text, color, and brightness.
  The announcement type is always ``df.announcement_type.REACHED_PEAK``,
  which uses the alert badge for ``df.announcement_alert_type.GENERAL``.

* ``dfhack.gui.showZoomAnnouncement(type,pos,text[,color[,is_bright]])``

  Like above, but also specifies a position you can zoom to from the announcement menu,
  as well as being able to set the announcement type.

* ``dfhack.gui.showPopupAnnouncement(text[,color[,is_bright]])``

  Displays a megabeast-style modal announcement window.
  DF is currently ignoring the color and brightness settings
  (see: `bug report <https://dwarffortressbugtracker.com/view.php?id=12672>`_.)
  Add ``[C:`` color ``:0:`` bright ``]`` (where color is 0-7 and bright is 0-1)
  in front of your text string to force the popup text to be colored.

  Text is run through a parser as it is converted into a markup text box.
  The parser accepts tokens in square brackets (``[`` ``]``.)
  Use ``[[`` and ``]]`` to include actual square brackets in text.

  The following tokens are accepted:

  ``[R]``: (NEW_LINE) Ends the current line and begins on the next.

  ``[B]``: (BLANK_LINE) Ends the current line and adds an additional blank line,
  beginning on the line after that.

  ``[P]``: (INDENT) Ends the current line and begins four spaces indented on the next.

  ``[CHAR:`` n ``]``, ``[CHAR:~`` ch ``]``: Add a single character. First version
  takes a base-10 integer ``n`` representing a CP437 character.
  Second version accepts a character ``ch`` instead. ``"[CHAR:154]"`` and
  ``"[CHAR:~"..string.char(154).."]"`` both result in ``Ü``. Use ``[CHAR:32]`` or
  ``[CHAR:~ ]`` to add extra spaces, which would normally be trimmed by the parser.

  ``[LPAGE:`` link_type ``:`` id ``]``, ``[LPAGE:`` link_type ``:`` id ``:`` subid ``]``:
  Start a ``markup_text_linkst``. These are intended for Legends mode page links and
  don't work in popups. The text will just be colored based on ``link_type``.
  Valid link types are: ``HF`` (``HIST_FIG``,) ``SITE``, ``ARTIFACT``, ``BOOK``,
  ``SR`` (``SUBREGION``,) ``FL`` (``FEATURE_LAYER``,) ``ENT`` (``ENTITY``,)
  ``AB`` (``ABSTRACT_BUILDING``,) ``EPOP`` (``ENTITY_POPULATION``,) ``ART_IMAGE``,
  ``ERA``, ``HEC``.
  ``subid`` is only used for ``AB`` and ``ART_IMAGE``. ``[/LPAGE]`` ends the link text.

  ``[C:`` screenf ``:`` screenb ``:`` screenbright ``]``: Color text. Sets the
  respective values in ``df.global.gps`` and then sets text color.
  ``color`` = ``screenf``, ``bright`` = ``screenbright``, ``screenb`` does nothing
  since popup backgrounds are always black.
  Example: ``"Light gray, [C:4:0:0]red, [C:4:0:1]orange, [C:7:0:0]light gray."``

  ``[KEY:`` n ``]``: Keybinding. Shows the (first) keybinding for the
  ``df.interface_key`` ``n``. The keybinding will be displayed in light green, but
  the previous text color will be restored afterwards.

* ``dfhack.gui.showAutoAnnouncement(type,pos,text[,color[,is_bright[,unit_a[,unit_d]]]])``

  Uses the type to look up options from announcements.txt, and calls the above
  operations accordingly. The units are used to call ``addCombatReportAuto``.

* ``dfhack.gui.autoDFAnnouncement(report,text)``
  ``dfhack.gui.autoDFAnnouncement(type,pos,text[,color[,is_bright[,unit_a[,unit_d[,is_sparring]]]]])``

  Takes a ``df.announcement_infost`` (see:
  `structure definition <https://github.com/DFHack/df-structures/blob/master/df.announcements.xml>`_)
  and a string and processes them just like DF does. Can also be built from parameters instead of
  an ``announcement_infost``. Setting ``is_sparring`` to ``true`` means the report will be added
  to sparring logs (if applicable) rather than hunting or combat.

  The announcement will not display if units are involved and the player can't see
  them (or hear, for adventure mode sound announcement types.)
  Returns ``true`` if a report was created or repeated.
  For detailed info on why an announcement failed to display, enable
  ``debugfilter set Debug core gui`` in the DFHack console.
  If you want a guaranteed announcement, use ``dfhack.gui.showAutoAnnouncement`` instead.

* ``dfhack.gui.getMousePos([allow_out_of_bounds])``

  Returns the map coordinates of the map tile the mouse is over as a table of
  ``{x, y, z}``. If the cursor is not over a valid tile, returns ``nil``. To
  allow the function to return coordinates outside of the map, set
  ``allow_out_of_bounds`` to ``true``.

Other
~~~~~

* ``dfhack.gui.getDepthAt(x, y)``

  Returns the distance from the z-level of the tile at map coordinates (x, y) to
  the closest rendered ground z-level below. Defaults to 0, unless overridden by
  plugins.

Job module
----------

* ``dfhack.job.cloneJobStruct(job)``

  Creates a deep copy of the given job.

* ``dfhack.job.printJobDetails(job)``

  Prints info about the job.

* ``dfhack.job.printItemDetails(jobitem,idx)``

  Prints info about the job item.

* ``dfhack.job.removeJob(job)``

  Cancels a job, cleans up all references to it, and removes it from the world.

* ``dfhack.job.addGeneralRef(job, type, id)``

  Create a general reference of the given type, pointing to the object with the
  specified id, and add the created general reference to the provided job.

* ``dfhack.job.getGeneralRef(job, type)``

  Searches for a general_ref with the given type.

* ``dfhack.job.getSpecificRef(job, type)``

  Searches for a specific_ref with the given type.

* ``dfhack.job.getHolder(job)``

  Returns the building holding the job.

* ``dfhack.job.getWorker(job)``

  Returns the unit performing the job.

* ``dfhack.job.setJobCooldown(building,worker,cooldown)``

  Prevent the worker from taking jobs at the specified workshop for the
  specified cooldown period (in ticks). This doesn't decrease the cooldown
  period in any circumstances.

* ``dfhack.job.addWorker(job, unit)``

  Assign the specified job to the provided unit, unless the unit already has an
  active job. Also cleans up a potential job posting for the provided job.

* ``dfhack.job.removeWorker(job,cooldown)``

  Removes the worker from the specified workshop job, and sets the cooldown
  period (using the same logic as ``setJobCooldown``). Returns *true* on
  success.

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

* ``dfhack.job.attachJobItem(job, item, role, filter_idx, insert_idx)``

  Attach a real item to this job. If the item is intended to satisfy a job_item
  filter, the index of that filter should be passed in ``filter_idx``; otherwise,
  pass ``-1``. Similarly, if you don't care where the item is inserted, pass
  ``-1`` for ``insert_idx``. The ``role`` param is a ``df.job_item_ref.T_role``.
  If the item needs to be brought to the job site, then the value should be
  ``df.job_item_ref.T_role.Hauled``.

* ``dfhack.job.isSuitableItem(job_item, item_type, item_subtype)``

  Does basic sanity checks to verify if the suggested item type matches
  the flags in the job item.

* ``dfhack.job.isSuitableMaterial(job_item, mat_type, mat_index, item_type)``

  Likewise, if replacing material.

* ``dfhack.job.getName(job)``

  Returns the job's description, as seen in the Units and Jobs screens.

Units module
------------

* ``dfhack.units.isActive(unit)``

  The unit is active (non-dead and probably on the map). Unit must also be
  present in the ``world.units.active`` vector to rule out raid missions. Use
  ``utils.linear_index`` after this function returns true if you aren't
  certain (i.e., you aren't already iterating that vector).

* ``dfhack.units.isVisible(unit)``

  The unit is on a visible map tile. Doesn't account for sneaking.

* ``dfhack.units.isCitizen(unit[,include_insane])``

  The unit is a non-dead sane citizen of the fortress; wraps the
  same checks the game uses to decide game-over by extinction,
  with an additional sanity check. You can identify citizens,
  regardless of their sanity, by passing ``true`` as the optional
  second parameter.

* ``dfhack.units.isResident(unit[,include_insane])``

  The unit is a resident of the fortress. Same ``include_insane`` semantics as
  ``isCitizen``.

* ``dfhack.units.isFortControlled(unit)``

  Similar to ``dfhack.units.isCitizen(unit)``, but is based on checks
  for units hidden in ambush, and includes tame animals. Returns *false*
  if not in fort mode.

* ``dfhack.units.isOwnCiv(unit)``

  The unit belongs to the player's civilization.

* ``dfhack.units.isOwnGroup(unit)``

  The unit belongs to the player's group.

* ``dfhack.units.isOwnRace(unit)``

  The unit belongs to the player's race.

* ``dfhack.units.isAlive(unit)``

  The unit isn't dead or undead. Naturally inorganic is okay.

* ``dfhack.units.isDead(unit)``

  The unit is completely dead and passive, or a ghost. Equivalent to
  ``dfhack.units.isKilled(unit) or dfhack.units.isGhost(unit)``.

* ``dfhack.units.isKilled(unit)``

  The unit has been killed.

* ``dfhack.units.isSane(unit)``

  The unit is normally capable of rational action. I.e., not dead, insane, zombie,
  nor crazed (unless active werebeast).

* ``dfhack.units.isCrazed(unit)``

  The unit is berserk and will attack all other creatures except crazed members of
  its own species. (Can be modified by curses.)

* ``dfhack.units.isGhost(unit)``

  The unit is a ghost.

* ``dfhack.units.isHidden(unit)``

  The unit is hidden to the player, accounting for sneaking. Works for any game mode.

* ``dfhack.units.isHidingCurse(unit)``

  The unit is currently hiding a curse (i.e., vampire).

* ``dfhack.units.isMale(unit)``
* ``dfhack.units.isFemale(unit)``
* ``dfhack.units.isBaby(unit)``
* ``dfhack.units.isChild(unit)``
* ``dfhack.units.isAdult(unit)``

  Simple unit property checks

* ``dfhack.units.isGay(unit)``

  Not willing to breed. Also includes any creature caste without a gender.

* ``dfhack.units.isNaked(unit[,no_items])``

  Not wearing anything (including rings, etc.). Can optionally check for
  empty inventory.

* ``dfhack.units.isVisiting(unit)``

  The unit is visiting. E.g., merchants, diplomats, and travelers.

* ``dfhack.units.isTrainableHunting(unit)``

  The unit is trainable for hunting.

* ``dfhack.units.isTrainableWar(unit)``

  The unit is trainable for war.

* ``dfhack.units.isTrained(unit)``

  The unit is trained for hunting or war, or is non-wild and non-domesticated.

* ``dfhack.units.isHunter(unit)``

  The unit is a trained hunter.

* ``dfhack.units.isWar(unit)``

  The unit is trained for war.

* ``dfhack.units.isTame(unit)``
* ``dfhack.units.isTamable(unit)``
* ``dfhack.units.isDomesticated(unit)``
* ``dfhack.units.isMarkedForTraining(unit)``
* ``dfhack.units.isMarkedForTaming(unit)``
* ``dfhack.units.isMarkedForWarTraining(unit)``
* ``dfhack.units.isMarkedForHuntTraining(unit)``
* ``dfhack.units.isMarkedForSlaughter(unit)``
* ``dfhack.units.isMarkedForGelding(unit)``
* ``dfhack.units.isGeldable(unit)``
* ``dfhack.units.isGelded(unit)``
* ``dfhack.units.isEggLayer(unit)``
* ``dfhack.units.isEggLayerRace(unit)``
* ``dfhack.units.isGrazer(unit)``
* ``dfhack.units.isMilkable(unit)``

  Simple unit property checks.

* ``dfhack.units.isForest(unit)``

  The unit is of the forest.

* ``dfhack.units.isMischievous(unit)``

  The unit is mischievous and will randomly pull levers, etc.

* ``dfhack.units.isAvailableForAdoption(unit)``

  The unit is available for adoption.

* ``dfhack.units.isPet(unit)``

  Unit has pet owner.

* ``dfhack.units.hasExtravision(unit)``
* ``dfhack.units.isOpposedToLife(unit)``
* ``dfhack.units.isBloodsucker(unit)``

  Simple checks of caste attributes that can be modified by curses.

* ``dfhack.units.isDwarf(unit)``

  The unit is of the same race for the fortress. (Includes active werebeasts.)

* ``dfhack.units.isAnimal(unit)``
* ``dfhack.units.isMerchant(unit)``
* ``dfhack.units.isDiplomat(unit)``

  Simple unit type checks.

* ``dfhack.units.isVisitor(unit)``

  The unit is a regular visitor with no special purpose (e.g., merchant).

* ``dfhack.units.isWildlife(unit)``

  The unit is surface or cavern wildlife.

* ``dfhack.units.isAgitated(unit)``

  The unit is an agitated creature.

* ``dfhack.units.isInvader(unit)``

  The unit is an active invader or marauder.

* ``dfhack.units.isUndead(unit[,hiding_curse])``

  The unit is undead. Pass ``true`` as the optional second parameter to
  count undead hiding their curse (i.e., vampires).

* ``dfhack.units.isNightCreature(unit)``
* ``dfhack.units.isSemiMegabeast(unit)``
* ``dfhack.units.isMegabeast(unit)``
* ``dfhack.units.isTitan(unit)``
* ``dfhack.units.isForgottenBeast(unit)``
* ``dfhack.units.isDemon(unit)``

  Simple enemy type checks.

* ``dfhack.units.isDanger(unit)``

  The unit is dangerous and probably hostile. This includes night creatures,
  semi-megabeasts, invaders, agitated wildlife, crazed units, and Great Dangers
  (see below).

* ``dfhack.units.isGreatDanger(unit)``

  The unit is of Great Danger. This includes megabeasts, titans,
  forgotten beasts, and demons.

* ``dfhack.units.isUnitInBox(unit,x1,y1,z1,x2,y2,z2)``

  Returns true if the unit is within a box defined by the specified
  coordinates. Make sure the unit is flagged active and is present in
  ``world.units.active`` first, as the result may indicate that the unit
  died or left map here.

* ``dfhack.units.getUnitsInBox(x1,y1,z1,x2,y2,z2[,filter])``

  Returns a table of all units within the specified coordinates. Returned
  units are guaranteed to be active (unlike ``isUnitInBox`` above).
  If the ``filter`` argument is given, only units where ``filter(unit)``
  returns true will be included. Note that ``pos2xyz()`` cannot currently
  be used to convert coordinate objects to the arguments required by
  this function.

* ``dfhack.units.getUnitByNobleRole(role_name)``

  Returns the unit assigned to the given noble role, if any.
  ``role_name`` must be one of the position codes associated with the
  active fort or civilization government. For example:
  ``CAPTAIN_OF_THE_GUARD``, ``MAYOR``, or ``BARON``.
  Note that if more than one unit has the role, only the first will be
  returned. See ``getUnitsByNobleRole`` below for retrieving all units
  with a particular role.

* ``dfhack.units.getUnitsByNobleRole(role_name)``

  Returns a list of units (possibly empty) assigned to the given noble role.

* ``dfhack.units.getCitizens([exclude_residents[,include_insane]])``

  Returns a list of all living, sane citizens and residents that are
  currently on the map. Can ``exclude_residents`` or ``include_insane``
  (both default to ``false``).

* ``dfhack.units.getPosition(unit)``

  Returns the true *x,y,z* of the unit, or *nil* if invalid. You should
  generally use this method instead of reading *unit.pos* directly since
  that field can be inaccurate when the unit is caged. Make sure the unit is
  active (and present in ``world.units.active``) first or else the result can
  indicate where the unit died or left the map.

* ``dfhack.units.teleport(unit, pos)``

  Moves the specified unit and any riders to the target coordinates, setting
  tile occupancy flags appropriately. Returns true if successful.

* ``dfhack.units.getGeneralRef(unit, type)``

  Searches for a ``general_ref`` with the given type.

* ``dfhack.units.getSpecificRef(unit, type)``

  Searches for a ``specific_ref`` with the given type.

* ``dfhack.units.getContainer(unit)``

  Returns the container (i.e., cage) holding the unit or *nil*.

* ``dfhack.units.getOuterContainerRef(unit)``

  Returns a table (in the style of a ``specific_ref`` struct) of the
  outermost object that contains the unit (or one of the unit itself).
  The ``type`` field contains a
  ``specific_ref_type`` of ``UNIT``, ``ITEM_GENERAL``, or ``VERMIN_EVENT``.
  The ``object`` field contains a pointer to a unit, item, or vermin,
  respectively.

* ``dfhack.units.getIdentity(unit)``

  Returns the false identity of the unit if it has one, or *nil*.

* ``dfhack.units.getNemesis(unit)``

  Returns the nemesis record of the unit if it has one, or *nil*.

* ``dfhack.units.setNickname(unit, nick)``

  Sets the unit's nickname properly.

* ``dfhack.units.getVisibleName(unit)``

  Returns the ``language_name`` object visible in game, accounting for
  false identities.

* ``dfhack.units.assignTrainer(unit[,trainer_id])``
* ``dfhack.units.unassignTrainer(unit)``

  Assigns (or unassigns) a trainer for the specified trainable unit. The
  trainer ID can be omitted if "any trainer" is desired. Returns a boolean
  indicating whether the operation was successful.

* ``dfhack.units.makeown(unit)``

  Makes the selected unit a member of the current fortress and site.
  Note that this operation may silently fail for any of several reasons,
  so it may be prudent to check if the operation has succeeded by using
  ``dfhack.units.isOwnCiv`` or another appropriate predicate on the unit
  in question.

* ``dfhack.units.create(race, caste)``

  Creates a new unit from scratch. The unit will be added to the
  ``world.units.all`` vector, but not to the ``world.units.active`` vector.
  The unit will not have an associated historical figure, nemesis record,
  map position, labors, or any group associations. The unit *will* have a
  race, caste, name, soul, and initialized body and mind (including
  personality). The unit must be configured further as needed and put into
  play by the client.

* ``dfhack.units.getCasteRaw(unit)``
* ``dfhack.units.getCasteRaw(race, caste)``

  Returns the relevant ``caste_raw`` or *nil*.

* ``dfhack.units.getPhysicalAttrValue(unit, attr_type)``
* ``dfhack.units.getMentalAttrValue(unit, attr_type)``

  Computes the effective attribute value, including curse effect.

* ``dfhack.units.casteFlagSet(race, caste, flag)``

  Returns whether the given ``df.caste_raw_flags`` flag is set for the given
  race and caste.

* ``dfhack.units.getMiscTrait(unit, type[, create])``

  Finds (or creates if requested) a misc trait object with the given id.

* ``dfhack.units.getRaceNameById(race)``
* ``dfhack.units.getRaceName(unit)``

  Get raw token name (e.g., "DWARF").

* ``dfhack.units.getRaceReadableNameById(race)``
* ``dfhack.units.getRaceReadableName(unit)``
* ``dfhack.units.getRaceNamePluralById(race)``
* ``dfhack.units.getRaceNamePlural(unit)``

  Get human-readable name (e.g., "dwarf" or "dwarves").

* ``dfhack.units.getRaceBabyNameById(race[,plural])``
* ``dfhack.units.getRaceBabyName(unit[,plural])``
* ``dfhack.units.getRaceChildNameById(race[,plural])``
* ``dfhack.units.getRaceChildName(unit[,plural])``

  Get human-readable baby or child name (e.g., "dwarven baby" or
  "dwarven child").

* ``dfhack.units.getReadableName(unit or historical_figure)``

  Returns a string that includes the language name of the unit (if any), the
  race of the unit (if different from fort), whether it is trained for war or
  hunting, any syndrome-given descriptions (such as "necromancer"), the
  training level (if tame), and profession or noble role. If a
  ``historical_figure`` is passed instead of a unit, some information
  (e.g., agitation status) is not available, and the profession may be
  different (e.g., "Monk") from what is displayed in fort mode.

* ``dfhack.units.getAge(unit[,true_age])``

  Returns the age of the unit in years as a floating-point value.
  If ``true_age`` is true, ignores false identities.

* ``dfhack.units.getKillCount(unit)``

  Returns the number of units the unit has killed.

* ``dfhack.units.getNominalSkill(unit, skill[, use_rust])``

  Retrieves the nominal skill level for the given unit. If ``use_rust``
  is *true*, subtracts the rust penalty.

* ``dfhack.units.getEffectiveSkill(unit, skill)``

  Computes the effective rating for the given skill, taking into account
  skill rust, exhaustion, pain, etc.

* ``dfhack.units.getExperience(unit, skill[, total])``

  Returns the experience value for the given skill. If ``total`` is true,
  adds experience implied by the current skill level.

* ``dfhack.units.isValidLabor(unit, unit_labor)``

  Returns whether the indicated labor is settable for the given unit.

* ``dfhack.units.setLaborValidity(unit_labor, isValid)``

  Sets the given labor to the given (boolean) validity for all units that are
  part of your fortress civilization. Valid labors are allowed to be toggled
  in the in-game labor management screens (including DFHack's `labor
  manipulator screen <manipulator>`).

* ``dfhack.units.computeMovementSpeed(unit)``

  Computes number of frames * 100 it takes the unit to move in its current
  state of mind and body. **Currently broken due to move speed changes,
  will always return 0!**

* ``dfhack.units.computeSlowdownFactor(unit)``

  Meandering and floundering in liquid introduces additional slowdown.
  It is random, but the function computes and returns the expected mean
  factor as a float.

* ``dfhack.units.getNoblePositions(unit or historical_figure)``

  Returns a list of tables describing noble position assignments, or *nil*.
  Every table has fields ``entity``, ``assignment``, and ``position``.

* ``dfhack.units.getProfession(unit)``

  Returns unit's profession ID (``df.profession``), accounting for
  false identity.

* ``dfhack.units.getProfessionName(unit[,ignore_noble[,plural[,land_title]]])``
* ``dfhack.units.getProfessionName(historical_figure[,ignore_noble[,plural[,land_title]]])``

  Retrieves the profession name using custom profession, noble assignments,
  or raws. The ``ignore_noble`` boolean disables the use of noble positions
  ("Prisoner", "Slave", and noble spouse titles included). The ``land_title``
  boolean causes ``of Sitename`` to be appended when applicable. If a
  ``historical_figure`` is passed instead of a unit, some information (e.g.,
  agitation status) is not available, and the profession may be different
  (e.g., "Monk") from what is displayed in fort mode.

* ``dfhack.units.getCasteProfessionName(race, caste, prof_id[, plural])``

  Retrieves the profession name for the given race and caste using raws.

* ``dfhack.units.getProfessionColor(unit[,ignore_noble])``

  Retrieves the color associated with the profession, using noble assignments
  or raws. The ``ignore_noble`` boolean disables the use of noble positions.

* ``dfhack.units.getCasteProfessionColor(race, caste, prof_id)``

  Retrieves the profession color for the given race and caste using raws.

* ``dfhack.units.getGoalType(unit[,goalIndex])``

  Retrieves the goal type of the dream that the given unit has.
  By default the goal of the first dream is returned.
  The ``goalIndex`` parameter may be used to retrieve additional dream goals.
  Currently only one dream per unit is supported by Dwarf Fortress.
  Support for multiple dreams may be added in future versions of
  Dwarf Fortress.

* ``dfhack.units.getGoalName(unit[,goalIndex])``

  Retrieves the short name describing the goal of the dream that the given
  unit has. By default the goal of the first dream is returned (see above).

* ``dfhack.units.isGoalAchieved(unit[,goalIndex])``

  Checks if given unit has achieved the goal of the dream.
  By default the status of the goal of the first dream is returned (see above).

* ``dfhack.units.getMainSocialActivity(unit)``
* ``dfhack.units.getMainSocialEvent(unit)``

  Return the ``df.activity_entry`` or ``df.activity_event`` representing the
  unit's current social activity.

* ``dfhack.units.getStressCategory(unit)``

  Returns a number from 0-6 indicating stress. 0 is most stressed; 6 is least.
  Note that 0 is guaranteed to remain the most stressed but 6 could change in
  the future.

* ``dfhack.units.getStressCategoryRaw(stress_level)``

  Identical to ``getStressCategory`` but takes a raw stress level instead
  of a unit.

* ``dfhack.units.getStressCutoffs()``

  Returns a table of the cutoffs used by the above stress level functions.

Action Timer API
~~~~~~~~~~~~~~~~

This is an API to allow manipulation of unit action timers, to speed them up or slow
them down. All functions in this API have overflow/underflow protection when modifying
action timers (the value will cap out). Actions with a timer of 0 (or less) will not
be modified as they are completed (or invalid in the case of negatives).
Timers will be capped to go no lower than 1.
``affectedActionType`` parameters are values from the DF enum ``unit_action_type``
(e.g., ``df.unit_action_type.Move``).
``affectedActionTypeGroup`` parameters are values from the (custom) DF enum
``unit_action_type_group`` (see
`unit_action_type definition <https://github.com/DFHack/df-structures/blob/master/df.units.xml>`_
for which action types each group contains). They are as follows:

  * ``All``
  * ``Movement``
  * ``MovementFeet`` (affects only walking and crawling speed. If you need to
    differentiate between walking and crawling, check the unit's ``flags1.on_ground`` flag,
    like the Pegasus boots do in the `modding-guide`).
  * ``MovementFeet`` (for walking speed, such as with pegasus boots from the `modding-guide`).
  * ``Combat`` (includes bloodsucking).
  * ``Work``

API functions:

* ``dfhack.units.subtractActionTimers(unit, amount, affectedActionType)``

  Subtract ``amount`` (32-bit integer) from the timers of any actions the unit is performing
  of ``affectedActionType`` (usually one or zero actions in normal gameplay). Negative
  amount adds to timers.

* ``dfhack.units.subtractGroupActionTimers(unit, amount, affectedActionTypeGroup)``

  Subtract ``amount`` (32-bit integer) from the timers of any actions the unit is performing
  that match the ``affectedActionTypeGroup`` category. Negative amount adds to timers.

* ``dfhack.units.multiplyActionTimers(unit, amount, affectedActionType)``

  Multiply the timers of any actions of ``affectedActionType`` the unit is performing by
  ``amount`` (float) (usually one or zero actions in normal gameplay).

* ``dfhack.units.multiplyGroupActionTimers(unit, amount, affectedActionTypeGroup)``

  Multiply the timers of any actions that match the ``affectedActionTypeGroup`` category
  the unit is performing by ``amount`` (float).

* ``dfhack.units.setActionTimers(unit, amount, affectedActionType)``

  Set the timers of any action the unit is performing of ``affectedActionType`` to ``amount``
  (32-bit integer) (usually one or zero actions in normal gameplay).

* ``dfhack.units.setGroupActionTimers(unit, amount, affectedActionTypeGroup)``

  Set the timers of any action the unit is performing that match the
  ``affectedActionTypeGroup`` category to ``amount`` (32-bit integer).

Military module
---------------

* ``dfhack.military.makeSquad(assignment_id)``

  Creates a new squad associated with the assignment (i.e.,
  ``df::entity_position_assignment``, via ``id``) and returns it.
  Fails if a squad already exists that is associated with that assignment, or if
  the assignment is not a fort mode player controlled squad.
  Note: This function does not name the squad. Consider setting a nickname
  (under ``squad.name.nickname``), and/or filling out the ``language_name`` object
  at ``squad.name``. The returned squad is otherwise complete and requires no more
  setup to work correctly.

* ``dfhack.military.updateRoomAssignments(squad_id, assignment_id, squad_use_flags)``

  Sets the sleep, train, indiv_eq, and squad_eq flags when training at a barracks.

* ``dfhack.military.getSquadName(squad_id)``

  Returns the name of a squad as a string.

Items module
------------

* ``dfhack.items.findType(string)``

  Finds an item type by string and returns the ``df.item_type``. String is
  case-sensitive (e.g., "TOOL").

* ``dfhack.items.findSubtype(string)``

  Finds an item subtype by string and returns the subtype or *-1*. String is
  case-sensitive (e.g., "TOOL:ITEM_TOOL_HIVE").

* ``dfhack.items.isCasteMaterial(item_type)``

  Returns *true* if this item type uses a creature/caste pair as its material.

* ``dfhack.items.getSubtypeCount(item_type)``

  Returns the number of raw-defined subtypes of the given item type, or *-1*
  if not applicable.

* ``dfhack.items.getSubtypeDef(item_type, subtype)``

  Returns the raw definition for the given item type and subtype, or *nil*
  if invalid.

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

  Returns the item's container item or *nil*.

* ``dfhack.items.getOuterContainerRef(item)``

  Returns a table (in the style of a ``specific_ref`` struct) of the outermost object
  that contains the item (or one of the item itself.) The ``type`` field contains a
  ``specific_ref_type`` of ``UNIT``, ``ITEM_GENERAL``, or ``VERMIN_EVENT``.
  The ``object`` field contains a pointer to a unit, item, or vermin, respectively.

* ``dfhack.items.getContainedItems(item)``

  Returns a list of items contained in this one.

* ``dfhack.items.getHolderBuilding(item)``

  Returns the holder building or *nil*.

* ``dfhack.items.getHolderUnit(item)``

  Returns the holder unit or *nil*.

* ``dfhack.items.getPosition(item)``

  Returns the true *x,y,z* of the item, or *nil* if invalid. You should generally
  use this method instead of reading *item.pos* directly since that field only stores
  the last position where the item was on the ground. Make sure the item is present in
  ``world.items.other.IN_PLAY`` first, otherwise the result can indicate where a unit
  left the map with the item.

* ``dfhack.items.getBookTitle(item)``

  Returns the title of the "book" item, or an empty string if the item isn't a "book"
  or it doesn't have a title. A "book" is a codex or a tool item that has page or
  writing improvements, such as scrolls and quires.

* ``dfhack.items.getDescription(item, type[, decorate])``

  Returns the string description of the item, as produced by the ``getItemDescription``
  method. A ``type`` of ``0`` results in a string like ``prickle berries [2]``. ``1``
  results in the singular ``prickle berry``, and ``2`` results in the plural
  ``prickle berries``. If decorate is *true*, also adds markings for quality and
  improvements, as well as ``(foreign)`` indicator (when applicable).

* ``dfhack.items.getReadableDescription(item)``

  Returns a string generally fit to usefully describe the item to the player.
  When the item description appears anywhere in a script output or in the UI,
  this is usually the string you should use.

* ``dfhack.items.moveToGround(item,pos)``

  Move the item to the ground at position. Returns *false* if impossible.

* ``dfhack.items.moveToContainer(item,container)``

  Move the item to the container. Returns *false* if impossible.

* ``dfhack.items.moveToBuilding(item,building[,use_mode[,force_in_building])``

  Move the item to the building. Returns *false* if impossible.
  ``use_mode`` defaults to ``df.building_item_role_type.TEMP``.
  If set to ``df.building_item_role_type.PERM``, the item will be treated as part
  of the building. If ``force_in_building`` is true, the item will be considered
  to be stored by the building (used for items temporarily used in traps in
  vanilla DF).

* ``dfhack.items.moveToInventory(item,unit[,use_mode[,body_part]])``

  Move the item to the unit inventory. Returns *false* if impossible.
  ``use_mode`` defaults to ``df.unit_inventory_item.T_mode.Hauled``.
  ``body_part`` defaults to ``-1``.

* ``dfhack.items.remove(item[,no_uncat])``

  Cancels any jobs associated with the item, removes the item from containers
  and inventories, hides the item from the UI, and, unless ``no_uncat`` is
  true, marks it for garbage collection.

* ``dfhack.items.makeProjectile(item)``

  Turns the item into a projectile, and returns the new object, or *nil*
  if impossible.

* ``dfhack.items.getItemBaseValue(item_type, subtype, material, mat_index)``

  Calculates the base value for an item of the specified type and material.

* ``dfhack.items.getValue(item[,caravan_state])``

  Calculates the value of an item. If a ``df.caravan_state`` object is given
  (from ``df.global.plotinfo.caravans`` or
  ``df.global.game.main_interface.trade.mer``), then the value is modified by civ
  properties and any trade agreements that might be in effect.

* ``dfhack.items.createItem(unit, item_type, item_subtype, mat_type, mat_index, no_floor)``

  Creates an item, similar to the `createitem` plugin. Returns a list of created
  ``df.item`` objects.

* ``dfhack.items.checkMandates(item)``

  Returns true if the item is free from mandates, or false if mandates prevent
  trading the item.

* ``dfhack.items.canTrade(item)``

  Checks whether the item can be traded.

* ``dfhack.items.canTradeWithContents(item)``

  Returns false if the item or any contained items cannot be traded.

* ``canTradeAnyWithContents(item)``

  Returns true if the item is empty and can be traded or if the item contains
  any item that can be traded.

* ``dfhack.items.markForTrade(item, depot)``

  Marks the given item for trade at the given depot.

* ``dfhack.items.isRequestedTradeGood(item[,caravan_state])``

  Returns whether a caravan will pay extra for the given item. If caravan_state
  is not given, checks all active caravans.

* ``dfhack.items.canMelt(item[,game_ui])``

  Returns true if the item can be melted (at a smelter). Unless ``game_ui`` is
  given and true, bars, non-empty metal containers, and items in unit
  inventories are not considered meltable, even though they can be designated
  for melting using the game UI.

* ``dfhack.items.markForMelting(item)``

  Marks the given item for melting, unless already marked. Returns true if the
  melting status was changed.

* ``dfhack.items.cancelMelting(item)``

  Removes melting designation, if present, from the given item. Returns true if
  the melting status was changed.

* ``dfhack.items.isRouteVehicle(item)``

  Checks whether the item is an assigned hauling vehicle.

* ``dfhack.items.isSquadEquipment(item)``

  Checks whether the item is assigned to a squad.

* ``dfhack.items.getCapacity(item)``

  Returns the capacity volume of an item that can serve as a container for
  other items. Return value will be ``0`` for items that cannot serve as a
  container.

.. _lua-world:

World module
------------

* ``dfhack.world.ReadPauseState()``

  Returns *true* if the game is paused.

* ``dfhack.world.SetPauseState(paused)``

  Sets the pause state of the game.

* ``dfhack.world.ReadCurrentYear()``

  Returns the current game year.

* ``dfhack.world.ReadCurrentTick()``

  Returns the number of game ticks (``df.global.world.frame_counter``) since the
  start of the current game year.

* ``dfhack.world.ReadCurrentMonth()``

  Returns the current game month, ranging from 0-11. The Dwarven year has 12 months.

* ``dfhack.world.ReadCurrentDay()``

  Returns the current game day, ranging from 1-28. Each Dwarven month has 28 days.

* ``dfhack.world.ReadCurrentWeather()``

  Returns the current game weather (``df.weather_type``).

* ``dfhack.world.SetCurrentWeather(weather)``

  Sets the current game weather to ``weather``.

* ``dfhack.world.ReadWorldFolder()``

  Returns the name of the directory/folder the current saved game is under, or an
  empty string if no game was loaded this session.

* ``dfhack.world.isFortressMode([gametype])``
* ``dfhack.world.isAdventureMode([gametype])``
* ``dfhack.world.isArena([gametype])``
* ``dfhack.world.isLegends([gametype])``

  Without any arguments, returns *true* if the current gametype matches.
  Optionally accepts a ``gametype`` id to match against.

* ``dfhack.world.getCurrentSite()``

  Returns the currently loaded ``df.world_site`` or ``nil`` if no site is loaded.

* ``dfhack.world.getAdventurer()``

  Returns the current adventurer unit (if in adventure mode).

.. _lua-maps:

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

* ``dfhack.maps.isTileVisible(coords)``, or ``isTileVisible(x,y,z)``

  Checks if the given df::coord or x,y,z in local tile coordinates is visible.

* ``dfhack.maps.getTileBlock(coords)``, or ``getTileBlock(x,y,z)``

  Returns a map block object for given df::coord or x,y,z in local
  tile coordinates.

* ``dfhack.maps.ensureTileBlock(coords)``, or ``ensureTileBlock(x,y,z)``

  Like ``getTileBlock``, but if the block is not allocated, try creating it.

* ``dfhack.maps.getTileType(coords)``, or ``getTileType(x,y,z)``

  Returns the tile type at the given coordinates, or *nil* if invalid.

* ``dfhack.maps.getTileFlags(coords)``, or ``getTileFlags(x,y,z)``

  Returns designation and occupancy references for the given coordinates, or
  *nil, nil* if invalid.

* ``dfhack.maps.getRegionBiome(region_coord2d)``, or ``getRegionBiome(x,y)``

  Returns the biome info struct for the given global map region.

  ``dfhack.maps.getBiomeType(region_coord2d)`` or ``getBiomeType(x,y)``

  Returns the biome_type for the given global map region.

* ``dfhack.maps.enableBlockUpdates(block[,flow[,temperature]])``

  Enables updates for liquid flow or temperature, unless already active.

* ``dfhack.maps.spawnFlow(pos,type,mat_type,mat_index,dimension)``

  Spawns a new flow (i.e., steam/mist/dust/etc) at the given pos, and with
  the given parameters. Returns it, or *nil* if unsuccessful.

* ``dfhack.maps.getGlobalInitFeature(index)``

  Returns the global feature object with the given index.

* ``dfhack.maps.getLocalInitFeature(region_coord2d,index)``

  Returns the local feature object with the given region coords and index.

* ``dfhack.maps.getTileBiomeRgn(coords)``, or ``getTileBiomeRgn(x,y,z)``

  Returns *x, y* for use with ``getRegionBiome`` and ``getBiomeType``.

* ``dfhack.maps.getPlantAtTile(pos)``, or ``getPlantAtTile(x,y,z)``

  Returns the plant struct that owns the tile at the specified position.

* ``dfhack.maps.getWalkableGroup(pos)``

  Returns the walkability group for the given tile position. A return value
  of ``0`` indicates that the tile is not walkable. The data comes from a
  pathfinding cache maintained by DF.

  .. note::
    This cache is only updated when the game is unpaused, and thus
    can get out of date if doors are forbidden or unforbidden, or
    tools like `liquids` or `tiletypes` are used. It also cannot possibly
    take into account anything that depends on the actual units, like
    burrows, or the presence of invaders.

* ``dfhack.maps.canWalkBetween(pos1, pos2)``

  Checks if both positions are walkable and also share a walkability group.

* ``dfhack.maps.hasTileAssignment(tilemask)``

  Checks if the tile_bitmask object is not *nil* and contains any set bits.
  Returns *true* or *false*.

* ``dfhack.maps.getTileAssignment(tilemask,x,y)``

  Checks if the tile_bitmask object is not *nil* and has the relevant bit set.
  Returns *true* or *false*.

* ``dfhack.maps.setTileAssignment(tilemask,x,y,enable)``

  Sets the relevant bit in the tile_bitmask object to the *enable* argument.

* ``dfhack.maps.resetTileAssignment(tilemask[,enable])``

  Sets all bits in the mask to the *enable* argument.

* ``dfhack.maps.isTileAquifer(pos)``, or ``isTileAquifer(x,y,z)``

  Checks if there's an aquifer on the given tile position.
  Returns *true* or *false*.

* ``dfhack.maps.isTileHeavyAquifer(pos)``, or ``isTileHeavyAquifer(x,y,z)``

  Checks if there's a heavy aquifer on the given tile position.
  Returns *true* or *false*.

* ``dfhack.maps.setTileAquifer(pos[,heavy])``, or ``setTileAquifer(x,y,z[,heavy])``

  Adds a light aquifer on the given tile position, or a heavy aquifer if the
  *heavy* argument is *true*. Returns *true* or *false* depending on success.

* ``dfhack.maps.removeTileAquifer(pos)``, or ``removeTileAquifer(x,y,z)``

  Removes an aquifer from the given tile position.
  Returns *true* or *false* depending on success.

Burrows module
--------------

* ``dfhack.burrows.findByName(name[, ignore_final_plus])``

  Returns the burrow pointer or *nil*. if ``ignore_final_plus`` is ``true``,
  then ``+`` characters at the end of the names are ignored, both for the
  specified ``name`` and the names of the burrows that it matches against.

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

  Adds or removes the tile from the burrow.
  Returns *false* if invalid coords.

* ``dfhack.burrows.isAssignedBlockTile(burrow,block,x,y)``

  Checks if the tile within the block is in burrow.

* ``dfhack.burrows.setAssignedBlockTile(burrow,block,x,y,enable)``

  Adds or removes the tile from the burrow.
  Returns *false* if invalid coords.

Buildings module
----------------

General
~~~~~~~

* ``dfhack.buildings.getGeneralRef(building, type)``

  Searches for a general_ref with the given type.

* ``dfhack.buildings.getSpecificRef(building, type)``

  Searches for a specific_ref with the given type.

* ``dfhack.buildings.setOwner(civzone,unit)``

  Replaces the owner of the civzone. If unit is *nil*, removes ownership.
  Returns *false* in case of error.

  ``dfhack.buildings.getName(building)``

  Returns the name of the building as it would appear in game.

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

* ``dfhack.buildings.checkFreeTiles(pos,size[,extents[,change_extents[,allow_occupied[,allow_wall[,allow_flow]]]]])``

  Checks if the rectangle defined by ``pos`` and ``size``, and possibly extents,
  can be used for placing a building. If ``change_extents`` is true, bad tiles
  are removed from extents. If ``allow_occupied``, the occupancy test is skipped.
  Set ``allow_wall`` to true if the building is unhindered by walls (such as an
  activity zone). Set ``allow_flow`` to true if the building can be built even
  if there is deep water or any magma on the tile (such as abstract buildings).

* ``dfhack.buildings.countExtentTiles(extents,defval)``

  Returns the number of tiles included by extents, or defval.

* ``dfhack.buildings.containsTile(building, x, y)``

  Checks if the building contains the specified tile. If the building contains
  extents, then the extents are checked. Otherwise, returns whether the x and y
  map coordinates are within the building's bounding box.

* ``dfhack.buildings.hasSupport(pos,size)``

  Checks if a bridge constructed at specified position would have
  support from terrain, and thus won't collapse if retracted.

* ``dfhack.buildings.getStockpileContents(stockpile)``

  Returns a list of items stored on the given stockpile.
  Ignores empty bins, barrels, and wheelbarrows assigned as storage and
  transport for that stockpile.

* ``dfhack.buildings.getCageOccupants(cage)``

  Returns a list of units in the given built cage. Note that this is different
  from the list of units assigned to the cage, which can be accessed with
  ``cage.assigned_units``.

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
  if any tiles were removed from designation. You can specify a non-rectangular
  designation for building types that support extents by setting the
  ``room.extents`` bitmap before calling this function. The extents will be
  reset, however, if the size returned by this function doesn't match the
  input size parameter.

* ``dfhack.buildings.constructAbstract(building)``

  Links a fully configured object created by ``allocInstance`` into the
  world. The object must be an abstract building, i.e., a stockpile or civzone.
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

* ``dfhack.buildings.notifyCivzoneModified(building)``

  Rebuilds the civzone <-> overlapping building association mapping.
  Call after changing extents or modifying size in some fashion

* ``dfhack.buildings.markedForRemoval(building)``

  Returns *true* if the building is marked for removal (with :kbd:`x`), *false*
  otherwise.

* ``dfhack.buildings.getRoomDescription(building[, unit])``

  If the building is a room, returns a description including quality modifiers,
  e.g., "Royal Bedroom". Otherwise, returns an empty string.

  The unit argument is passed through to DF and may modify the room's value
  depending on the unit given.

* ``dfhack.buildings.completeBuild(building)``

  Complete an unconstructed or partially-constructed building and link it into
  the world.

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
  ``chain``, ``mechanism``, ``screw``, ``pipe``, ``anvil``, ``weapon`` are used
  to augment the basic attributes with more detailed information if the
  building has input items with the matching name (see the tables for naming
  details). Note that it is impossible to *override* any properties this way,
  only supply those that are not mentioned otherwise. One exception is that
  ``flags2.non_economic`` is automatically cleared if an explicit material
  is specified.

* ``dfhack.buildings.constructBuilding{...}``

  Creates a building in one call, using options contained
  in the argument table. Returns the building, or *nil, error*.

  .. note::
    Despite the name, unless the building is abstract,
    the function creates it in an 'unconstructed' stage, with
    a queued in-game job that will actually construct it. I.e.,
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

    Initializes fields of the building object after creation with
    ``df.assign``. If ``room.extents`` is assigned this way and this function
    returns with error, the memory allocated for the extents is freed.

  - ``width = ..., height = ..., direction = ...``

    Sets size and orientation of the building. If it is
    fixed-size, specified dimensions are ignored.

  - ``full_rectangle = true``

    For buildings like stockpiles or farm plots that can normally
    accommodate individual tile exclusion, forces an error if any
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

* ``dfhack.constructions.findAtTile(pos)``, or ``findAtTile(x,y,z)``

  Returns the construction at the given position, or ``nil`` if there isn't one.

* ``dfhack.constructions.insert(construction)``

  Properly inserts the given construction into the game. Returns false and fails
  to insert if there was already a construction at the position.

Kitchen module
--------------

* ``dfhack.kitchen.findExclusion(type, item_type, item_subtype, mat_type, mat_index)``

  Finds a kitchen exclusion in the vectors in ``df.global.ui.kitchen``. Returns
  -1 if not found.

  * ``type`` is a ``df.kitchen_exc_type`` with exactly one flag set, i.e
    ``{Cook=true}`` or ``{Brew=true}``.
  * ``item_type`` is a ``df.item_type``
  * ``item_subtype``, ``mat_type``, and ``mat_index`` are all numeric

* ``dfhack.kitchen.addExclusion(type, item_type, item_subtype, mat_type, mat_index)``
* ``dfhack.kitchen.removeExclusion(type, item_type, item_subtype, mat_type, mat_index)``

  Adds or removes a kitchen exclusion, using the same parameters as
  ``findExclusion``. Both return ``true`` on success and ``false`` on failure,
  e.g., when adding an exclusion that already exists or removing one that does
  not.

Screen API
----------

The screen module implements support for drawing to the tiled screen of the game.
Note that drawing only has any effect when done from callbacks, so it can only
be feasibly used in the `core context <lua-core-context>`.

.. contents::
  :local:

Basic painting functions
~~~~~~~~~~~~~~~~~~~~~~~~

Common parameters to these functions include:

* ``x``, ``y``: screen coordinates in tiles; the upper left corner of the screen
  is ``x = 0, y = 0``
* ``pen``: a `pen object <lua-screen-pen>`
* ``map``: a boolean indicating whether to draw to a separate map buffer
  (defaults to false, which is suitable for off-map text or a screen that hides
  the map entirely). Note that only third-party plugins like TWBT currently
  implement a separate map buffer. If no such plugins are enabled, passing
  ``true`` has no effect. However, this parameter should still be used to ensure
  that scripts work properly with such plugins.

Functions:

* ``dfhack.screen.getWindowSize()``

  Returns *width, height* of the screen.

* ``dfhack.screen.getMousePos()``

  Returns *x,y* of the UI interface tile the mouse is over, with the upper left
  corner being ``0,0``. To get the map tile coordinate that the mouse is over,
  see ``dfhack.gui.getMousePos()``.

* ``dfhack.screen.getMousePixels()``

  Returns *x,y* of the screen coordinates the mouse is over in pixels, with the
  upper left corner being ``0,0``.

* ``dfhack.screen.inGraphicsMode()``

  Checks if [GRAPHICS:YES] was specified in init.

* ``dfhack.screen.paintTile(pen,x,y[,char[,tile[,map]]])``

  Paints a tile using given parameters. `See below <lua-screen-pen>` for a
  description of ``pen``.

  Returns *false* on error, e.g., if coordinates are out of bounds

* ``dfhack.screen.readTile(x,y[,map])``

  Retrieves the contents of the specified tile from the screen buffers.
  Returns a `pen object <lua-screen-pen>`, or *nil* if invalid or TrueType.

* ``dfhack.screen.paintString(pen,x,y,text[,map])``

  Paints the string starting at *x,y*. Uses the string characters
  in sequence to override the ``ch`` field of `pen <lua-screen-pen>`.

  Returns *true* if painting at least one character succeeded.

* ``dfhack.screen.fillRect(pen,x1,y1,x2,y2[,map])``

  Fills the rectangle specified by the coordinates with the given
  `pen <lua-screen-pen>`. Returns *true* if painting at least one
  character succeeded.

* ``dfhack.screen.findGraphicsTile(pagename,x,y)``

  Finds a tile from a graphics set (i.e., the raws used for creatures),
  if in graphics mode and loaded.

  Returns: *tile, tile_grayscale*, or *nil* if not found.
  The values can then be used for the *tile* field of *pen* structures.

* ``dfhack.screen.hideGuard(screen,callback[,args...])``

  Removes screen from the viewscreen stack, calls the callback
  (with optional supplied arguments), and then restores the screen on
  the top of the viewscreen stack.

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

.. _lua-screen-pen:

Pen API
~~~~~~~

The ``pen`` argument used by ``dfhack.screen`` functions may be represented
by a table with the following possible fields:

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
  ``keep_lower``
    If set to true, will not overwrite the background tile when filling in
    the foreground tile.
  ``write_to_lower``
    If set to true, the specified ``tile`` will be written to the background
    instead of the foreground.
  ``top_of_text``
    If set to true, the specified ``tile`` will have the top half of the specified
    ``ch`` character superimposed over the lower half of the tile.
  ``bottom_of_text``
    If set to true, the specified ``tile`` will have the bottom half of the specified
    ``ch`` character superimposed over the top half of the tile.

Alternatively, it may be a pre-parsed native object with the following API:

* ``dfhack.pen.make(base[,pen_or_fg[,bg[,bold]]])``

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

* ``dfhack.pen.parse(base[,pen_or_fg[,bg[,bold]]])``

  Exactly like the above function, but returns ``base`` or ``pen_or_fg``
  directly if they are already a pre-parsed native object.

* ``pen.property``, ``pen.property = value``, ``pairs(pen)``

  Pre-parsed pens support reading and setting their properties,
  but don't behave exactly like a simple table would; for instance,
  assigning to ``pen.tile_color`` also resets ``pen.tile_fg`` and
  ``pen.tile_bg`` to *nil*.

Screen management
~~~~~~~~~~~~~~~~~

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

.. note::

  * The `gui.Screen class <lua-gui-screen>` provides stubs for all of the
    functions listed below, and its use is recommended
  * Lua-implemented screens are only supported in the `core context <lua-core-context>`.

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
  In order to make a dialog where portions of the parent viewscreen are still
  visible in the background, call ``screen:renderParent()``.

  If artifacts are left on the parent even after this function is called, such
  as when the window is dragged or is resized, any code can set
  ``gui.Screen.request_full_screen_refresh`` to ``true``. Then when
  ``screen.renderParent()`` is next called, it will do a full flush of the
  graphics and clear the screen of artifacts.

* ``function screen:onIdle()``

  Called every frame when the screen is on top of the stack.

* ``function screen:onHelp()``

  Called when the help keybinding is activated (usually '?').

* ``function screen:onInput(keys)``

  Called when keyboard or mouse events are available.
  If any keys are pressed, the keys argument is a table mapping them to *true*.
  Note that this refers to logical keybindings computed from real keys via
  options; if multiple interpretations exist, the table will contain multiple keys.

  The table also may contain special keys:

  ``_STRING``
    Maps to an integer in range 0-255. Duplicates a separate "STRING_A???" code
    for convenience.

  ``_MOUSE_L, _MOUSE_R, _MOUSE_M``
    If the left, right, and/or middle mouse button was just pressed.

  ``_MOUSE_L_DOWN, _MOUSE_R_DOWN, _MOUSE_M_DOWN``
    If the left, right, and/or middle mouse button is being held down.

  If this method is omitted, the screen is dismissed on reception of the
  ``LEAVESCREEN`` key.

* ``function screen:onGetSelectedUnit()``
* ``function screen:onGetSelectedItem()``
* ``function screen:onGetSelectedJob()``
* ``function screen:onGetSelectedBuilding()``
* ``function screen:onGetSelectedStockpile()``
* ``function screen:onGetSelectedCivZone()``
* ``function screen:onGetSelectedPlant()``

  Override these if you want to provide a custom return value for the matching
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


Textures module
---------------

In order for the game to render a particular tile (graphic), it needs to know the
``texpos`` - the position in the vector of the registered game textures (also the
graphical tile id passed as the ``tile`` field in a `Pen <lua-screen-pen>`).
Adding new textures to the vector is not difficult, but the game periodically
deletes textures that are in the vector, and that's a problem since it
invalidates the ``texpos`` value that used to point to that texture.
The ``textures`` module solves this problem by providing a stable handle instead of a
raw ``texpos``. When we need to draw a particular tile, we can look up the current
``texpos`` value via the handle.
Texture module can register textures in two ways: to reserved and dynamic ranges.
Reserved range is a limit buffer in a game texture vector, that will never be wiped.
It is good for static assets, which need to be loaded at the very beginning and will
be used during the process running. In other cases, it is better to use dynamic range.
If reserved range buffer limit has been reached, dynamic range will be used by default.

* ``loadTileset(file, tile_px_w, tile_px_h[, reserved])``

  Loads a tileset from the image ``file`` with give tile dimensions in pixels. The
  image will be sliced in row major order. Returns an array of ``TexposHandle``.
  ``reserved`` is optional boolean argument, which indicates texpos range.
  ``true`` - reserved, ``false`` - dynamic (default).

  Example usage::

    local logo_textures = dfhack.textures.loadTileset('hack/data/art/dfhack.png', 8, 12)
    local first_texposhandle = logo_textures[1]

* ``getTexposByHandle(handle)``

  Get the current ``texpos`` for the given ``TexposHandle``. Always use this method to
  get the ``texpos`` for your texture. ``texpos`` can change when game textures are
  reset, but the handle will be the same.

* ``createTile(pixels, tile_px_w, tile_px_h[, reserved])``

  Create and register a new texture with the given tile dimensions and an array of
  ``pixels`` in row major order. Each pixel is an integer representing color in packed
  RBGA format (for example, #0022FF11). Returns a ``TexposHandle``.
  ``reserved`` is optional boolean argument, which indicates texpos range.
  ``true`` - reserved, ``false`` - dynamic (default).

* ``createTileset(pixels, texture_px_w, texture_px_h, tile_px_w, tile_px_h[, reserved])``

  Create and register a new texture with the given texture dimensions and an array of
  ``pixels`` in row major order. Then slice it into tiles with the given tile
  dimensions. Each pixel is an integer representing color in packed RBGA format (for
  example #0022FF11). Returns an array of ``TexposHandle``.
  ``reserved`` is optional boolean argument, which indicates texpos range.
  ``true`` - reserved, ``false`` - dynamic (default).

* ``deleteHandle(handle)``

  ``handle`` here can be single ``TexposHandle`` or an array of ``TexposHandle``.
  Deletes all metadata and texture(s) related to the given handle(s). The handles
  become invalid after this call.


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

  Returns the current working directory. To retrieve the DF path, use
  ``dfhack.getDFPath()`` instead.

* ``dfhack.filesystem.chdir(path)``

  Changes the current directory to ``path``. Use with caution.

* ``dfhack.filesystem.restore_cwd()``

  Restores the current working directory to what it was when DF started.

* ``dfhack.filesystem.get_initial_cwd()``

  Returns the value of the working directory when DF was started.

* ``dfhack.filesystem.mkdir(path)``

  Creates a new directory. Returns ``false`` if unsuccessful, including if
  ``path`` already exists.

* ``dfhack.filesystem.mkdir_recursive(path)``

  Creates a new directory, including any intermediate directories that
  don't exist yet. Returns ``true`` if the folder was created or already
  existed, or ``false`` if unsuccessful.

* ``dfhack.filesystem.rmdir(path)``

  Removes a directory. Only works if the directory is already empty.

* ``dfhack.filesystem.mtime(path)``

  Returns the modification time (in seconds) of the file or directory
  specified by ``path``, or -1 if ``path`` does not exist.
  This depends on the system clock and should only be used locally.

* ``dfhack.filesystem.atime(path)``
* ``dfhack.filesystem.ctime(path)``

  Return values vary across operating systems - return the ``st_atime`` and
  ``st_ctime`` fields of a C++ stat struct, respectively.

* ``dfhack.filesystem.listdir(path)``

  Lists files/directories in a directory.  Returns ``{}`` if ``path`` does not exist.

* ``dfhack.filesystem.listdir_recursive(path [, depth = 10[, include_prefix = true]])``

  Lists all files/directories in a directory and its subdirectories. All directories
  are listed before their contents. Returns a table with subtables of the format::

    {path: 'path to file', isdir: true|false}

  Note that ``listdir()`` returns only the base name of each directory entry, while
  ``listdir_recursive()`` returns the initial path and all components following it
  for each entry. Set ``include_prefix`` to false if you don't want the ``path``
  string prepended to the returned filenames.

Console API
-----------

* ``dfhack.console.clear()``

  Clears the console; equivalent to the ``cls`` built-in command.

* ``dfhack.console.flush()``

  Flushes all output to the console. This can be useful when printing text that
  does not end in a newline but should still be displayed.

.. _lua-api-internal:

Internal API
------------

These functions are intended for the use by dfhack developers,
and are only documented here for completeness:

* ``dfhack.internal.getPE()``

  Returns the PE timestamp of the DF executable (only on Windows)

* ``dfhack.internal.getMD5()``

  Returns the MD5 of the DF executable (only on OS X and Linux)

* ``dfhack.internal.getAddress(name)``

  Returns the global address ``name``, or *nil*.

* ``dfhack.internal.setAddress(name, value)``

  Sets the global address ``name``. Returns the value of ``getAddress``
  before the change.

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

  Like memmove below, but works even if dest is read-only memory, e.g., code.
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

* ``dfhack.internal.diffscan(old_data, new_data, start_idx, end_idx, eltsize[, oldval[, newval[, delta]]])``

  Searches for differences between buffers at ptr1 and ptr2, as integers of size eltsize.
  The oldval, newval or delta arguments may be used to specify additional constraints.
  Returns: *found_index*, or *nil* if end reached.

* ``dfhack.internal.cxxDemangle(mangled_name)``

  Decodes a mangled C++ symbol name. Returns the demangled name on success, or
  ``nil, error_message`` on failure.

* ``dfhack.internal.getDir(path)``

  Lists files/directories in a directory.
  Returns: *file_names* or empty table if not found. Identical to ``dfhack.filesystem.listdir(path)``.

* ``dfhack.internal.strerror(errno)``

  Wraps strerror() - returns a string describing a platform-specific error code

* ``dfhack.internal.addScriptPath(path, search_before)``

  Registers ``path`` as a `script path <script-paths>`.
  If ``search_before`` is passed and ``true``, the path will be searched before
  the default paths (e.g., ``dfhack-config/scripts``, ``hack/scripts``); otherwise,
  it will be searched after.

  Returns ``true`` if successful or ``false`` otherwise (e.g., if the path does
  not exist or has already been registered).

* ``dfhack.internal.removeScriptPath(path)``

  Removes ``path`` from the list of `script paths <script-paths>` and returns
  ``true`` if successful.

* ``dfhack.internal.getScriptPaths()``

  Returns the list of `script paths <script-paths>` in the order they are
  searched, including defaults. (This can change if a world is loaded.)

* ``dfhack.internal.findScript(name)``

  Searches `script paths <script-paths>` for the script ``name`` and returns the
  path of the first file found, or ``nil`` on failure.

  .. note::
    This requires an extension to be specified (``.lua`` or ``.rb``) - use
    ``dfhack.findScript()`` to include the ``.lua`` extension automatically.

* ``dfhack.internal.runCommand(command[, use_console])``

  Runs a DFHack command with the core suspended. Used internally by the
  ``dfhack.run_command()`` family of functions.

  - ``command``: either a table of strings or a single string which is parsed by
    the default console tokenization strategy (not recommended)
  - ``use_console``: if true, output is sent directly to the DFHack console

  Returns a table with a ``status`` key set to a ``command_result`` constant
  (``status = CR_OK`` indicates success). Additionally, if ``use_console`` is
  not true, enumerated table entries of the form ``{color, text}`` are included,
  e.g., ``result[1][0]`` is the color of the first piece of text printed (a
  ``COLOR_`` constant). These entries can be iterated over with ``ipairs()``.

* ``dfhack.internal.md5(string)``

  Returns the MD5 hash of the given string.

* ``dfhack.internal.md5File(filename[,first_kb])``

  Computes the MD5 hash of the given file. Returns ``hash, length`` on success
  (where ``length`` is the number of bytes read from the file), or ``nil,
  error`` on failure.

  If the parameter ``first_kb`` is specified and evaluates to ``true``, and the
  hash was computed successfully, a table containing the first 1024 bytes of the
  file is returned as the third return value.

* ``dfhack.internal.threadid()``

  Returns a numeric identifier of the current thread.

* ``dfhack.internal.msizeAddress(address)``

  Returns the allocation size of an address.
  Does not require a heap snapshot. This function will crash on an invalid pointer.
  Windows only.

* ``dfhack.internal.getHeapState()``

  Returns the state of the heap. 0 == ok or empty, 1 == heap bad ptr, 2 == heap bad begin,
  3 == heap bad node. Does not require a heap snapshot. This may be unsafe to use directly
  from lua if the heap is corrupt. Windows only.

* ``dfhack.internal.heapTakeSnapshot()``

  Clears any existing heap snapshot, and takes an internal heap snapshot for later
  consumption. Windows only. Returns the same values as getHeapState()

* ``dfhack.internal.isAddressInHeap(address)``

  Checks if an address is a member of the heap. It may be dangling.
  Requires a heap snapshot.

* ``dfhack.internal.isAddressActiveInHeap(address)``

  Checks if an address is a member of the heap, and actively in use (i.e., valid).
  Requires a heap snapshot.

* ``dfhack.internal.isAddressUsedAfterFreeInHeap(address)``

  Checks if an address is a member of the heap, but is not currently allocated
  (i.e., use after free). Requires a heap snapshot.
  Note that Windows eagerly removes freed pointers from the heap,
  so this is unlikely to trigger.

* ``dfhack.internal.getAddressSizeInHeap(address)``

  Gets the allocated size of a member of the heap. Useful for detecting misaligns,
  as this does not return block size. Requires a heap snapshot.

* ``dfhack.internal.getRootAddressOfHeapObject(address)``

  Gets the base heap allocation address of a address that lies internally within
  a piece of allocated memory. E.g., if you have a heap allocated struct and call
  this function on the address of the second member, it will return the address
  of the struct. Returns 0 if the address is not found. Requires a heap snapshot.

* ``dfhack.internal.getClipboardTextCp437()``

  Gets the system clipboard text (and converts text to CP437 encoding).

* ``dfhack.internal.setClipboardTextCp437(text)``

  Sets the system clipboard text from a CP437 string.

* ``dfhack.internal.getClipboardTextCp437Multiline()``

  Gets the system clipboard text (and converts text to CP437 encoding).
  Character 0x10 is interpreted as a newline instead of the usual CP437 glyph.
  The text is returned as a list of strings, one for each line of text on the
  clipboard.

* ``dfhack.internal.setClipboardTextCp437Multiline(text)``

  Sets the system clipboard text from a CP437 string. Character 0x10 is
  interpreted as a newline instead of the usual CP437 glyph.

* ``dfhack.internal.getSuppressDuplicateKeyboardEvents()``
* ``dfhack.internal.setSuppressDuplicateKeyboardEvents(suppress)``

  Gets and sets the flag for whether to suppress DF key events when a DFHack
  keybinding is matched and a command is launched.

* ``dfhack.internal.setMortalMode(value)``
* ``dfhack.internal.setArmokTools(tool_names)``

  Used to sync mortal mode state to DFHack Core memory for use in keybinding
  checks.

* ``dfhack.internal.setPreferredNumberFormat(value)``
* ``dfhack.internal.getPreferredNumberFormat()``

  Sets (gets) the preferred numeric format. ``0`` means no formatting (e.g.,
  ``1234567``), ``1`` means English formatting (e.g., ``1,234,567``), ``2``
  means system locale formatting (e.g., ``12.345`` on German systems,
  ``12,34,567`` on Indian systems, etc.), ``3`` means SI suffix formatting
  (e.g., ``12.3M``), and ``4`` means scientific notation (e.g., ``1.23457e+06``).

For the internal preference values, be aware that setting the values via these
functions will not persist the choice across program invocations. You must set
preferences via the `control-panel` or `gui/control-panel` interfaces for that.

.. _lua-core-context:

Core interpreter context
========================

While plugins can create any number of interpreter instances,
there is one special context managed by the DFHack core. It is the
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
  ``'frames'`` are canceled when the world is unloaded,
  and cannot be queued until it is loaded again.
  Returns the timer id, or *nil* if unsuccessful due to
  world being unloaded.

* ``dfhack.timeout_active(id[,new_callback])``

  Returns the active callback with the given id, or *nil*
  if inactive or nil id. If called with 2 arguments, replaces
  the current callback with the given value, if still active.
  Using ``timeout_active(id,nil)`` cancels the timer.

* ``dfhack.onStateChange.foo = function(code)``

  Creates a handler for state change events. Receives the same
  `SC_ codes <lua-globals>` as ``plugin_onstatechange()`` in C++.


Event type
----------

An event is a native object transparently wrapping a lua table,
and implementing a __call metamethod. When it is invoked, it loops
through the table with next and calls all contained values.
This is intended as an extensible way to add listeners.

This type itself is available in any context, but only the
`core context <lua-core-context>` has the actual events defined by C++ code.

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

  If called the second time, returns the same table,
  thus providing reload support.

* ``reload(name)``

  Reloads a previously ``require``-d module *"name"* from the file.
  Intended as a help for module development.

* ``dfhack.BASE_G``

  This variable contains the root global environment table, which is
  used as a base for all module and script environments. Its contents
  should be kept limited to the standard Lua library and API described
  in this document.

.. _lua-globals:

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

  ``COLOR_GREY`` and ``COLOR_DARKGREY`` can also be spelled ``COLOR_GRAY`` and
  ``COLOR_DARKGRAY``.

* State change event codes, used by ``dfhack.onStateChange``

  Available only in the `core context <lua-core-context>`, as is the event itself:

  SC_WORLD_LOADED, SC_WORLD_UNLOADED, SC_MAP_LOADED,
  SC_MAP_UNLOADED, SC_VIEWSCREEN_CHANGED, SC_CORE_INITIALIZED

* Command result constants (equivalent to ``command_result`` in C++), used by
  ``dfhack.run_command()`` and related functions:

  CR_OK, CR_LINK_FAILURE, CR_NEEDS_CONSOLE, CR_NOT_IMPLEMENTED, CR_FAILURE,
  CR_WRONG_USAGE, CR_NOT_FOUND

* Functions already described above

  safecall, qerror, mkmodule, reload

* Miscellaneous constants

  ``NEWLINE``, ``COMMA``, ``PERIOD``
    evaluate to the relevant character strings.
  ``DEFAULT_NIL``
    is an unspecified unique token used by the class module below.

* ``printall(obj)``

  If the argument is a lua table or DF object reference, prints all fields.

* ``printall_recurse(obj)``

  If the argument is a lua table or DF object reference, prints all fields recursively.

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
  Returns *nil* if any of obj or indices is *nil*, obj isn't indexable, or a numeric
  index is out of array bounds.

* ``ensure_key(t, key[, default_value])``

  If the Lua table ``t`` doesn't include the specified ``key``, ``t[key]`` is
  set to the value of ``default_value``, which defaults to ``{}`` if not set.
  The new or existing value of ``t[key]`` is then returned.

* ``ensure_keys(t, key...)``

  Walks a series of keys, creating any missing keys as empty tables. The new or
  existing table from the last specified key is returned from the function.

.. _lua-string:

String class extensions
-----------------------

DFHack extends Lua's basic string class to include a number of convenience
functions. These are invoked just like standard string functions, e.g.::

    if imastring:startswith('imaprefix') then

* ``string:startswith(prefix)``

  Returns ``true`` if the first ``#prefix`` characters of the string are equal
  to ``prefix``. Note that ``prefix`` is not interpreted as a pattern.

* ``string:endswith(suffix)``

  Returns ``true`` if the last ``#suffix`` characters of the string are equal
  to ``suffix``. Note that ``suffix`` is not interpreted as a pattern.

* ``string:split([delimiter[, plain]])``

  Split a string by the given delimiter. If no delimiter is specified, space
  (``' '``) is used. The delimiter is treated as a pattern unless a ``plain`` is
  specified and set to ``true``. To treat multiple successive delimiter
  characters as a single delimiter, e.g., to avoid getting empty string elements,
  pass a pattern like ``' +'``. Be aware that passing patterns that match empty
  strings (like ``' *'``) will result in improper string splits.

* ``string:trim()``

  Removes spaces (i.e., everything that matches ``'%s'``) from the start and end
  of a string. Spaces between non-space characters are left untouched.

* ``string:wrap([width[, opts]])``

  Inserts newlines into a string so no individual line exceeds the given width.
  Lines are split at space-separated word boundaries. Any existing newlines are
  kept in place. If a single word is longer than width, it is split over
  multiple lines. If ``width`` is not specified, 72 is used. The ``opts``
  parameter can be a table with the following boolean fields specified:

    :return_as_table: if ``true``, then the function will return a table of
        strings, with each string representing one wrapped line. Otherwise, a
        single multi-line string is returned.
    :keep_trailing_spaces: if ``true``, then spaces at the end of a wrapped
        line will be kept. Normally, spaces at the end of a wrapped line are
        elided.
    :keep_original_newlines: if ``true`` (and ``return_as_table`` is also
        ``true``), then if a newline was encountered in the original string, it
        will be included in the relevant table entry.

* ``string:escape_pattern()``

  Escapes regex special chars in a string. E.g., ``'a+b'`` -> ``'a%+b'``.

.. _script-manager:

script-manager
==============

This module contains functions useful for mods that contain DFHack scripts to
retrieve source and state paths. The value to pass as ``mod_id`` must be the
same as the mod ID in the mod's :file:`info.txt` metadata file. The returned
paths will be relative to the top level game directory and will end in a slash
(``/``).

* ``scriptmanager.getModSourcePath(mod_id)``

  Retrieve the source directory path for the mod with the given ID or ``nil``
  if the mod cannot be found. If multiple versions of a mod are found, the path
  for the version loaded by the current world is used. If the current world
  does not have the mod loaded (or if a world is not currently loaded) then the
  path for the most recent version of the mod is returned. Example::

      local scriptmanager = require('script-manager')
      local path = scriptmanager.getModSourcePath('my_awesome_mod')
      print(path)

  Which would print something like: ``mods/2945575779/`` or
  ``data/installed_mods/my_awesome_mod (108)/``, depending on where the mod is
  being loaded from.

* ``scriptmanager.getModStatePath(mod_id)``

  Retrieve the directory path where a mod with the given ID should store its
  persistent state. Example::

      local json = require('json')
      local scriptmanager = require('script-manager')
      local path = scriptmanager.getModStatePath('my_awesome_mod')
      config = config or json.open(path .. 'settings.json')

  Which would open ``dfhack-config/mods/my_awesome_mod/settings.json``. After
  calling ``getModStatePath``, the returned directory is guaranteed to exist.

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

* ``utils.invert(table)``

  Returns a table where keys and values are reversed (i.e., a table containing a
  ``value = key`` entry for every ``key = value`` entry in the argument).

* ``utils.tabulate(fun, start, stop[, step])``

  For numbers ``start``, ``stop`` and ``step``, with ``step`` defaulting to 1,
  returns a lua sequence ``{ fun(start), fun(start+step), ... , fun(stop) }``.

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

  Removes the item with the given key from the list.
  Returns: *did_erase, vector[idx], idx*.

* ``utils.erase_sorted(vector,item,field,cmpfun)``

  Exactly like ``erase_sorted_key``, but if field is specified,
  takes the key from ``item[field]``.

* ``utils.search_text(text,search_tokens)``

  Returns true if all the search tokens are found within ``text``. The text and
  search tokens are normalized to lower case and special characters (e.g., ``A``
  with a circle on it) are converted to their "basic" forms (e.g., ``a``).
  ``search_tokens`` can be a string or a table of strings. If it is a string,
  it is split into space-separated tokens before matching. The search tokens
  are treated literally, so any special regular expression characters do not
  need to be escaped. If ``utils.FILTER_FULL_TEXT`` is ``true``, then the
  search tokens can match any part of ``text``. If it is ``false``, then the
  matches must happen at the beginning of words within ``text``. You can change
  the value of ``utils.FILTER_FULL_TEXT`` in `gui/control-panel` on the
  "Preferences" tab.

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

argparse
========

The ``argparse`` module provides functions to help scripts process commandline
parameters.

* ``argparse.processArgs(args, validArgs)``

  A basic commandline processing function with simple syntax, useful if your
  script doesn't need the more advanced features of
  ``argparse.processArgsGetopt()``.

  If ``validArgs`` is specified, it should contain a set of valid option names
  (without the leading dashes). For example::

    argparse.processArgs(args, utils.invert{'opt1', 'opt2', 'opt3'})

  ``processArgs`` returns a map of option names it found in ``args`` to:

  - the token that came after the option
  - ``''`` if the next token was another option
  - a list of strings if the next token was ``'['`` (see below)

  Options in ``args`` from the commandline can be prefixed with either one dash
  (``'-'``) or two dashes (``'--'``). The user can add a backslash before the
  dash to allow a string to be identified as an option value instead of another
  option. For example: ``yourscript --opt1 \-arg1``.

  If a ``'['`` token is found in ``args``, the subsequent tokens will be
  interpreted as elements of a list until the matching closing ``']'`` is found.
  Brackets can be nested, but the inner brackets will be added to the list of
  tokens as literal ``'['`` and ``']'`` strings.

  Example commandlines::

    yourscript --optName --opt2
    yourscript --optName value
    yourscript --optName [ list of values ]
    yourscript --optName [ list of [ nested values ] [ in square brackets ] ]
    yourscript --optName \--value

  Note that ``processArgs`` does not support non-option ("positional")
  parameters. They are supported by ``processArgsGetopt`` (see below).

* ``argparse.processArgsGetopt(args, optionActions)``

  A fully-featured commandline processing function, with behavior based on the
  popular ``getopt`` library. You would use this instead of the simpler
  ``processArgs`` function if any of the following are true:

  * You want both short (e.g., ``-f``) and aliased long-form (e.g.,
    ``--filename``) options
  * You have commandline components that are not arguments to options (e.g., you
    want to run your script like ``yourscript command --verbose arg1 arg2 arg3``
    instead of
    ``yourscript command --verbose --opt1 arg1 --opt2 arg2 --opt3 arg3)``.
  * You want the convenience of combining options into shorter strings (e.g.,
    ``'-abcarg'`` instead of ``'-a -b -c arg``)
  * You want to be able to parse and validate the option arguments as the
    commandline is being processed, as opposed to validating everything after
    commandline processing is complete.

  Commandlines processed by ``processArgsGetopt`` can have both "short" and
  "long" options, with each short option often having a long-form alias that
  behaves exactly the same as the short form. Short options have properties that
  make them very easy to type quickly by users who are familiar with your script
  options. Long options, on the other hand, are easily understandable by
  everyone and are useful in places where clarity is more important than
  brevity, e.g., in example commands. Each option can be configured to take an
  argument, which will be the string token that follows the option name on the
  commandline.

  Short options are a single letter long and are specified on a commandline by
  prefixing them with a single dash (e.g., the short option ``a`` would appear
  on the commandline as ``-a``). Multiple successive short options that do not
  take arguments can be combined into a single option string (e.g., ``'-abc'``
  instead of ``'-a -b -c'``). Moreover, the argument for a short option can be
  appended directly to the single-letter option without an intervening space
  (e.g., ``-d param`` can be written as ``-dparam``). These two convenience
  shorthand forms can be combined, allowing groups of short parameters to be
  written together, as long as at most the last short option takes an argument
  (e.g., combining the previous two examples into ``-abcdparam``)

  Long options focus on clarity. They are usually entire words, or several words
  combined with hyphens (``-``) or underscores (``_``). If they take an
  argument, the argument can be separated from the option name by a space or an
  equals sign (``=``). For example, the following two commandlines are
  equivalent: ``yourscript --style pretty`` and ``yourscript --style=pretty``.

  Another reason to use long options is if they represent an esoteric parameter
  that you don't expect to be commonly used and that you don't want to "waste" a
  single-letter option on. In this case, you can define a long option without a
  corresponding short option.

  ``processArgsGetopt`` takes two parameters::

      args: list of space-separated strings the user wrote on the commandline
      optionActions: list of option specifications

  and returns a list of positional parameters -- that is, all strings that are
  neither options nor arguments to options. Options and positional parameters
  can appear in any order on the commandline, as long as arguments to options
  immediately follow the option itself.

  Each option specification in ``optionActions`` has the following format:
  ``{shortOptionName, longOptionAlias, hasArg=boolean, handler=fn}``

  * ``shortOptionName`` is a one-character string (or ``''`` or ``nil`` if the
    parameter only has a long form). Numbers cannot be short options, and
    negative numbers (e.g., ``'-10'``) will be interpreted as positional
    parameters and returned in the positional parameters list.
  * ``longOptionAlias`` is an optional longer form of the short option name. If
    no short option name is specified, then this element is required.
  * ``hasArg`` indicates whether the handler function for the option takes a
    parameter.
  * ``handler`` is the handler function for the option. If ``hasArg`` is
    ``true`` then the next token on the commandline is passed to the handler
    function as an argument.

  Example usage::

    local args = {...}
    local open_readonly, filename = false, nil     -- set defaults

    local positionals = argparse.processArgsGetopt(args, {
      {'r', handler=function() open_readonly = true end},
      {'f', 'filename', hasArg=true,
       handler=function(optarg) filename = optarg end}
      })

  In this example, if ``args`` is ``{'first', '-rf', 'fname', 'second'}`` or,
  equivalently, ``{'first', '-r', '--filename', 'myfile.txt', 'second'}`` (note
  the double dash in front of the long option alias), then ``open_readonly``
  will be ``true``, ``filename`` will be ``'myfile.txt'`` and ``positionals``
  will be ``{'first', 'second'}``.

* ``argparse.stringList(arg, arg_name, list_length)``

  Parses a comma-separated sequence of strings and returns a lua list. Leading
  and trailing spaces are trimmed from the strings. If ``arg_name`` is
  specified, it is used to make error messages more useful. If ``list_length``
  is specified and greater than ``0``, then exactly that number of elements must
  be found or the function will error. Example::

    stringList('hello , world,alist', 'words') => {'hello', 'world', 'alist'}

* ``argparse.numberList(arg, arg_name, list_length)``

  Parses a comma-separated sequence of numeric strings and returns a list of
  the discovered numbers (as numbers, not strings). If ``arg_name`` is
  specified, it is used to make error messages more useful. If ``list_length``
  is specified and greater than ``0``, exactly that number of elements must be
  found or the function will error. Example::

    numberList('10, -20 ,  30.5') => {10, -20, 30.5}

* ``argparse.coords(arg, arg_name, skip_validation)``

  Parses a comma-separated coordinate string and returns a coordinate table of
  ``{x, y, z}``. If the string ``'here'`` is passed, returns the coordinates of
  the active game cursor, or throws an error if the cursor is not active. This
  function also verifies that the coordinates are valid for the current map and
  throws if they are not (unless ``skip_validation`` is set to true).

* ``argparse.positiveInt(arg, arg_name)``

  Throws if ``tonumber(arg)`` is not a positive integer; otherwise returns
  ``tonumber(arg)``. If ``arg_name`` is specified, it is used to make error
  messages more useful.

* ``argparse.nonnegativeInt(arg, arg_name)``

  Throws if ``tonumber(arg)`` is not a non-negative integer; otherwise returns
  ``tonumber(arg)``. If ``arg_name`` is specified, it is used to make error
  messages more useful.

* ``argparse.boolean(arg, arg_name)``

  Converts ``string.lower(arg)`` from "yes/no/on/off/true/false/etc..." to a lua
  boolean. Throws if the value can't be converted, otherwise returns
  ``true``/``false``. If ``arg_name`` is specified, it is used to make error
  messages more useful.

dumper
======

A third-party lua table dumper module from
http://lua-users.org/wiki/DataDumper. Defines one
function:

* ``dumper.DataDumper(value, varname, fastmode, ident, indent_step)``

  Returns ``value`` converted to a string. The ``indent_step``
  argument specifies the indentation step size in spaces. For
  the other arguments see the original documentation link above.

.. _helpdb:

helpdb
======

Unified interface for DFHack tool help text. Help text is read from the rendered
text in ``hack/docs/docs/tools``. If no rendered text exists, help is read from
the script sources (for scripts) or the string passed to the ``PluginCommand``
initializer (for plugins). See `documentation` for details on how DFHack's help
system works.

The database is loaded when DFHack initializes, but can be explicitly refreshed
with a call to ``helpdb.refresh()`` if docs are added/changed during a play
session.

Each entry has several properties associated with it:

- The entry name, which is the name of a plugin, script, or command provided by
  a plugin.
- The entry types, which can be ``builtin``, ``plugin``, and/or ``command``.
  Entries for built-in commands (like ``ls`` or ``quicksave``) are both type
  ``builtin`` and ``command``. Entries named after plugins are type ``plugin``,
  and if that plugin also provides a command with the same name as the plugin,
  then the entry is also type ``command``. Entry types are returned as a map
  of one or more of the type strings to ``true``.
- Short help, a the ~54 character description string.
- Long help, the entire contents of the associated help file.
- A list of tags that define the groups that the entry belongs to.

* ``helpdb.refresh()``

  Scan for changes in available commands and their documentation.

* ``helpdb.is_entry(str)``, ``helpdb.is_entry(list)``

  Returns whether the given string (or list of strings) is an entry (are all
  entries) in the db.

* ``helpdb.get_entry_types(entry)``

  Returns the set (that is, a map of string to ``true``) of entry types for the
  given entry.

* ``helpdb.get_entry_short_help(entry)``

  Returns the short (~54 character) description for the given entry.

* ``helpdb.get_entry_long_help(entry[, width])``

  Returns the full help text for the given entry. If ``width`` is specified, the
  text will be wrapped at that width, preserving block indents. The wrap width
  defaults to 80.

* ``helpdb.get_entry_tags(entry)``

  Returns the set of tag names for the given entry.

* ``helpdb.has_tag(entry, tag)``

  Returns whether the given entry exists and has the specified tag.

* ``helpdb.is_tag(str)``, ``helpdb.is_tag(list)``

  Returns whether the given string (or list of strings) is a (are all) valid tag
  name(s).

* ``helpdb.get_tags()``

  Returns the full alphabetized list of valid tag names.

* ``helpdb.get_tag_data(tag)``

  Returns a list of entries that have the given tag. The returned table also
  has a ``description`` key that contains the string description of the tag.

* ``helpdb.search_entries([include[, exclude]])``

  Returns a list of names for entries that match the given filters. The list is
  alphabetized by their last path component, with populated path components
  coming before null path components (e.g., ``autobutcher`` will immediately
  follow ``gui/autobutcher``).
  The optional ``include`` and ``exclude`` filter params are maps (or lists of
  maps) with the following elements:

  :str:   if a string, filters by the given substring. if a table of strings,
          includes entry names that match any of the given substrings.
  :tag:   if a string, filters by the given tag name. if a table of strings,
          includes entries that match any of the given tags.
  :entry_type: if a string, matches entries of the given type. if a table of
          strings, includes entries that match any of the given types.

  Elements in a map are ANDed together (e.g., if both ``str`` and ``tag`` are
  specified, the match is on any of the ``str`` elements AND any of the ``tag``
  elements).

  If lists of filters are passed instead of a single map, the match succeeds if
  all of the filters match.

  If ``include`` is ``nil`` or empty, then all entries are included. If
  ``exclude`` is ``nil`` or empty, then no entries are filtered out.

profiler
========

A third-party lua profiler module from
http://lua-users.org/wiki/PepperfishProfiler. Module defines one function to
create profiler objects which can be used to profile and generate report.

* ``profiler.newProfiler([variant[, sampling_frequency]])``

  Returns a profile object with ``variant`` either ``'time'`` or ``'call'``.
  ``'time'`` variant takes optional ``sampling_frequency`` parameter to select
  lua instruction counts between samples. Default is ``'time'`` variant with
  ``10*1000`` frequency.

  ``'call'`` variant has much higher runtime cost which will increase the
  runtime of profiled code by factor of ten. For the extreme costs it provides
  accurate function call counts that can help locate code which takes much time
  in native calls.

* ``obj:start()``

  Resets collected statistics. Then it starts collecting new statistics.

* ``obj:stop()``

  Stops profile collection.

* ``obj:report(outfile[, sort_by_total_time])``

  Write a report from previous statistics collection to ``outfile``.
  ``outfile`` should be writeable io file object (``io.open`` or
  ``io.stdout``). Passing ``true`` as second parameter ``sort_by_total_time``
  switches sorting order to use total time instead of default self time order.

* ``obj:prevent(function)``

  Adds an ignore filter for a ``function``. It will ignore the pointed function
  and all of it children.

Examples
--------

::

    local prof = profiler.newProfiler()
    prof:start()

    profiledCode()

    prof:stop()

    local out = io.open( "lua-profile.txt", "w+")
    prof:report(out)
    out:close()

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

  Declares certain instance fields to be attributes, i.e., auto-initialized
  from fields in the table used as the constructor argument. If omitted,
  they are initialized with the default values specified in this declaration.

  If the default value should be *nil*, use ``ATTRS { foo = DEFAULT_NIL }``.

  Declaring an attribute is mostly the same as defining your ``init`` method
  like this::

    function Class.init(args)
        self.attr1 = args.attr1 or default1
        self.attr2 = args.attr2 or default2
        ...
    end

  The main difference is that attributes are processed as a separate
  initialization step, before any ``init`` methods are called. They
  also make the direct relation between instance fields and constructor
  arguments more explicit.

* ``new_obj = Class{ foo = arg, bar = arg, ... }``

  Calling the class as a function creates and initializes a new instance.
  Initialization happens in this order:

  1. An empty instance table is created, and its metatable set.
  2. The ``preinit`` methods are called via ``invoke_before`` (see below)
     with the table used as the argument to the class. These methods are
     intended for validating and tweaking that argument table.
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
  i.e., invocations happen in the parent to child order.

  These two methods are inspired by the Common Lisp before and after methods, and
  are intended for implementing similar protocols for certain things. The class
  library itself uses them for constructors.

To avoid confusion, these methods cannot be redefined.

.. _custom-raw-tokens:

custom-raw-tokens
=================

A module for reading custom tokens added to the raws by mods.

* ``customRawTokens.getToken(typeDefinition, token)``

  Where ``typeDefinition`` is a type definition struct as seen in
  ``df.global.world.raws`` (e.g.: ``dfhack.gui.getSelectedItem().subtype``)
  and ``token`` is the name of the custom token you want read. The arguments
  from the token will then be returned as strings using single or multiple
  return values. If the token is not present, the result is false; if it is present
  but has no arguments, the result is true. For ``creature_raw``, it checks against
  no caste. For ``plant_raw``, it checks against no growth.

* ``customRawTokens.getToken(typeInstance, token)``

  Where ``typeInstance`` is a unit, entity, item, job, projectile, building,
  plant, or interaction instance. Gets ``typeDefinition`` and then returns the same
  as ``getToken(typeDefinition, token)``. For units, it gets the token from the race
  or caste instead if applicable. For plant growth items, it gets the token from the
  plant or plant growth instead if applicable. For plants it does the same but with
  growth number -1.

* ``customRawTokens.getToken(raceDefinition, casteNumber, token)``

  The same as ``getToken(unit, token)`` but with a specified race and caste.
  Caste number -1 is no caste.

* ``customRawTokens.getToken(raceDefinition, casteName, token)``

  The same as ``getToken(unit, token)`` but with a specified race and caste, using
  caste name (e.g., "FEMALE") instead of number.

* ``customRawTokens.getToken(plantDefinition, growthNumber, token)``

  The same as ``getToken(plantGrowthItem, token)`` but with a specified plant and
  growth. Growth number -1 is no growth.

* ``customRawTokens.getToken(plantDefinition, growthName, token)``

  The same as ``getToken(plantGrowthItem, token)`` but with a specified plant and
  growth, using growth name (e.g., "LEAVES") instead of number.

It is recommended to prefix custom raw tokens with the name of your mod to avoid
duplicate behaviour where two mods make callbacks that work on the same tag.

Examples:

* Using an eventful onReactionComplete hook, something for disturbing dwarven science::

    if customRawTokens.getToken(reaction, "EXAMPLE_MOD_CAUSES_INSANITY") then
        -- make unit who performed reaction go insane

* Using an eventful onProjItemCheckMovement hook, a fast or slow-firing crossbow::

    -- check projectile distance flown is zero, get firer, etc...
    local multiplier = tonumber(customRawTokens.getToken(bow, "EXAMPLE_MOD_FIRE_RATE_MULTIPLIER")) or 1
    if firer.counters.think_counter > 0 then
      firer.counters.think_counter = math.max(math.floor(firer.counters.think_counter * multiplier), 1)
    end

* Something for a script that prints help text about different types of units::

    local unit = dfhack.gui.getSelectedUnit()
    if not unit then return end
    local helpText = customRawTokens.getToken(unit, "EXAMPLE_MOD_HELP_TEXT")
    if helpText then print(helpText) end

* Healing armour::

    -- (per unit every tick)
    local healAmount = 0
    for _, entry in ipairs(unit.inventory) do
        if entry.mode == 2 then -- Worn
            healAmount = healAmount + tonumber((customRawTokens.getToken(entry.item, "EXAMPLE_MOD_HEAL_AMOUNT")) or 0)
        end
    end
    unit.body.blood_count = math.min(unit.body.blood_max, unit.body.blood_count + healAmount)

.. _lua-ui-library:

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

* ``CLEAR_PEN``

  The black pen used to clear the screen. In graphics mode, it will clear the
  foreground and set the background to the standard black tile.

* ``TRANSPARENT_PEN``

  A pen that will clear all textures from the UI layer, making the tile transparent.

* ``KEEP_LOWER_PEN``

  A pen that will write tiles over existing background tiles instead of clearing
  them.

* ``simulateInput(screen, keys...)``

  This function wraps an undocumented native function that passes a set of
  keycodes to a screen, and is the official way to do that.

  Every argument after the initial screen may be *nil*, a numeric keycode,
  a string keycode, a sequence of numeric or string keycodes, or a mapping
  of keycodes to *true* or *false*. For instance, it is possible to use the
  table passed as argument to ``onInput``. The ``_STRING`` convenience field of
  an ``onInput`` keys table will be ignored; the presence (or absence) of a
  ``STRING_A???`` keycode will determine the text content of the simulated
  input.

  You can send mouse clicks as well by setting the ``_MOUSE_L`` key or other
  mouse-related pseudo-keys documented with the ``screen:onInput(keys)``
  function above. Note that if you are simulating a click at a specific spot on
  the screen, you must set ``df.global.gps.mouse_x`` and
  ``df.global.gps.mouse_y`` if you are clicking on the interface layer or
  ``df.global.gps.precise_mouse_x`` and ``df.global.gps.precise_mouse_y`` if
  you are clicking on the map.

* ``mkdims_xy(x1,y1,x2,y2)``

  Returns a table containing the arguments as fields, and also ``width`` and
  ``height`` that contains the rectangle dimensions.

* ``mkdims_wh(x1,y1,width,height)``

  Returns the same kind of table as ``mkdims_xy``, only this time it computes
  ``x2`` and ``y2``.

* ``get_interface_rect()``

  Returns the table rect (as per ``mkdims_xy``) for the interface area of the
  screen, respecting the player's setting for ``max_interface_percentage``.

* ``get_interface_frame()``

  Returns the frame (as per `Widget class`_) for configuring a ``Widget`` with
  a body that represents the interface area.

* ``is_in_rect(rect,x,y)``

  Checks if the given point is within a rectangle, represented by a table produced
  by one of the ``mkdims`` functions.

* ``blink_visible(delay)``

  Returns *true* or *false*, with the value switching to the opposite every ``delay``
  msec. This is intended for rendering blinking interface objects.

* ``getKeyDisplay(keycode)``

  Wraps ``dfhack.screen.getKeyDisplay`` in order to allow using strings for
  the keycode argument.


* ``invert_color(color, bold)``

  This inverts the brightness of ``color``. If this color is coming from a pen's
  foreground color, include ``pen.bold`` in ``bold`` for this to work properly.


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

  Returns *true* if the clip area is empty, i.e., no painting is possible.

* ``rect:inClipGlobalXY(x,y)``

  Checks if these global coordinates are within the clip rectangle.

* ``rect:inClipLocalXY(x,y)``

  Checks if these coordinates (specified relative to ``x1,y1``) are within the
  clip rectangle.

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
coordinates, and tracks current cursor position and current pen. It also
supports drawing to a separate map buffer if applicable (see ``map()`` below
for details).

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

  Returns the current cursor *x,y* in screen coordinates.

* ``painter:cursorX()``

  Returns just the current *x* cursor coordinate

* ``painter:cursorY()``

  Returns just the current *y* cursor coordinate

* ``painter:seek(x,y)``

  Sets the current cursor position, and returns *self*.
  Either of the arguments may be *nil* to keep the current value.

* ``painter:advance(dx,dy)``

  Adds the given offsets to the cursor position, and returns *self*.
  Either of the arguments may be *nil* to keep the current value.

* ``painter:newline([dx])``

  Advances the cursor to the start of the next line plus the given x offset,
  and returns *self*.

* ``painter:pen(...)``

  Sets the current pen to ``dfhack.pen.parse(old_pen,...)``, and returns *self*.

* ``painter:color(fg[,bold[,bg]])``

  Sets the specified colors of the current pen and returns *self*.

* ``painter:key_pen(...)``

  Sets the current keybinding pen to ``dfhack.pen.parse(old_pen,...)``,
  and returns *self*.

* ``painter:map(to_map)``

  Enables or disables drawing to a separate map buffer. ``to_map`` is a boolean
  that will be passed as the ``map`` parameter to any ``dfhack.screen`` functions
  that accept it. Note that only third-party plugins like TWBT currently implement
  a separate map buffer; if none are enabled, this function has no effect (but
  should still be used to ensure proper support for such plugins). Returns *self*.

* ``painter:clear()``

  Fills the whole clip rectangle with ``CLEAR_PEN``, and returns *self*.

* ``painter:fill(x1,y1,x2,y2[,...])`` or ``painter:fill(rect[,...])``

  Fills the specified local coordinate rectangle with
  ``dfhack.pen.parse(cur_pen,...)``, and returns *self*.

* ``painter:char([char[, ...]])``

  Paints one character using ``char`` and ``dfhack.pen.parse(cur_pen,...)``.
  Returns *self*. The ``char`` argument, if not nil, is used to override the
  ``ch`` property of the pen.

* ``painter:tile([char, tile[, ...]])``

  Like ``char()`` above, but also allows overriding the ``tile`` property on
  ad-hoc basis.

* ``painter:string(text[, ...])``

  Paints the string with ``dfhack.pen.parse(cur_pen,...)``; returns *self*.

* ``painter:key(keycode[, ...])``

  Paints the description of the keycode using ``dfhack.pen.parse(cur_key_pen,...)``.
  Returns *self*.

* ``painter:key_string(keycode, text, ...)``

  A convenience wrapper around both ``key()`` and ``string()`` that prints both
  the specified keycode description and text, separated by ``:``. Any extra
  arguments are passed directly to ``string()``. Returns *self*.

Unless specified otherwise above, all Painter methods return *self*, in order to
allow chaining them like this::

  painter:pen(foo):seek(x,y):char(1):advance(1):string('bar')...


View class
----------

This class is the common abstract base of both the stand-alone screens
and common widgets to be used inside them. It defines the basic layout,
rendering and event handling framework.

The class defines the following attributes:

:visible: Specifies that the view should be painted. This can be a boolean or
          a function that returns a boolean.
:active: Specifies that the view should receive events, if also visible. This
         can be a boolean or a function that returns a boolean.
:view_id: Specifies an identifier to easily identify the view among subviews.
          This is reserved for use by script writers and should not be set by
          library widgets for their internal subviews.
:on_focus: Called when the view gains keyboard focus; see ``setFocus()`` below.
:on_unfocus: Called when the view loses keyboard focus.

It also always has the following fields:

:subviews: Contains a table of all subviews. The sequence part of the
           table is used for iteration. In addition, subviews are also
           indexed under their ``view_id``, if any; see ``addviews()`` below.
:parent_view: A reference to the parent view. This field is ``nil`` until the
              view is added as a subview to another view with ``addviews()``.
:focus_group: The list of widgets in a hierarchy. This table is unique and empty
              when a view is initialized, but is replaced by a shared table when
              the view is added to a parent via ``addviews()``. If a view in the
              focus group has keyboard focus, that widget can be accessed via
              ``focus_group.cur``.
:focus: A boolean indicating whether the view currently has keyboard focus.

These fields are computed by the layout process:

:frame_parent_rect: The ViewRect representing the client area of the parent view.
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

* ``view:getMousePos([view_rect])``

  Returns the mouse *x,y* in coordinates local to the given ViewRect (or
  ``frame_body`` if no ViewRect is passed) if it is within its clip area, or
  nothing otherwise.

* ``view:getMouseFramePos()``

  Returns the mouse *x,y* in coordinates local to ``frame_rect`` if it is
  within its clip area, or nothing otherwise.

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
  Returns ``true`` if any of the subviews handled the event. If a subview within
  the view's ``focus_group`` has focus and it and all of its ancestors are
  active and visible, that subview is offered the chance to handle the input
  before any other subviews.

* ``view:getPreferredFocusState()``

  Returns ``false`` by default, but should be overridden by subclasses that may
  want to take keyboard focus (if it is unclaimed) when they are added to a
  parent view with ``addviews()``.

* ``view:setFocus(focus)``

  Sets the keyboard focus to the view if ``focus`` is ``true``, or relinquishes
  keyboard focus if ``focus`` is ``false``. Views that newly acquire keyboard
  focus will trigger the ``on_focus`` callback, and views that lose keyboard
  focus will trigger the ``on_unfocus`` callback. While a view has focus, all
  keyboard input is sent to that view before any of its siblings or parents.
  Keyboard input is propagated as normal (see ``inputToSubviews()`` above) if
  there is no view with focus or if the view with focus returns ``false`` from
  its ``onInput()`` function.

.. _lua-gui-screen:

Screen class
------------

This is a View subclass intended for use as a stand-alone modal dialog or screen.
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

  Asks the parent native screen to render itself, or clears the screen
  if impossible.

* ``screen:sendInputToParent(...)``

  Uses ``simulateInput`` to send keypresses to the native parent screen.

* ``screen:show([parent])``

  Adds the screen to the display stack with the given screen as the parent;
  if parent is not specified, places this one one topmost. Before calling
  ``dfhack.screen.show``, calls ``self:onAboutToShow(parent)``. Note that
  ``onAboutToShow()`` can dismiss active screens, and therefore change the
  potential parent. If parent is not specified, this function will re-detect
  the current topmost window after ``self:onAboutToShow(parent)`` returns.
  This function returns ``self`` as a convenience so you can write such code
  as ``local view = MyScreen{params=val}:show()``.

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

ZScreen class
-------------

A screen subclass that allows multi-layer interactivity. For example, a DFHack
GUI tool implemented as a ZScreen can allow the player to interact with the
underlying map, or even other DFHack ZScreen windows! That is, even when the
DFHack tool window is visible, players will be able to use vanilla designation
tools, select units, and scan/drag the map around.

At most one ZScreen can have input focus at a time. That ZScreen's widgets
will have a chance to handle the input before anything else. If unhandled, the
input skips all unfocused ZScreens under that ZScreen and is passed directly to
the first non-ZScreen viewscreen. There are class attributes that can be set to
control what kind of unhandled input is passed to the lower layers.

If multiple ZScreens are visible and the player scrolls or left/right clicks on
a visible element of a non-focused ZScreen, that ZScreen will be given focus.
This allows multiple DFHack GUI tools to be usable at the same time. If the
mouse is clicked away from the ZScreen widgets, that ZScreen loses focus. If no
ZScreen has focus, all input is passed directly through to the first underlying
non-ZScreen viewscreen.

For a ZScreen with keyboard focus, if :kbd:`Esc` or the right mouse button is
pressed, and the ZScreen widgets don't otherwise handle them, then the ZScreen
is dismissed.

All this behavior is implemented in ``ZScreen:onInput()``, which subclasses
**must not override**. Instead, ZScreen subclasses should delegate all input
processing to subviews. Consider using a `Window class`_ widget subview as your
top level input processor.

When rendering, the parent viewscreen is automatically rendered first, so
subclasses do not have to call ``self:renderParent()``. Calls to ``logic()``
(a world "tick" when playing the game) are also passed through, so the game
progresses normally and can be paused/unpaused as normal by the player. Note
that passing ``logic()`` calls through to the underlying map is required for
allowing the player to drag the map with the mouse. ZScreen subclasses can set
attributes that control whether the game is paused when the ZScreen is shown and
whether the game is forced to continue being paused while the ZScreen is shown.
If pausing is forced, child ``Window`` widgets will show a force-pause indicator
to show which tool is forcing the pausing.

ZScreen provides the following functions:

* ``zscreen:raise()``

  Raises the ZScreen to the top of the viewscreen stack, gives it keyboard
  focus, and returns a reference to ``self``. A common pattern is to check if
  a tool dialog is already active when the tool command is run and raise the
  existing dialog if it exists or show a new dialog if it doesn't. See the
  sample code below for an example.

* ``zscreen:isMouseOver()``

  The default implementation iterates over the direct subviews of the ZScreen
  subclass (which usually only includes a single Window subview) and sees if
  ``getMouseFramePos()`` returns a position for any of them. Subclasses can
  override this function if that logic is not appropriate.

* ``zscreen:hasFocus()``

  Whether the ZScreen has keyboard focus. Subclasses will generally not need to
  check this because they can assume if they are getting input, then they have
  focus.

ZScreen subclasses can set the following attributes:

* ``defocusable`` (default: ``true``)

  Whether the ZScreen loses keyboard focus when the player clicks on an area
  of the screen other than the tool window. If the player clicks on a different
  ZScreen window, focus still transfers to that other ZScreen.

* ``defocused`` (default: ``false``)

  Whether the ZScreen starts in a defocused state.

* ``initial_pause`` (default: ``DEFAULT_INITIAL_PAUSE or not pass_mouse_clicks``)

  Whether to pause the game when the ZScreen is shown. If not explicitly set,
  this attribute will be true if the system-wide ``DEFAULT_INITIAL_PAUSE`` is
  ``true`` (which is its default value) or if the ``pass_mouse_clicks`` attribute
  is ``false`` (see below). It depends on ``pass_mouse_clicks`` because if the
  player normally pauses/unpauses the game with the mouse, they will not be able
  to pause the game like they usually do while the ZScreen has focus.
  ``DEFAULT_INITIAL_PAUSE`` can be customized permanently via `gui/control-panel`
  or set for the session by running a command like::

    :lua require('gui.widgets').DEFAULT_INITIAL_PAUSE = false

* ``force_pause`` (default: ``false``)

  Whether to ensure the game *stays* paused while the ZScreen is shown,
  regardless of whether it has input focus.

* ``pass_pause`` (default: ``true``)

  Whether to pass the pause key to the lower viewscreens if it is not handled
  by this ZScreen.

* ``pass_movement_keys`` (default: ``false``)

  Whether to pass the map movement keys to the lower viewscreens if they are not
  handled by this ZScreen.

* ``pass_mouse_clicks`` (default: ``true``)

  Whether to pass mouse clicks to the lower viewscreens if they are not handled
  by this ZScreen.

Here is an example skeleton for a ZScreen tool window::

    local gui = require('gui')
    local widgets = require('gui.widgets')

    MyWindow = defclass(MyWindow, widgets.Window)
    MyWindow.ATTRS {
        frame_title='My Window',
        frame={w=50, h=45},
        resizable=true, -- if resizing makes sense for your dialog
        resize_min={w=50, h=20}, -- try to allow users to shrink your windows
    }

    function MyWindow:init()
        self:addviews{
            -- add subview widgets here
        }
    end

    -- implement if you need to handle custom input
    --function MyWindow:onInput(keys)
    --    return MyWindow.super.onInput(self, keys)
    --end

    MyScreen = defclass(MyScreen, gui.ZScreen)
    MyScreen.ATTRS {
        focus_path='myscreen',
        -- set pause and passthrough attributes as appropriate
        -- (but most tools can use the defaults)
    }

    function MyScreen:init()
        self:addviews{MyWindow{}}
    end

    function MyScreen:onDismiss()
        view = nil
    end

    view = view and view:raise() or MyScreen{}:show()

ZScreenModal class
------------------

A ZScreen convenience subclass that sets the attributes to something
appropriate for modal dialogs. The game is force paused, and no input is passed
through to the underlying viewscreens.

gui.widgets
===========

This module implements some basic widgets based on the View infrastructure.

.. _widget:

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
  :h: maximum height of the frame.
  :xalign: X alignment of the frame.
  :yalign: Y alignment of the frame.

  First the ``l,t,r,b`` fields restrict the available area for
  placing the frame. If ``w`` and ``h`` are not specified or
  larger than the computed area, it becomes the frame. Otherwise
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

  Omitted fields are interpreted as having the value of 0.

* ``frame_background = pen``

  The pen to fill the outer frame with. Defaults to no fill.

.. _panel:

Panel class
-----------

Inherits from Widget, and intended for framing and/or grouping subviews. Though
this can be used for your "main window", see the `Window class`_ below for a
more conveniently configured ``Panel`` subclass.

Has attributes:

* ``subviews = {}``

  Used to initialize the subview list in the constructor.

* ``on_render = function(painter)``

  Called from ``onRenderBody``.

* ``on_layout = function(frame_body)``

  Called from ``postComputeFrame``.

* ``draggable = bool`` (default: ``false``)
* ``drag_anchors = {}`` (default: ``{title=true, frame=false/true, body=true}``)
* ``drag_bound = 'frame' or 'body'`` (default: ``'frame'``)
* ``on_drag_begin = function()`` (default: ``nil``)
* ``on_drag_end = function(success, new_frame)`` (default: ``nil``)

  If ``draggable`` is set to ``true``, then the above attributes come into play
  when the panel is dragged around the screen, either with the mouse or the
  keyboard. ``drag_anchors`` sets which parts of the panel can be clicked on
  with the left mouse button to start dragging. The frame is a drag anchor by
  default only if ``resizable`` (below) is ``false``. ``drag_bound`` configures
  whether the frame of the panel (if any) can be dragged outside the containing
  parent's boundary. The body will never be draggable outside of the parent,
  but you can allow the frame to cross the boundary by setting ``drag_bound`` to
  ``'body'``. The boolean passed to the ``on_drag_end`` callback will be
  ``true`` if the drag was "successful" (i.e., not canceled) and ``false``
  otherwise. Dragging can be canceled by right clicking while dragging with the
  mouse, hitting :kbd:`Esc` (while dragging with the mouse or keyboard), or by
  calling ``Panel:setKeyboardDragEnabled(false)`` (while dragging with the
  keyboard). If it is more convenient to do so, you can choose to override the
  ``panel:onDragBegin`` and/or the ``panel:onDragEnd`` methods instead of
  setting the ``on_drag_begin`` and/or ``on_drag_end`` attributes.

* ``resizable = bool`` (default: ``false``)
* ``resize_anchors = {}`` (default: ``{t=false, l=true, r=true, b=true}``
* ``resize_min = {}`` (default: w and h from the ``frame``, or ``{w=5, h=5}``)
* ``on_resize_begin = function()`` (default: ``nil``)
* ``on_resize_end = function(success, new_frame)`` (default: ``nil``)

  If ``resizable`` is set to ``true``, then the player can click the mouse on
  any edge specified in ``resize_anchors`` and drag the border to resize the
  window. If two adjacent edges are enabled as anchors, then the tile where they
  meet can be used to resize both edges at the same time. The minimum dimensions
  specified in ``resize_min`` (or inherited from ``frame`` are respected when
  resizing. The panel is also prevented from resizing beyond the boundaries of
  its parent. When the player clicks on a valid anchor, ``on_resize_begin()`` is
  called. The boolean passed to the ``on_resize_end`` callback will be ``true``
  if the drag was "successful" (i.e., not canceled) and ``false`` otherwise.
  Dragging can be canceled by right clicking while resizing with the mouse,
  hitting :kbd:`Esc` (while resizing with the mouse or keyboard), or by calling
  ``Panel:setKeyboardResizeEnabled(false)`` (while resizing with the keyboard).
  If it is more convenient to do so, you can choose to override the
  ``panel:onResizeBegin`` and/or the ``panel:onResizeEnd`` methods instead of
  setting the ``on_resize_begin`` and/or ``on_resize_end`` attributes.

* ``autoarrange_subviews = bool`` (default: ``false``)
* ``autoarrange_gap = int`` (default: ``0``)

  If ``autoarrange_subviews`` is set to ``true``, the Panel will
  automatically handle subview layout. Subviews are laid out vertically
  according to their current height, with ``autoarrange_gap`` empty lines
  between subviews. This allows you to have widgets dynamically change
  height or become visible/hidden and you don't have to worry about
  recalculating subview positions.

* ``frame_style``, ``frame_title`` (default: ``nil``)

  If defined, a frame will be drawn around the panel and subviews will be
  inset by 1. The following predefined frame styles are defined:

  * ``FRAME_WINDOW``

    A frame suitable for a draggable, optionally resizable window.

  * ``FRAME_PANEL``

    A frame suitable for a static (non-resizable) panel.

  * ``FRAME_MEDIUM``

    A frame suitable for overlay widget panels.

  * ``FRAME_THIN``

    A frame suitable for floating tooltip panels that need the DFHack signature.

  * ``FRAME_BOLD``

    A frame suitable for a non-draggable panel meant to capture the user's
    focus, like an important notification, confirmation dialog or error message.

  * ``FRAME_INTERIOR``

    A frame suitable for light interior accent elements. This frame does *not*
    have a visible ``DFHack`` signature on it, so it must not be used as the
    external frame for a DFHack-owned UI.

  * ``FRAME_INTERIOR_MEDIUM``

    A copy of ``FRAME_MEDIUM`` that lacks the ``DFHack`` signature. Suitable for
    panels that are part of a larger widget cluster. Must *not* be used as the
    external frame for a DFHack-owned UI.

  When using the predefined frame styles in the ``gui`` module, remember to
  ``require`` the gui module and prefix the identifier with ``gui.``, e.g.,
  ``gui.FRAME_THIN``.

* ``no_force_pause_badge`` (default: ``false``)

  If true, then don't display the PAUSE FORCED badge on the frame even if the
  game has been force paused.

Has functions:

* ``panel:setKeyboardDragEnabled(bool)``

  If called with ``true`` and the panel is not already in keyboard drag mode,
  then any current drag or resize operations are halted where they are (not
  canceled), the panel seizes input focus (see `View class`_ above for
  information on the DFHack focus subsystem), and further keyboard cursor keys
  move the window as if it were being dragged. Shift-cursor keys move by larger
  amounts. Hit :kbd:`Enter` to commit the new window position or :kbd:`Esc` to
  cancel. If dragging is canceled, then the window is moved back to its original
  position.

* ``panel:setKeyboardResizeEnabled(bool)``

  If called with ``true`` and the panel is not already in keyboard resize mode,
  then any current drag or resize operations are halted where they are (not
  canceled), the panel seizes input focus (see `View class`_ above for
  information on the DFHack focus subsystem), and further keyboard cursor keys
  resize the window as if it were being dragged from the lower right corner. If
  neither the bottom or right edge is a valid anchor, an appropriate corner will
  be chosen. Shift-cursor keys move by larger amounts. Hit :kbd:`Enter` to
  commit the new window size or :kbd:`Esc` to cancel. If resizing is canceled,
  then the window size from before the resize operation is restored.

* ``panel:onDragBegin()``
* ``panel:onDragEnd(success, new_frame)``
* ``panel:onResizeBegin()``
* ``panel:onResizeEnd(success, new_frame)``

The default implementations of these methods call the associated attribute (if
set). You can override them in a subclass if that is more convenient than
setting the attributes.

Double clicking:

If the panel is resizable and the user double-clicks on the top edge (the frame
title, if the panel has a frame), then the panel will jump to its maximum size.
If the panel has already been maximized in this fashion, then it will jump to
its minimum size. Both jumps respect the resizable edges defined by the
``resize_anchors`` attribute.

The time duration that a double click can span is defined by the global variable
``DOUBLE_CLICK_MS``. The default value is ``500`` and can be changed by the end
user with a command like::

  :lua require('gui.widgets').DOUBLE_CLICK_MS=1000

Window class
------------

Subclass of Panel; sets Panel attributes to useful defaults for a top-level
framed, draggable window.

ResizingPanel class
-------------------

Subclass of Panel; automatically adjusts its own frame height and width to the
minimum required to show its subviews. Pairs nicely with a parent Panel that has
``autoarrange_subviews`` enabled.

It has the following attributes:

:auto_height: Sets self.frame.h from the positions and height of its subviews
              (default is ``true``).
:auto_width: Sets self.frame.w from the positions and width of its subviews
             (default is ``false``).

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

Divider class
-------------

Subclass of Widget; implements a divider line that can optionally connect to
existing frames via T-junction edges. A ``Divider`` instance is required to
have a ``frame`` that is either 1 unit tall or 1 unit wide.

``Divider`` widgets should be a sibling with the framed ``Panel`` that they
are dividing, and they should be added to the common parent widget **after**
the ``Panel`` so that the ``Divider`` can overwrite the ``Panel`` frame  with
the appropriate T-junction graphic. If the ``Divider`` will not have
T-junction edges, then it could potentially be a child of the ``Panel`` since
the ``Divider`` won't need to overwrite the ``Panel``'s frame.

If two ``Divider`` widgets are set to cross, then you must have a third 1x1
``Divider`` widget for the crossing tile so the other two ``Divider``\s can
be seamlessly connected.

Attributes:

* ``frame_style``

    The ``gui`` ``FRAME`` instance to use for the graphical tiles. Defaults to
    ``gui.FRAME_THIN``.

* ``interior``

    Whether the edge T-junction tiles should connect to interior lines (e.g., the
    vertical or horizontal segment of another ``Divider`` instance) or the
    exterior border of a ``Panel`` frame. Defaults to ``false``, meaning
    exterior T-junctions will be chosen.

* ``frame_style_t``
* ``frame_style_b``
* ``frame_style_l``
* ``frame_style_r``

    Overrides for the frame style for specific T-junctions. Note that there are
    not currently any frame styles that allow borders of different weights to be
    seamlessly connected. If set to ``false``, then the indicated edge will end
    in a straight segment instead of a T-junction.

* ``interior_t``
* ``interior_b``
* ``interior_l``
* ``interior_r``

    Overrides for the interior/exterior specification for specific T-junctions.

EditField class
---------------

Subclass of Widget; implements a simple edit field.

Attributes:

:label_text: The optional text label displayed before the editable text.
:text: The current contents of the field.
:text_pen: The pen to draw the text with.
:on_char: Input validation callback; used as ``on_char(new_char,text)``.
          If it returns false, the character is ignored.
:on_change: Change notification callback; used as ``on_change(new_text,old_text)``.
:on_submit: Enter key callback; if set the field will handle the key and call ``on_submit(text)``.
:key: If specified, the field is disabled until this key is pressed. Must be given as a string.
:key_sep: If specified, will be used to customize how the activation key is
          displayed. See ``token.key_sep`` in the ``Label`` documentation below.
:modal: Whether the ``EditField`` should prevent input from propagating to other
        widgets while it has focus. You can set this to ``true``, for example,
        if you don't want a ``List`` widget to react to arrow keys while the
        user is editing.
:ignore_keys: If specified, must be a list of key names that the edit field
              should ignore. This is useful if you have plain string characters
              that you want to use as hotkeys (like ``+``).

An ``EditField`` will only read and process text input if it has keyboard focus.
It will automatically acquire keyboard focus when it is added as a subview to
a parent that has not already granted keyboard focus to another widget. If you
have more than one ``EditField`` on a screen, you can select which has focus by
calling ``setFocus(true)`` on the field object.

If an activation ``key`` is specified, the ``EditField`` will manage its own
focus. It will start in the unfocused state, and pressing the activation key
will acquire keyboard focus. Pressing the Enter key will release keyboard focus
and then call the ``on_submit`` callback. Pressing the Escape key (or r-clicking
with the mouse) will also release keyboard focus, but first it will restore the
text that was displayed before the ``EditField`` gained focus and then call the
``on_change`` callback.

The ``EditField`` cursor can be moved to where you want to insert/remove text.
You can click where you want the cursor to move or you can use any of the
following keyboard hotkeys:

- Left/Right arrow: move the cursor one character to the left or right.
- Ctrl-B/Ctrl-F: move the cursor one word back or forward.
- Ctrl-A/Ctrl-E: move the cursor to the beginning/end of the text.

The widget also supports integration with the system clipboard:

- Ctrl-C: copy current text to the system clipboard
- Ctrl-X: copy current text to the system clipboard and clear text in widget
- Ctrl-V: paste text from the system clipboard (text is converted to cp437)

The ``EditField`` class also provides the following functions:

* ``editfield:setCursor([cursor_pos])``

  Sets the text insert cursor to the specified position. If ``cursor_pos`` is
  not specified or is past the end of the current text string, the cursor will
  be set to the end of the current input (that is, ``#editfield.text + 1``).

* ``editfield:setText(text[, cursor_pos])``

  Sets the input text string and, optionally, the cursor position. If the
  cursor position is not specified, it sets it to the end of the string.

* ``editfield:insert(text)``

  Inserts the given text at the current cursor position.

Scrollbar class
---------------

This Widget subclass implements mouse-interactive scrollbars whose bar sizes
represent the amount of content currently visible in an associated display
widget (like a `Label class`_ or a `List class`_). They are styled like scrollbars
used in vanilla DF.

Scrollbars have the following attributes:

:on_scroll: A callback called when the scrollbar is scrolled. If the scrollbar
  is clicked, the callback will be called with one of the following string parameters:
  "up_large", "down_large", "up_small", or "down_small". If the scrollbar is dragged,
  the callback will be called with the value that ``top_elem`` should be set to on
  the next call to ``update()`` (see below).

The Scrollbar widget implements the following methods:

* ``scrollbar:update(top_elem, elems_per_page, num_elems)``

  Updates the info about the widget that the scrollbar is paired with.
  The ``top_elem`` param is the (one-based) index of the first visible element.
  The ``elems_per_page`` param is the maximum number of elements that can be
  shown at one time. The ``num_elems`` param is the total number of elements
  that the paired widget can scroll through. If ``elems_per_page`` or
  ``num_elems`` is not specified, the most recently specified value for these
  parameters is used. The scrollbar will adjust its scrollbar size and position
  according to the values passed to this function.

Clicking on the arrows at the top or the bottom of a scrollbar will scroll an
associated widget by a small amount. Clicking on the unfilled portion of the
scrollbar above or below the filled area will scroll by a larger amount in that
direction. The amount of scrolling done in each case in determined by the
associated widget, and after scrolling is complete, the associated widget must
call ``scrollbar:update()`` with updated new display info.

If the mouse wheel is scrolled while the mouse is over the Scrollbar widget's
parent view, then the parent is scrolled accordingly. Holding :kbd:`Shift`
while scrolling will result in faster movement.

You can click and drag the scrollbar to scroll to a specific spot, or you can
click and hold on the end arrows or in the unfilled portion of the scrollbar to
scroll multiple times, just like in a normal browser scrollbar. The speed of
scroll events when the mouse button is held down is controlled by two global
variables:

:``SCROLL_INITIAL_DELAY_MS``: The delay before the second scroll event.
:``SCROLL_DELAY_MS``: The delay between further scroll events.

The defaults are 300 and 20, respectively, but they can be overridden by the
user in their :file:`dfhack-config/init/dfhack.init` file, for example::

  :lua require('gui.widgets').SCROLL_DELAY_MS = 100

Label class
-----------

This Widget subclass implements flowing semi-static text.

It has the following attributes:

:text_pen: Specifies the pen for active text.
:text_dpen: Specifies the pen for disabled text.
:text_hpen: Specifies the pen for text hovered over by the mouse, if a click
    handler is registered. By default, this will invert the foreground and
    background colors.
:disabled: Boolean or a callback; if true, the label is disabled.
:enabled: Boolean or a callback; if false, the label is disabled.
:auto_height: Sets self.frame.h from the text height.
:auto_width: Sets self.frame.w from the text width.
:on_click: A callback called when the label is clicked (optional)
:on_rclick: A callback called when the label is right-clicked (optional)
:scroll_keys: Specifies which keys the label should react to as a table. The
    table should map keys to the number of lines to scroll as positive or
    negative integers or one of the keywords supported by the ``scroll``
    method. The default is up/down arrows scrolling by one line and page
    up/down scrolling by one page.

``text_pen``, ``text_dpen``, and ``text_hpen`` can either be a pen or a
function that dynamically returns a pen.

The text itself is represented as a complex structure, and passed
to the object via the ``text`` argument of the constructor, or via
the ``setText`` method, as one of:

* A simple string, possibly containing newlines.
* A sequence of tokens.

Every token in the sequence in turn may be either a string, possibly
containing newlines (or equal to ``NEWLINE``), or a table with the following
possible fields:

* ``token.text = ...``

  Specifies the main text content of a token, and may be a string, or
  a callback returning a string.

* ``token.gap = ...``

  Specifies the number of character positions to advance on the line
  before rendering the token.

* ``token.tile``, ``token.htile``

  Specifies a pen or texture index (or a function that returns a pen or texture
  index) to paint as one tile before the main part of the token. If ``htile``
  is specified, that is used instead of ``tile`` when the Label is hovered over
  with the mouse.

* ``token.width = ...``

  If specified either as a value or a callback, the text (or tile) field is
  padded or truncated to the specified number.

* ``token.pad_char = '?'``

  If specified together with ``width``, the padding area is filled with
  this character instead of just being skipped over.

* ``token.key = '...'``

  Specifies the keycode associated with the token. The string description
  of the key binding is added to the text content of the token.

* ``token.key_sep = '...'``

  Specifies the separator to place between the keybinding label produced
  by ``token.key``, and the main text of the token. If the separator starts with
  '()', the token is formatted as ``text..' ('..binding..sep:sub(2)``. Otherwise
  it is simply ``binding..sep..text``.

* ``token.enabled``, ``token.disabled``

  Same as the attributes of the label itself, but applies only to the token.

* ``token.pen``, ``token.dpen``, ``token.hpen``

  Specify the pen, disabled pen, and hover pen to be used for the token's text.
  The fields may be either the pen itself, or a callback that returns it.

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

* ``label:scroll(nlines)``

  This method takes the number of lines to scroll as positive or negative
  integers or one of the following keywords: ``+page``, ``-page``,
  ``+halfpage``, ``-halfpage``, ``home``, or ``end``. It returns the number of
  lines that were actually scrolled (negative for scrolling up).

* ``label:shouldHover()``

  This method returns whether or not this widget should show a hover effect,
  generally you want to return ``true`` if there is some type of mouse handler
  present. For example, for a ``HotKeyLabel``::

    function HotkeyLabel:shouldHover()
        -- When on_activate is set, text should also hover on mouseover
        return HotkeyLabel.super.shouldHover(self) or self.on_activate
    end

The widgets module also provides the following methods for help in constructing
common text token lists that you can then pass as ``text`` to a ``Label``:

* ``makeButtonLabelText(spec)``

    Returns a list of ``Label`` text tokens that represent a button according
    to the given ``spec``, which is a table with the following fields. Fields
    that contain ``_hover`` are optional and specify alternate values to be
    used when the mouse cursor is hovering over the button.

    - ``chars``, ``chars_hover``: A list of strings or a list of lists of
        characters. These strings (or lists of characters) make up the ASCII
        representation of the button. If a list of strings is passed, each
        string must be the same length. ``chars`` is the only required element
        in the spec. If ``chars_hover`` is not specified, it defaults to the
        value of ``chars``.

    - ``pens``, ``pens_hover``: A color or a pen or a list of lists of colors
        or pens. This controls what color and other pen properties should be
        applied to the corresponding button tile position. If a single color or
        pen is passed, then that color or pen will apply to all tiles of the
        button. If not specified, ``pens`` defaults to ``COLOR_GRAY`` and
        ``pens_hover`` defaults to ``COLOR_WHITE``

    - ``tileset``, ``tileset_hover``: If specified, must be a tileset that was
        returned from ``dfhack.textures.loadTileset``.

    - ``tileset_offset``, ``tileset_hover_offset``: The 1-based offset within
        the tileset to the tile that represents the upper left corner of the
        button. If not specified, defaults to ``1``.

    - ``tileset_stride``, ``tileset_hover_stride``: The number of tiles in one
        row of the tileset. This is used to find the start position of
        subsequent rows of tiles for the button. If not specified, defaults to
        the width of a button row specified in ``chars``, which is appropriate
        for a tileset that has only a single button image per logical row.

    - ``asset``, ``asset_hover``: If specified, must be a table defining a
        graphic asset loaded by DF from the vanilla sprite sheets or a mod. The
        table must indicate which sprite page to read and the x and y offsets
        of the upper left corner of the target asset in the following format:
        ``{page=pagename, x=x_offset, y=y_offset}``.

    - ``tiles_override``, ``tiles_hover_override``: A list of lists of integers
        representing raw tile texpos values to be displayed at the
        corresponding button position. Tiles specified here will override
        corresponding tiles from ``tileset`` and ``asset``. The lists can be
        sparse, so any unspecified values in the override array will fall
        through to other specifiers.

    If no tile is set for a particular button position, the corresponding
    pen is used without setting a ``tile`` value.

    Example 1: The civ-alert button - a text-only (no graphic tiles) button
    that highlights the text on hover::

        widgets.Label{
            text=widgets.makeButtonLabelText{
                chars={
                    ' Activate ',
                    ' civilian ',
                    '  alert   ',
                },
                pens={fg=COLOR_BLACK, bg=COLOR_LIGHTRED},
                pens_hover={fg=COLOR_WHITE, bg=COLOR_RED},
            },
            on_click=sound_alarm,
        },

    Example 2: The DFHack logo - a graphical button in graphics mode and a text
    button in ASCII mode. The ASCII colors use the default for hovering::

        widgets.Label{
            text=widgets.makeButtonLabelText{
                chars={
                    {179, 'D', 'F', 179},
                    {179, 'H', 'a', 179},
                    {179, 'c', 'k', 179},
                },
                tileset=dfhack.textures.loadTileset(
                    'hack/data/art/logo.png', 8, 12, true),
                tileset_hover=dfhack.textures.loadTileset(
                    'hack/data/art/logo_hovered.png', 8, 12, true),
            },
            on_click=function()
                dfhack.run_command{'hotkeys', 'menu', self.name}
            end,
        },

    Example 3: One of the warm/damp toolbar buttons - similar to example 2, but
    with custom colors throughout the button when in ASCII mode::

        widgets.Label{
            text=widgets.makeButtonLabelText{
                chars={
                    {218, 196, 196, 191},
                    {179, '~', '~', 179},
                    {192, 196, 196, 217},
                },
                pens={
                    {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                    {COLOR_WHITE, COLOR_RED,   COLOR_GRAY,  COLOR_WHITE},
                    {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                },
                tileset=toolbar_textures,
                tileset_offset=25,
                tileset_stride=8,
            },
            on_click=launch_warm_damp_dig_config,
        },

    Example 4: A copy of the mining toolbar button (except that it has a
    highlight on hover), loaded from the DF assets::

        widgets.Label{
            text=widgets.makeButtonLabelText{
                chars={
                    {218, 196, 196, 191},
                    {179, '-', ')', 179},
                    {192, 196, 196, 217},
                },
                pens={
                    {COLOR_GRAY, COLOR_GRAY,  COLOR_GRAY, COLOR_GRAY},
                    {COLOR_GRAY, COLOR_BROWN, COLOR_GRAY, COLOR_GRAY},
                    {COLOR_GRAY, COLOR_GRAY,  COLOR_GRAY, COLOR_GRAY},
                },
                pens_hover={
                    {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                    {COLOR_WHITE, COLOR_BROWN, COLOR_GRAY,  COLOR_WHITE},
                    {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                },
                asset={page='INTERFACE_BITS', x=0, y=22},
            },
            on_click=self:callback('mining_menu'),
        },

    Example 5: A copy of the mining toolbar button (except that it has a
    custom hotkey hint in the upper corner), loaded from the DF assets and with
    one tile overridden::

        widgets.Label{
            text=widgets.makeButtonLabelText{
                chars={
                    {218, 196, 196, self.hint_char},
                    {179, '-', ')', 179},
                    {192, 196, 196, 217},
                },
                pens={
                    {COLOR_GRAY, COLOR_GRAY,  COLOR_GRAY, COLOR_RED},
                    {COLOR_GRAY, COLOR_BROWN, COLOR_GRAY, COLOR_GRAY},
                    {COLOR_GRAY, COLOR_GRAY,  COLOR_GRAY, COLOR_GRAY},
                },
                asset={page='INTERFACE_BITS', x=0, y=22},
                tiles_override={{[4]=string.byte(self.hint_char)}},
            },
            on_click=self:callback('mining_menu'),
        },

WrappedLabel class
------------------

This Label subclass represents text that you want to be able to dynamically
wrap. This frees you from having to pre-split long strings into multiple lines
in the Label ``text`` list.

It has the following attributes:

:text_to_wrap: The string (or a table of strings or a function that returns a
    string or a table of strings) to display. The text will be autowrapped to
    the width of the widget, though any existing newlines will be kept.
:indent: The number of spaces to indent the text from the left margin. The
    default is ``0``.

The displayed text is refreshed and rewrapped whenever the widget bounds change.
To force a refresh (to pick up changes in the string that ``text_to_wrap``
returns, for example), all ``updateLayout()`` on this widget or on a widget that
contains this widget.

TooltipLabel class
------------------

This WrappedLabel subclass represents text that you want to be able to
dynamically hide, like help text in a tooltip.

It has the following attributes:

:show_tooltip: Boolean or a callback; if true, the widget is visible.

The ``text_pen`` attribute of the ``Label`` class is overridden with a default
of ``COLOR_GREY`` and the ``indent`` attribute of the ``WrappedLabel`` class is
overridden with a default of ``2``.

The text of the tooltip can be passed in the inherited ``text_to_wrap``
attribute so it can be autowrapped, or in the basic ``text`` attribute if no
wrapping is required.

HotkeyLabel class
-----------------

This Label subclass is a convenience class for formatting text that responds to
a hotkey or mouse click.

It has the following attributes:

:key: The hotkey keycode to display, e.g., ``'CUSTOM_A'``.
:key_sep: If specified, will be used to customize how the activation key is
          displayed. See ``token.key_sep`` in the ``Label`` documentation.
:label: The string (or a function that returns a string) to display after the
    hotkey.
:on_activate: If specified, it is the callback that will be called whenever
    the hotkey is pressed or the label is clicked.

The HotkeyLabel widget implements the following methods:

* ``hotkeylabel:setLabel(label)``

    Updates the label without altering the hotkey text.

* ``hotkeylabel:setOnActivate(on_activate)``

    Updates the on_activate callback.

CycleHotkeyLabel class
----------------------

This Label subclass represents a group of related options that the user can
cycle through by pressing a specified hotkey or clicking on the text.

It has the following attributes:

:key: The hotkey keycode to display, e.g., ``'CUSTOM_A'``.
:key_back: Similar to ``key``, but will cycle backwards (optional)
:key_sep: If specified, will be used to customize how the activation key is
          displayed. See ``token.key_sep`` in the ``Label`` documentation.
:label: The string (or a function that returns a string) to display after the
    hotkey.
:label_width: The number of spaces to allocate to the ``label`` (for use in
    aligning a column of ``CycleHotkeyLabel`` labels).
:label_below: If ``true``, then the option value will appear below the label
    instead of to the right of it. Defaults to ``false``.
:option_gap: The size of the gap between the label text and the option value.
    Default is ``1``. If set to ``0``, there'll be no gap between the strings.
    If ``label_below`` == ``true``, negative values will shift the value leftwards.
:options: A list of strings or tables of
    ``{label=string or fn, value=val[, pen=pen]}``. String options use the same
    string for the label and value and use the default pen. The optional ``pen``
    element could be a color like ``COLOR_RED``.
:initial_option: The value or numeric index of the initial option.
:on_change: The callback to call when the selected option changes. It is called
    as ``on_change(new_option_value, old_option_value)``.

The index of the currently selected option in the ``options`` list is kept in
the ``option_idx`` instance variable.

The CycleHotkeyLabel widget implements the following methods:

* ``cyclehotkeylabel:cycle([backwards])``

    Cycles the selected option and triggers the ``on_change`` callback.
    If ``backwards`` is defined and is truthy, the cycle direction will be reversed

* ``cyclehotkeylabel:setOption(value_or_index, call_on_change)``

    Sets the current option to the option with the specified value or
    index. If ``call_on_change`` is set to ``true``, then the ``on_change``
    callback is triggered.

* ``cyclehotkeylabel:getOptionLabel([option_idx])``

    Retrieves the option label at the given index, or the label of the
    currently selected option if no index is given.

* ``cyclehotkeylabel:getOptionValue([option_idx])``

    Retrieves the option value at the given index, or the value of the
    currently selected option if no index is given.

* ``cyclehotkeylabel:getOptionPen([option_idx])``

    Retrieves the option pen at the given index, or the pen of the currently
    selected option if no index is given. If an option was defined as just a
    string, then this function will return ``nil`` for that option.

ButtonGroup class
-----------------

This is a specialized subclass of CycleHotkeyLabel that, in addition to the
regular clickable widget, displays a corresponding row of clickable graphical
buttons and synchronizes their selection state with the currently selected
option.

It takes two additional required parameters to define the buttons:

:button_specs: A list of specs to pass to ``makeButtonLabelText`` (defined in
    `Label class`_ above).
:button_specs_selected: A list of specs that represent the buttons in their
    selected state.

ToggleHotkeyLabel class
-----------------------

This is a specialized subclass of CycleHotkeyLabel that has two options:
``On`` (with a value of ``true``) and ``Off`` (with a value of ``false``). The
``On`` option is rendered in green.

HelpButton class
----------------

A 3x1 tile button with a question mark on it, intended to represent a help
icon. Clicking on the icon will launch `gui/launcher` with a given command
string, showing the help text for that command.

It has the following attributes:

:command: The command to load in `gui/launcher`.

It also sets the ``frame`` attribute so the button appears in the upper right
corner of the parent, but you can override this to your liking if you want a
different position.

ConfigureButton class
---------------------

A 3x1 tile button with a gear mark on it, intended to represent a configure
icon. Clicking on the icon will run the given callback.

It has the following attributes:

:on_click: The function on run when the icon is clicked.

BannerPanel class
-----------------

This is a Panel subclass that prints a distinctive banner along the far left
and right columns of the widget frame. Note that this is not a "proper" frame
since it doesn't have top or bottom borders. Subviews of this panel should
inset their frames one tile from the left and right edges.

TextButton class
----------------

This is a BannerPanel subclass that wraps a HotkeyLabel with some decorators on
the sides to make it look more like a button, suitable for both graphics and
ASCII modes. All HotkeyLabel parameters passed to the constructor are passed
through to the wrapped HotkeyLabel.

List class
----------

The List widget implements a simple list with paging. You can click on a list
item to call the ``on_submit`` callback for that item.

It has the following attributes:

:text_pen: Specifies the pen for deselected list entries.
:text_hpen: Specifies the pen for entries that the mouse is hovered over.
            Defaults to swapping the background/foreground colors.
:cursor_pen: Specifies the pen for the selected entry.
:inactive_pen: If specified, used for the cursor when the widget is not active.
:icon_pen: Default pen for icons.
:on_select: Selection change callback; called as ``on_select(index,choice)``.
            This is also called with *nil* arguments if ``setChoices`` is called
            with an empty list.
:on_submit: Enter key or mouse click callback; if specified, the list reacts to the
            key/click and calls the callback as ``on_submit(index,choice)``.
:on_submit2: Shift-click callback; if specified, the list reacts to the click and
             calls the callback as ``on_submit2(index,choice)``.
:on_double_click: Mouse double click callback; if specified, the list reacts to the
            click and calls the callback as ``on_double_click(index,choice)``.
:on_double_click2: Shift-double click callback; if specified, the list reacts to the
             click and calls the callback as ``on_double_click2(index,choice)``.
:row_height: Height of every row in text lines.
:icon_width: If not *nil*, the specified number of character columns
             are reserved to the left of the list item for the icons.
:scroll_keys: Specifies which keys the list should react to as a table.

Every list item may be specified either as a string, or as a lua table
with the following fields:

:text: Specifies the label text in the same format as the Label text.
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

* ``list:getIdxUnderMouse()``

  Returns the index of the list item under the mouse cursor, or nothing if the
  list is empty or the mouse is not over a list item.

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
construction that allows filtering the list.

In addition to passing through all attributes supported by List, it
supports:

:edit_pen: If specified, used instead of ``cursor_pen`` for the edit field.
:edit_below: If true, the edit field is placed below the list instead of above.
:edit_key: If specified, the edit field is disabled until this key is pressed.
:edit_ignore_keys: If specified, will be passed to the filter edit field as its ``ignore_keys`` attribute.
:edit_on_change: If specified, will be passed to the filter edit field as its ``on_change`` attribute.
:edit_on_char: If specified, will be passed to the filter edit field as its ``on_char`` attribute.
:not_found_label: Specifies the text of the label shown when no items match the filter.

The list choices may include the following attributes:

:search_key: If specified, used instead of **text** to match against the filter.
    This is required for any entries where **text** is not a string.

The widget implements:

* ``list:setChoices(choices[, selected])``

  Resets the filter, and passes through to the inner list.

* ``list:getChoices()``

  Returns the list of *all* choices.

* ``list:getVisibleChoices()``

  Returns the *filtered* list of choices.

* ``list:getFilter()``

  Returns the current filter string, and the *filtered* list of choices.

* ``list:setFilter(filter[,pos])``

  Sets the new filter string, filters the list, and selects the item at
  index ``pos`` in the *unfiltered* list if possible.

* ``list:canSubmit()``

  Checks if there are currently any choices in the filtered list.

* ``list:getSelected()``, ``list:getContentWidth()``, ``list:getContentHeight()``, ``list:submit()``

  Same as with an ordinary list.

Filter behavior:

By default, the filter matches substrings that start at the beginning of a word
(or after any punctuation). You can instead configure filters to match any
substring across the full text with a command like::

  :lua require('utils').FILTER_FULL_TEXT=true

TabBar class
------------

This widget implements a set of one or more tabs to allow navigation between groups
of content. Tabs automatically wrap on the width of the window and will continue
rendering on the next line(s) if all tabs cannot fit on a single line.

:key: Specifies a keybinding that can be used to switch to the next tab.
      Defaults to ``CUSTOM_CTRL_T``.
:key_back: Specifies a keybinding that can be used to switch to the previous
      tab. Defaults to ``CUSTOM_CTRL_Y``.
:labels: A table of strings; entry representing the label text for a single tab.
         The order of the entries determines the order the tabs will appear in.
:on_select: Callback executed when a tab is selected. It receives the selected tab
            index as an argument. The provided function should update the value of
            whichever variable your script uses to keep track of the currently
            selected tab.
:get_cur_page: The function used by the TabBar to determine which Tab is currently
               selected. The function you provide should return an integer that
               corresponds to the non-zero index of the currently selected Tab
               (i.e., whatever variable you update in your ``on_select`` callback).
:active_tab_pens: A table of pens used to render active tabs. See the default
                  implementation in widgets.lua for an example of how to construct
                  the table. Leave unspecified to use the default pens.
:inactive_tab_pens: A table of pens used to render inactive tabs. See the default
                    implementation in widgets.lua for an example of how to
                    construct the table. Leave unspecified to use the default pens.
:get_pens: A function used to determine which pens should be used to render a tab.
           Receives the index of the tab as the first argument and the TabBar widget
           itself as the second. The default implementation, which will handle most
           situations, returns ``self.active_tab_pens``, if ``self.get_cur_page() == idx``,
           otherwise returns ``self.inactive_tab_pens``.

Tab class
---------

This widget implements a single clickable tab and is the main component of the TabBar
widget. Usage of the ``TabBar`` widget does not require direct usage of ``Tab``.

:id: The id of the tab.
:label: The text displayed on the tab.
:on_select: Callback executed when the tab is selected.
:get_pens: A function that is used during ``Tab:onRenderBody`` to determine the pens
           that should be used for drawing. See the usage of ``Tab`` in
           ``TabBar:init()`` for an example. See the default value of
           ``active_tab_pens`` or ``inactive_tab_pens`` in ``TabBar``
           for an example of how to construct pens.

RangeSlider class
-----------------

This widget implements a mouse-interactable range-slider. The player can move its two
handles to set minimum and maximum values to define a range, or they can drag the bar
itself to move both handles at once. The parent widget owns the range values, and can
control them independently (e.g., with ``CycleHotkeyLabels``). If the range values
change, the ``RangeSlider`` appearance will adjust automatically.

:num_stops: Used to specify the number of "notches" in the range slider, the places
            where handles can stop. (This should match the parents' number of options.)
:get_left_idx_fn: The function used by the RangeSlider to get the notch index on which
                  to display the left handle.
:get_right_idx_fn: The function used by the RangeSlider to get the notch index on which
                   to display the right handle.
:on_left_change: Callback executed when moving the left handle.
:on_right_change: Callback executed when moving the right handle.

DimensionsTooltip class
-----------------------

This widget follows the mouse cursor around and displays a string that
indicates selected 3d dimensions. It is intended to be a child widget of a
full-screen ``View``, such as a ``ZScreen``.

:display_offset: the offset from the mouse cursor where the tooltip is
    displayed. Positive offsets are down and to the right. Defaults to
    ``{x=3, y=3}``.
:get_anchor_pos_fn: function that provides the other corner of the selected
    area as a ``df.coord``-style table, that is, a table with ``x``, ``y``, and
    ``z`` fields. Must return ``nil`` if there is no current selection.

gui.textures
============

This module contains convenience methods for accessing default DFHack graphic assets.
Pass the ``offset`` in tiles (in row major position) to get a particular tile from the
asset. ``offset`` 0 is the first tile.

* ``tp_green_pin(offset)`` tileset: ``hack/data/art/green-pin.png``
* ``tp_red_pin(offset)`` tileset: ``hack/data/art/red-pin.png``
* ``tp_icons(offset)`` tileset: ``hack/data/art/icons.png``
* ``tp_on_off(offset)`` tileset: ``hack/data/art/on-off.png``
* ``tp_control_panel(offset)`` tileset: ``hack/data/art/control-panel.png``
* ``tp_border_thin(offset)`` tileset: ``hack/data/art/border-thin.png``
* ``tp_border_medium(offset)`` tileset: ``hack/data/art/border-medium.png``
* ``tp_border_bold(offset)`` tileset: ``hack/data/art/border-bold.png``
* ``tp_border_panel(offset)`` tileset: ``hack/data/art/border-panel.png``
* ``tp_border_window(offset)`` tileset: ``hack/data/art/order-window.png``

Example usage::

  local textures = require('gui.textures')
  local first_border_texpos = textures.tp_border_thin(1)


.. _lua-plugins:

=======
Plugins
=======

.. contents::
   :local:

DFHack plugins may export native functions and events to Lua contexts. These are
exposed as ``plugins.<name>`` modules, which can be imported with
``require('plugins.<name>')``. The plugins listed in this section expose
functions and/or data to Lua in this way.

In addition to any native functions documented here, plugins that can be
enabled (that is, plugins that support the `enable/disable API <enable>`) will
have the following functions defined:

* ``isEnabled()`` returns whether the plugin is enabled.
* ``setEnabled(boolean)`` sets whether the plugin is enabled.

For plugin developers, note that a Lua file in ``plugins/lua`` is required for
``require()`` to work, even if it contains no pure-Lua functions. This file must
contain ``mkmodule('plugins.<name>')`` to import any native functions defined in
the plugin. See existing files in ``plugins/lua`` for examples.

blueprint
=========

Lua functions provided by the `blueprint` plugin to programmatically generate
blueprint files:

* ``dig(start, end, name)``
* ``build(start, end, name)``
* ``place(start, end, name)``
* ``query(start, end, name)``

  ``start`` and ``end`` are tables containing positions (see ``xyz2pos``).
  ``name`` is used as the basis for the generated filenames.

The names of the functions are also available as the keys of the
``valid_phases`` table.

.. _building-hacks-api:

building-hacks
==============

This plugin overwrites some methods in workshop df class so that mechanical workshops are
possible. Although plugin export a function it's recommended to use lua decorated function.

.. contents::
  :local:

Functions
---------

``registerBuilding(table)`` where table must contain name, as a workshop raw name,
the rest are optional:

    :name:
        custom workshop id e.g., ``SOAPMAKER``

        .. note:: this is the only mandatory field.

    :fix_impassible:
        if true make impassable tiles impassable to liquids too
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
        a flag if this building can be counted in room. 1 means it can,
        0 means it can't and -1 default building behaviour
    :auto_gears:
        a flag that automatically fills up gears and animations.
        It looks over the building definition for gear icons and maps them.

    Animate table also might contain:

    :frameLength:
        how many ticks does one frame take OR
    :isMechanical:
        a bool that says to try to match to mechanical system (i.e., how gears are turning)

``getPower(building)`` returns two number - produced and consumed power if building can be
modified and returns nothing otherwise

``setPower(building,produced,consumed)`` sets current power production and
consumption for a building.

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

buildingplan
============

Native functions provided by the `buildingplan` plugin:

* ``bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)``

  Returns whether the building type is handled by buildingplan.

* ``bool isPlanModeEnabled(df::building_type type, int16_t subtype, int32_t custom)``

  Returns whether the buildingplan UI is enabled for the specified building type.

* ``bool isPlannedBuilding(df::building *bld)``

  Returns whether the given building is managed by buildingplan.

* ``void addPlannedBuilding(df::building *bld)``

  Suspends the building jobs and adds the building to the monitor list.

* ``void doCycle()``

  Runs a check for whether buildings in the monitor list can be assigned
  items and unsuspended. This method runs automatically twice a game day,
  so you only need to call it directly if you want buildingplan to do a
  check right now.

* ``void scheduleCycle()``

  Schedules a cycle to be run during the next non-paused game frame.
  Can be called multiple times while the game is paused and only one
  cycle will be scheduled.

.. _cxxrandom-api:

cxxrandom
=========

Exposes some features of the C++11 random number library to Lua.

.. contents::
  :local:

Native functions (exported to Lua)
----------------------------------

- ``GenerateEngine(seed)``

  returns engine id

- ``DestroyEngine(rngID)``

  destroys corresponding engine

- ``NewSeed(rngID, seed)``

  re-seeds engine

- ``rollInt(rngID, min, max)``

  generates random integer

- ``rollDouble(rngID, min, max)``

  generates random double

- ``rollNormal(rngID, avg, stddev)``

  generates random double drawn from a normal (Gaussian) distribution

- ``rollBool(rngID, chance)``

  generates random boolean

- ``MakeNumSequence(start, end)``

  returns sequence id

- ``AddToSequence(seqID, num)``

  adds a number to the sequence

- ``ShuffleSequence(seqID, rngID)``

  shuffles the number sequence

- ``NextInSequence(seqID)``

  returns the next number in sequence


Lua plugin functions
--------------------

- ``MakeNewEngine(seed)``

  returns engine id

Lua plugin classes
------------------

crng
~~~~

- ``init(id, df, dist)``: constructor

  - ``id``: Reference ID of engine to use in RNGenerations.
  - ``df`` (optional): bool indicating whether to destroy the Engine
                       when the crng object is garbage collected.
  - ``dist`` (optional): lua number distribution to use.

- ``changeSeed(seed)``: alters engine's seed value
- ``setNumDistrib(distrib)``: sets the number distribution crng object should use.

  - ``distrib``: number distribution object to use in RNGenerations.

- ``next()``: returns the next number in the distribution.
- ``shuffle()``: effectively shuffles the number distribution.

normal_distribution
~~~~~~~~~~~~~~~~~~~

- ``init(avg, stddev)``: constructor
- ``next(id)``: returns next number in the distribution

  - ``id``: engine ID to pass to native function

real_distribution
~~~~~~~~~~~~~~~~~

- ``init(min, max)``: constructor
- ``next(id)``: returns next number in the distribution

  - ``id``: engine ID to pass to native function

int_distribution
~~~~~~~~~~~~~~~~

- ``init(min, max)``: constructor
- ``next(id)``: returns next number in the distribution

  - ``id``: engine ID to pass to native function

bool_distribution
~~~~~~~~~~~~~~~~~

- ``init(chance)``: constructor
- ``next(id)``: returns next boolean in the distribution

  - ``id``: engine ID to pass to native function

num_sequence
~~~~~~~~~~~~

- ``init(a, b)``: constructor
- ``add(num)``: adds num to the end of the number sequence
- ``shuffle()``: shuffles the sequence of numbers
- ``next()``: returns next number in the sequence

Usage
-----

The randomization state is kept in an "engine". The distribution class turns
that engine state into random numbers.

Example::

    local rng = require('plugins.cxxrandom')
    local norm_dist = rng.normal_distribution(6820, 116) -- avg, stddev
    local engID = rng.MakeNewEngine(0)
    print(norm_dist:next(engID))

    -- alternate syntax
    local cleanup = true   -- delete engine on cleanup
    local number_generator = rng.crng:new(engID, cleanup, norm_dist)
    print(number_generator:next())

    -- simplified
    print(rng.rollNormal(engID, 6820, 116))

The number sequences are much simpler. They're intended for where you need to
randomly generate an index, perhaps in a loop for an array. You technically
don't need an engine to use it, if you don't mind never shuffling.

Example::

    local rng = require('plugins.cxxrandom')
    local engID = rng.MakeNewEngine(0)
    local g = rng.crng:new(engId, true, rng.num_sequence:new(0, table_size))
    g:shuffle()
    for _ = 1, table_size do
        func(array[g:next()])
    end


dig-now
=======

The dig-now plugin exposes the following functions to Lua:

* ``dig_now_tile(pos)`` or ``dig_now_tile(x,y,z)``: Runs dig-now for the
    specified tile coordinate. Default options apply, as if you were running the
    command ``dig-now <pos> <pos>``. See the `dig-now` documentation for details
    on default settings.

.. _eventful-api:

eventful
========

This plugin exports some events to lua thus allowing to run lua functions
on DF world events.

.. contents::
  :local:

List of events
--------------

1. ``onReactionCompleting(reaction,reaction_product,unit,input_items,input_reagents,output_items,call_native)``

  Is called once per reaction product, before the reaction has a chance to
  call native code for item creation. Setting ``call_native.value=false``
  cancels further processing: no items are created and ``onReactionComplete``
  is not called.

2. ``onReactionComplete(reaction,reaction_product,unit,input_items,input_reagents,output_items)``

  Is called once per reaction product, when reaction finishes and has
  at least one product.

3. ``onItemContaminateWound(item,unit,wound,number1,number2)``

  Is called when item tries to contaminate wound (e.g., stuck in).

4. ``onProjItemCheckMovement(projectile)``

  Is called when projectile moves.

5. ``onProjItemCheckImpact(projectile,somebool)``

  Is called when projectile hits something.

6. ``onProjUnitCheckMovement(projectile)``

  Is called when projectile moves.

7. ``onProjUnitCheckImpact(projectile,somebool)``

  Is called when projectile hits something.

8. ``onWorkshopFillSidebarMenu(workshop,callnative)``

  Is called when viewing a workshop in 'q' mode, to populate
  reactions, useful for custom viewscreens for shops.

9. ``postWorkshopFillSidebarMenu(workshop)``

  Is called after calling (or not) native fillSidebarMenu().
  Useful for job button tweaking (e.g., adding custom reactions)

.. _EventManager:

Events from EventManager
------------------------
These events are straight from EventManager module. Each of them first needs
to be enabled. See functions for more info. If you register a listener before
the game is loaded, be aware that no events will be triggered immediately
after loading, so you might need to add another event listener for when the
game first loads in some cases.

1. ``onBuildingCreatedDestroyed(building_id)``

  Gets called when building is created or destroyed.

2. ``onConstructionCreatedDestroyed(building_id)``

  Gets called when construction is created or destroyed.

3. ``onJobInitiated(job)``

  Gets called when job is issued.

4. ``onJobCompleted(job)``

  Gets called when job is finished. The job that is passed to this function
  is a copy. Requires a frequency of 0 in order to distinguish between
  workshop jobs that were canceled by the user and workshop jobs that
  completed successfully.

5. ``onUnitDeath(unit_id)``

  Gets called on unit death.

6. ``onItemCreated(item_id)``

  Gets called when item is created (except due to traders, migrants,
  invaders, and spider webs).

7. ``onSyndrome(unit_id,syndrome_index)``

  Gets called when new syndrome appears on a unit.

8. ``onInvasion(invasion_id)``

  Gets called when new invasion happens.

9. ``onInventoryChange(unit_id,item_id,old_equip,new_equip)``

  Gets called when someone picks up an item, puts one down,
  or changes the way they are holding it. If an item is picked up,
  old_equip will be null. If an item is dropped, new_equip will be null.
  If an item is re-equipped in a new way, then neither will be null.
  You absolutely must NOT alter either old_equip or new_equip or you
  might break other plugins.

10. ``onReport(reportId)``

  Gets called when a report happens. This happens more often than
  you probably think, even if it doesn't show up in the announcements.

11. ``onUnitAttack(attackerId, defenderId, woundId)``

  Called when a unit wounds another with a weapon.
  Is NOT called if blocked, dodged, deflected, or parried.

12. ``onUnload()``

  A convenience event in case you don't want to register for every onStateChange event.

13. ``onInteraction(attackVerb, defendVerb, attackerId, defenderId, attackReportId, defendReportId)``

  Called when a unit uses an interaction on another.

Functions
---------

1. ``registerReaction(reaction_name,callback)``

  Simplified way of using onReactionCompleting; the callback is function (same params as event).

2. ``removeNative(shop_name)``

  Removes native choice list from the building.

3. ``addReactionToShop(reaction_name,shop_name)``

  Add a custom reaction to the building.

4. ``enableEvent(evType,frequency)``

  Enable event checking for EventManager events. For event types use ``eventType`` table.
  Note that different types of events require different frequencies to be effective. The
  frequency is how many ticks EventManager will wait before checking if that type of event
  has happened. If multiple scripts or plugins use the same event type, the smallest frequency
  is the one that is used, so you might get events triggered more often than the frequency
  you use here.

5. ``registerSidebar(shop_name,callback)``

  Enable callback when sidebar for ``shop_name`` is drawn. Useful for custom workshop views,
  e.g., using gui.dwarfmode lib. Also accepts a ``class`` instead of function as callback.
  Best used with ``gui.dwarfmode`` class ``WorkshopOverlay``.

Examples
--------
Spawn dragon breath on each item attempt to contaminate wound::

    b=require "plugins.eventful"
    b.onItemContaminateWound.one=function(item,unit,un_wound,x,y)
        local flw=dfhack.maps.spawnFlow(unit.pos,6,0,0,50000)
    end

Reaction complete example::

  b=require "plugins.eventful"

  b.registerReaction("LAY_BOMB",function(reaction,unit,in_items,in_reag,out_items,call_native)
    local pos=copyall(unit.pos)
    -- spawn dragonbreath after 100 ticks
    dfhack.timeout(100,"ticks",function() dfhack.maps.spawnFlow(pos,6,0,0,50000) end)
    --do not call real item creation code
    call_native.value=false
  end)

Grenade example::

  b=require "plugins.eventful"
  b.onProjItemCheckImpact.one=function(projectile)
    -- you can check if projectile.item e.g., has correct material
    dfhack.maps.spawnFlow(projectile.cur_pos,6,0,0,50000)
  end

Integrated tannery::

  b=require "plugins.eventful"
  b.addReactionToShop("TAN_A_HIDE","LEATHERWORKS")

.. _luasocket-api:

luasocket
=========

A way to access csocket from lua. The usage is made similar to luasocket in
vanilla lua distributions. Currently only a subset of the functions exist
and only TCP mode is implemented.

.. contents::
  :local:

Socket class
------------

This is a base class for ``client`` and ``server`` sockets. You can not create
it - it's like a virtual base class in c++.

* ``socket:close()``

  Closes the connection.

* ``socket:setTimeout(sec,msec)``

  Sets the operation timeout for this socket. It's possible to set timeout to 0.
  Then it performs like a non-blocking socket.

Client class
------------

Client is a connection socket to a server. You can get this object either from
``tcp:connect(address,port)`` or from ``server:accept()``.
It's a subclass of ``socket``.

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

  Starts listening on that port for incoming connections.
  Returns ``server`` object.

* ``tcp:connect(address,port)``

  Tries connecting to that address and port. Returns ``client`` object.


.. _map-render-api:

map-render
==========

A way to ask DF to render a section of the fortress mode map.
This uses a native DF rendering function so it's highly dependent
on DF settings (e.g., tileset, colors, etc.)

Functions
---------

- ``render_map_rect(x,y,z,w,h)``

  Returns a table with w*h*4 entries of rendered tiles. The format is
  the same as ``df.global.gps.screen`` (tile,foreground,bright,background).

.. _pathable-api:

pathable
========

This plugin implements the back end of the `gui/pathable` script. It exports a
single Lua function, in ``hack/lua/plugins/pathable.lua``:

* ``paintScreen(cursor[,skip_unrevealed])``: Paint each visible of the screen
  green or red, depending on whether it can be pathed to from the tile at
  ``cursor``. If ``skip_unrevealed`` is specified and true, do not draw
  unrevealed tiles.

reveal
======

Native functions provided by the `reveal` plugin:

* ``void unhideFlood(pos)``: Unhides map tiles according to visibility rules,
  starting from the given coordinates. This algorithm only processes adjacent
  hidden tiles, so it must start on a hidden tile in order to have any effect.
  It will not reveal hidden sections separated by already-unhidden tiles.

Example of revealing a cavern that happens to have an open tile at the specified
coordinate::

    unhideFlood({x=25, y=38, z=140})

sort
====

The `sort <sort>` plugin does not export any native functions as of now.
Instead, it calls Lua code to perform the actual ordering of list items.

tiletypes
=========

* ``bool tiletypes_setTile(pos, shape, material, special, variant)`` where
  the parameters are enum values from ``df.tiletype_shape``,
  ``df.tiletype_material``, etc. Returns whether the conversion succeeded.

* ``bool tiletypes_setTile(pos, tiletype_options)`` where
  the ``tiletype_options`` parameter takes in a table, with any fields matching
  the available tiletypes options. Any unspecified fields default to keeping
  the value of the original tile. Returns whether the conversion succeeded.
  - ``shape``: ``df.tiletype_shape``
  - ``material``: ``df.tiletype_material``
  - ``special``: ``df.tiletype_special``
  - ``variant``: ``df.tiletype_variant``
  - ``hidden``: -1, 0, or 1
  - ``light``: -1, 0, or 1
  - ``subterranean``: -1, 0, or 1
  - ``skyview``: -1, 0, or 1
  - ``aquifer``: -1, 0, 1, or 2
  - ``autocorrect``: 0 or 1
  - ``stone_material``: integer material id
  - ``vein_type``: ``df.inclusion_type``

.. _xlsxreader-api:

xlsxreader
==========

Utility functions to facilitate reading .xlsx spreadsheets. It provides the
following low-level API methods:

- ``open_xlsx_file(filename)`` returns a file_handle or nil on error
- ``close_xlsx_file(file_handle)`` closes the specified file_handle
- ``list_sheets(file_handle)`` returns a list of strings representing sheet
  names
- ``open_sheet(file_handle, sheet_name)`` returns a sheet_handle. This call
  always succeeds, even if the sheet doesn't exist. Non-existent sheets will
  have no data, though.
- ``close_sheet(sheet_handle)`` closes the specified sheet_handle
- ``get_row(sheet_handle, max_tokens)`` returns a list of strings representing
  the contents of the cells in the next row. The ``max_tokens`` parameter is
  optional. If set to a number > 0, it limits the number of cells read and
  returned for the row.

The plugin also provides Lua class wrappers for ease of use:

- ``XlsxioReader`` provides access to .xlsx files
- ``XlsxioSheetReader`` provides access to sheets within .xlsx files
- ``open(filepath)`` initializes and returns an ``XlsxioReader`` object

The ``XlsxioReader`` class has the following methods:

- ``XlsxioReader:close()`` closes the file. Be sure to close any open child
  sheet handles first!
- ``XlsxioReader:list_sheets()`` returns a list of strings representing sheet
  names
- ``XlsxioReader:open_sheet(sheet_name)`` returns an initialized
  ``XlsxioSheetReader`` object

The ``XlsxioSheetReader`` class has the following methods:

- ``XlsxioSheetReader:close()`` closes the sheet
- ``XlsxioSheetReader:get_row(max_tokens)`` reads the next row from the sheet.
  If ``max_tokens`` is specified and is a positive integer, only the first
  ``max_tokens`` elements of the row are returned.

Here is an end-to-end example::

    local xlsxreader = require('plugins.xlsxreader')

    local function dump_sheet(reader, sheet_name)
        print('reading sheet: ' .. sheet_name)
        local sheet_reader = reader:open_sheet(sheet_name)
        dfhack.with_finalize(
            function() sheet_reader:close() end,
            function()
                local row_cells = sheet_reader:get_row()
                while row_cells do
                    printall(row_cells)
                    row_cells = sheet_reader:get_row()
                end
            end
        )
    end

    local filepath = 'path/to/some_file.xlsx'
    local reader = xlsxreader.open(filepath)
    dfhack.with_finalize(
        function() reader:close() end,
        function()
            for _,sheet_name in ipairs(reader:list_sheets()) do
                dump_sheet(reader, sheet_name)
            end
        end
    )

=======
Scripts
=======

.. contents::
   :local:

Any files with the ``.lua`` extension placed into the :file:`hack/scripts` folder
(or any other folder in your `script-paths`) are automatically made available as
DFHack commands. The command corresponding to a script is simply the script's
filename, relative to the scripts folder, with the extension omitted. For example:

* :file:`dfhack-config/scripts/startup.lua` is invoked as ``startup``
* :file:`hack/scripts/gui/teleport.lua` is invoked as ``gui/teleport``

.. note::
    In general, scripts should be placed in subfolders in the following
    situations:

    * ``devel``: scripts that are intended exclusively for DFHack development,
      including examples, or scripts that are experimental and unstable
    * ``fix``: fixes for specific DF issues
    * ``gui``: GUI front-ends for existing tools (for example, see the
      relationship between `teleport` and `gui/teleport`)
    * ``modtools``: scripts that are intended to be run exclusively as part of
      mods, not directly by end-users (as a rule of thumb: if someone other than
      a mod developer would want to run a script from the console, it should
      not be placed in this folder)

Scripts are read from disk when run for the first time, or if they have changed
since the last time they were run.

Each script has an isolated environment where global variables set by the script
are stored. Values of globals persist across script runs in the same DF session.
See `devel/lua-example` for an example of this behavior. Note that ``local``
variables do *not* persist.

Arguments are passed in to the scripts via the ``...`` built-in quasi-variable;
when the script is called by the DFHack core, they are all guaranteed to be
non-nil strings.

Additional data about how a script is invoked is passed to the script as a
special ``dfhack_flags`` global, which is unique to each script. This table
is guaranteed to exist, but individual entries may be present or absent
depending on how the script was invoked. Flags that are present are described
in the subsections below.

DFHack invokes the scripts in the `core context <lua-core-context>`; however it
is possible to call them from any lua code (including from other scripts) in any
context with ``dfhack.run_script()`` below.

General script API
==================

* ``dfhack.run_script(name[,args...])``

  Run a Lua script in your `script-paths`, as if it were started from the
  DFHack command-line. The ``name`` argument should be the name of the script
  without its extension, as it would be used on the command line.

  Example:

  In DFHack prompt::

    repeat -time 14 -timeUnits days -command [ workorder ShearCreature ] -name autoShearCreature

  In Lua script::

    dfhack.run_script("repeat", "-time", "14", "-timeUnits", "days", "-command", "[", "workorder", "ShearCreature", "]", "-name", "autoShearCreature")

  Note that the ``dfhack.run_script()`` function allows Lua errors to propagate to the caller.

  To run other types of commands (i.e., built-in commands or commands provided by plugins),
  see ``dfhack.run_command()``. Note that this is slightly slower than ``dfhack.run_script()``
  when running Lua scripts.

* ``dfhack.script_help([name, [extension]])``

  Returns the contents of the rendered (or embedded) `documentation` for the
  specified script. ``extension`` defaults to "lua", and ``name`` defaults to
  the name of the script where this function was called. For example, the
  following can be used to print the current script's help text::

    local args = {...}
    if args[1] == 'help' then
        print(script_help())
        return
    end

.. _reqscript:

Importing scripts
=================

* ``dfhack.reqscript(name)`` or ``reqscript(name)``

  Loads a Lua script and returns its environment (i.e., a table of all global
  functions and variables). This is similar to the built-in ``require()``, but
  searches all `script-paths` for the first matching ``name.lua`` file instead
  of searching the Lua library paths (like ``hack/lua/``).

  Most scripts can be made to support ``reqscript()`` without significant
  changes (in contrast, ``require()`` requires the use of ``mkmodule()`` and
  some additional boilerplate). However, because scripts can have side effects
  when they are loaded (such as printing messages or modifying the game state),
  scripts that intend to support being imported must satisfy some criteria to
  ensure that they can be imported safely:

  1. Include the following line - ``reqscript()`` will fail if this line is
     not present::

      --@ module = true

  2. Include a check for ``dfhack_flags.module``, and avoid running any code
     that has side-effects if this flag is true. For instance::

      -- (function definitions)
      if dfhack_flags.module then
          return
      end
      -- (main script code with side-effects)

     or::

      -- (function definitions)
      function main()
          -- (main script code with side-effects)
      end
      if not dfhack_flags.module then
          main()
      end

  Example usage::

    local addThought = reqscript('add-thought')
    addThought.addEmotionToUnit(unit, ...)

  Circular dependencies between scripts are supported, as long as the scripts
  have no side-effects at load time (which should already be the case per
  the above criteria).

  .. warning::

    Avoid caching the table returned by ``reqscript()`` beyond storing it in
    a local variable as in the example above. ``reqscript()`` is fast for
    scripts that have previously been loaded and haven't changed. If you retain
    a reference to a table returned by an old ``reqscript()`` call, this may
    lead to unintended behavior if the location of the script changes (e.g., if a
    save is loaded or unloaded, or if a `script path <script-paths>` is added in
    some other way).

  .. admonition:: Tip

    Mods that include custom Lua modules can write these modules to support
    ``reqscript()`` and distribute them as scripts in ``raw/scripts``. Since the
    entire ``raw`` folder is copied into new saves, this will allow saves to be
    successfully transferred to other users who do not have the mod installed
    (as long as they have DFHack installed).

  .. admonition:: Backwards compatibility notes

    For backwards compatibility, ``moduleMode`` is also defined if
    ``dfhack_flags.module`` is defined, and is set to the same value.
    Support for this may be removed in a future version.

* ``dfhack.script_environment(name)``

  Similar to ``reqscript()`` but does not enforce the check for module support.
  This can be used to import scripts that support being used as a module but do
  not declare support as described above, although it is preferred to update
  such scripts so that ``reqscript()`` can be used instead.

.. _script-enable-api:

Enabling and disabling scripts
==============================

Scripts can choose to recognize the built-in ``enable`` and ``disable`` commands
by including the following line near the top of their file::

    --@enable = true
    --@module = true

Note that enableable scripts must also be `modules <reqscript>` so their
``isEnabled()`` functions can be called from outside the script.

When the ``enable`` and ``disable`` commands are invoked, the ``dfhack_flags``
table passed to the script will have the following fields set:

* ``enable``: Always ``true`` if the script is being enabled *or* disabled
* ``enable_state``: ``true`` if the script is being enabled, ``false`` otherwise

If you declare a global function named ``isEnabled`` that returns a boolean
indicating whether your script is enabled, then your script will be listed among
the other enableable scripts and plugins when the player runs the `enable`
command.

Example usage::

    --@enable = true
    --@module = true

    enabled = enabled or false
    function isEnabled()
        return enabled
    end

    -- (function definitions...)

    if dfhack_flags.enable then
        if dfhack_flags.enable_state then
            start()
            enabled = true
        else
            stop()
            enabled = false
        end
    end

If the state of your script can be tied to an active savegame, then your script
should hook the appropriate events to load persisted state when a savegame is
loaded. For example::

    local utils = require('utils')

    local GLOBAL_KEY = 'my-script-name'

    local function get_default_state()
        return {
            -- add default config here, e.g.,
            -- enabled=false,
        }
    end

    state = state or get_default_state()

    dfhack.onStateChange[GLOBAL_KEY] = function(sc)
        if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
            return
        end
        -- retrieve state saved in game. merge with default state so config
        -- saved from previous versions can pick up newer defaults.
        state = get_default_state()
        utils.assign(state, dfhack.persistent.getSiteData(GLOBAL_KEY, state))
    end

    -- to be called when global state changes that needs to be persisted
    local function persist_state()
        dfhack.persistent.saveSiteData(GLOBAL_KEY, state)
    end

The attachment to ``dfhack.onStateChange`` should appear in your script code
outside of any function. DFHack will load your script as a module just before
the ``SC_DFHACK_INITIALIZED`` state change event is sent, giving your code an
opportunity to run and attach hooks before the game is loaded.

If an enableable script is added to a DFHack `script path <script-paths>` while
DF is running, then it will miss the initial sweep that loads all the module
scripts and any ``onStateChange`` handlers the script may want to register will
not be registered until the script is loaded via some means, either by running
it or loading it as a module. If you just added new scripts that you want to
load so they can attach their ``onStateChange`` handlers, run ``enable`` without
parameters or call ``:lua require('script-manager').reload()`` to scan and load
all script modules.

Save init script
================

If a save directory contains a file called ``init.lua``, it is
automatically loaded and executed every time the save is loaded.
The same applies to any files called ``init.d/*.lua``. Every
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
