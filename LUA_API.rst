##############
DFHack Lua API
##############

.. contents::

====================
DF structure wrapper
====================

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

Struct references
-----------------

Struct references are used for class and struct objects.

They implement the following features:

* ``ref.field``, ``ref.field = value``

  Valid fields of the structure may be accessed by subscript.

  In case of inheritance, *superclass* fields have precedence
  over the subclass, but fields shadowed in this way can still
  be accessed as ``ref['subclasstype.field']``.

  This shadowing order is necessary because vtable-based classes
  are automatically exposed in their exact type, and the reverse
  rule would make access to superclass fields unreliable.

* ``ref._field(field)``

  Returns a reference to a valid field. That is, unlike regular
  subscript, it returns a pointer reference even for primitive
  typed fields.

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
  use ``#ref`` as index.

* ``ref:erase(index)``

  Removes the element at the given valid index.

Bitfield references
-------------------

Bitfields behave like special fixed-size containers.
The ``_enum`` property points to the bitfield type.

Numerical indices correspond to the shift value,
and if a subfield occupies multiple bits, the
``ipairs`` order would have a gap.

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
