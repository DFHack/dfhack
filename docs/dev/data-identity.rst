.. _data_identity:

###########################
DFHack Data Identity System
###########################

This article is an attempt to describe DFHack's data identity system.
DFHack internally has a collection of C++ classes that provide metedata about the data in DF itself as well as data used
by various components of DFHack. This metadata is used primarily to enable the Lua scripting system to access data
held by Dwarf Fortress in a transparent manner, but is also used for several other purposes within DFHack.

The base class of the identity system is the class ``type_identity``, defined in :source:`DataDefs.h <library/include/DataDefs.h>`. A ``type_identity`` object
provides information about one _type_ of data object, in either Dwarf Fortress or DFHack, that can be manipulated as a discrete entity in Lua.
With one specific exception (``global_identity``), there is a one-to-one relationship between C++ types and ``type_identity`` objects.
In Lua, objects that are being managed via the data identity system are represented as a Lua userdata object. The userdata object
contains both a pointer to the C++ object itself and a pointer to a ``type_identity`` object that describes the data pointed
by that pointer. Note that the userdata object does not own the objects pointed to by these pointers, and the Lua engine is
never responsible for managing their lifetimes.

``type_identity`` defines the following public methods:

- ``byte_size``: returns the size, in bytes, of the object held
- ``type``: returns an enum (of type ``identity_type``) classifying the object held.
- ``getFullName``: returns a string that describes the type. This will usually be similar to a C++ ``typedef``, although this is not guaranteed.
- ``lua_read``: Used by the Lua engine to "read" the data from a C++ object into the Lua state.
- ``lua_write``: Used by the Lua engine to "write" a value from the Lua state into a C++ data object.
- ``build_metatable``: Create a Lua metatable in the specified Lua state corresponding to this type identity.
- ``is_primitive``: indicates that ``lua_read`` will store a _copy_ of the object on the Lua stack instead of a non-owning reference to it. Used for types that have direct representations in Lua: numbers, booleans, simple strings
- ``is_constructed``: indicates that creating a C++ instance of this type requires the use of a possibly nontrivial constructor. A type identity that is both primitive and constructed cannot be inserted into a container. At the moment the only type identity that is both primitive and constructed is ``stl_string_identity``, which wraps the C++ ``std::string`` type.
- ``is_container``: indicates that the type is a container and thus implements the methods specific to ``container_identity``
- ``allocate``: allocate, and construct if necessary, a C++ instance of this type. This may fail if the type does not support construction.
- ``copy``: copy the object at ``src`` onto ``tgt``. This uses ``memmove`` for primitive types, and C++ copy-assignment (when possible) for other types

There are plethora of subclasses of ``type_identity``:

* ``type_identity``
  * ``constructed_identity`` anything with an internal structure (not primitive)
    * ``compound_identity`` anything with fields
      * ``bitfield_identity`` a structure defined with fields at bit rather than byte boundaries
      * ``enum_identity`` C++ ``enum``
      * ``struct_identity`` C++ ``class`` or ``structure``
        * ``global_identity`` holds, as a quasiobject, handles for all of the known Dwarf Fortress program-scope static objects as if they were fields of an object called ``global``
        * ``union_identity`` C++ ``union``
        * ``other_vectors_identity`` special-case identity for the categorized subvectors of objects that appears in many of Dwarf Fortress's "handler" classes
        * ``virtual_identity`` polymorphic C++ ``class`` or ``structure`` having a virtual table to handle virtual dispatch
      * ``stl_string_identity`` ``std::string``
      * ``xlsx_file_handle_identity`` special case
      * ``xlsx_sheet_handle_identity`` special case
    * ``container_identity`` "containers" generally. note that all container types are homogeneous (that is, the elements of the container must all be of the same type). abstract base class
      * ``bit_container_identity`` for containers that contain bools stored one element per _bit_ (rather than per byte)
        * ``bit_array_identity`` Dwarf Fortress's ``BitArray`` type
        * ``stl_bit_vector_identity`` ``std::vector<bool>``
      * ``buffer_container_identity`` C++ static arrays and raw C++ pointers acting as arrays of unspecified bound
      * ``enum_list_attr_identity`` (template) metaobject with metadata about a C++ enumeration; may also include additional metadata
      * ``ptr_container_identity`` containers that contain pointers
        *  ``stl_ptr_container_identity`` containers that are of the form ``std::vector<T*>`` for some ``T``
      * ``ro_stl_container_identity`` (template) "read only containers"
        * ``ro_stl_assoc_container_identity`` (template) ``std::map<KT,T>`` and ``std::unordered_map<KT,T>``
      * ``stl_container_identity`` (template) ``std::vector<T>`` where ``T`` is _not_ a pointer (and not `bool`)
    * ``opaque_identity`` opaque wrapper around any type, provides no functionality
    * ``stl_string_identity`` ``std::string``
  * ``function_identity_base`` abstract base class for ``function_identity``
    * ``function_identity`` (template) wrapper around a C++ function that can be invoked from Lua
  * ``primitive_identity`` wrapper around a primitive type. primitive types are fixed-length objects with no internal structure
    * ``bool_identity`` ``bool``
    * ``number_identity_base`` abstract base for numeric types
      * ``float_identity_base`` abstract base for floating point types
        * ``float_identity`` (template) ``double`` and ``float``
      * ``integer_identity_base`` abstract base for integral types
        * ``integer_identity`` (template) ``int8_t``, ``int16_t``, ``int32_t``, ``size_t``, etc. lots of these
    * ``pointer_identity`` any arbitrary C++ pointer (other than ``char*``)
    * ``ptr_string_identity`` C-style (``char *``) string

Types marked with "(template)" are C++ template types, all parameterized by a single typename.

Type identity object lifetime and mutability
============================================

_Most_ instances of ``type_identity`` are statically constructed and are effectively immutable,
although this is not at present enforced.
However, ``struct_identity``'s ``parent`` and ``child`` members can be mutated as additional identities are constructed
and so instances of ``struct_identity`` and its descendants are not immutable
while other ``struct_identity`` objects are still being constructed.
(Note that loading a plugin, which can happen at any time, can cause the construction of ``struct_identity`` objects and
thus the mutation of other existing ``struct_identity`` objects.)
In addition, ``virtual_identity`` objects contain the data for implementing vmethod interposes,
which can be added and removed dynamically during the life of the program, and so these objects remain mutable
for the life of the program.
It is therefore important that there be at most one ``virtual_identity`` object per virtual class,
although this is not enforced at present.
Having more than one ``struct_identity`` object for the same type might also potentially lead to misoperation.

In general, there should be a one to one correspondence between ``type_identity`` objects and C++ types
(with the special case that ``global_identity`` has no corresponding type).
As far as we know, for any type other than ``virtual_identity``,
violations of this constraint will not lead to misoperation, but this constraint should not be lightly violated.
The Lua/C++ interface does, in a handful of places, assume that it can compare ``type_identity``
pointers to determine if they reference the same type, but as far as we know all of these instances will fall
back to correct behavior as long as the shadow copies are indistinguishable from one another;
that is, two copies having the same values will compare equal in all known such comparisons.
Therefore, if two ``type_identity`` objects do exist (for any reason) for the same underlying C++ type,
those objects must be indistinguishable from one another by anything other than their address.

The ``type_identity`` object for a given C++ type can be obtained by using the ``get`` method of the ``identity_trait``
trait class.
More specifically, ``identity_class<T>::get()`` will return a pointer to a ``type_identity`` object for the type ``T``.
Developers who create new type identities must _either_ provide an specialization of ``identity_trait`` that implements
a ``get`` method that returns the correct ``type_identity`` _or_ ensure that a static instance of ``T::_identity`` exists
for the type T (which will result in a template in ``DataDefs.h`` providing one for that type).
Note that this is only possible for compound types, and is the way that the _vast_ majority of
compound type's have their identities specified (including all of those defined via codegen).

Because objects in the Lua environment are constructed as a pointer to the data and
a pointer to the data's ``type_identity`` object, it is necessary for ``type_identity`` objects to have a lifetime
that exceeds the lifetime in the Lua environment of any object that exists anywhere in the Lua environment.
It is therefore advised to avoid creating ``type_identity`` objects that do _not_ have program lifetime, since
predicting the lifetime of objects in the Lua environment can be difficult.
If it is necessary to create a ``type_identity`` that will not have program lifetime,
it is incumbent on the developer to ensure that no references of that type identity persist beyond its lifetime.

Due to the way template types are implemented in the C++ compilers we use for Dwarf Fortress, any specialization of one
of the type identity classes noted above as a template must at present be statically constructed in the DFHack core.
This is because we export the statically constructed instances from the core library to plugins, which then imports them
from the core library instead of instantiating them locally. As a result, referencing an instance in a plugin that has
not been instantiated in the core will result in linkage errors when linking the plugin to the core library.

We could instead _not_ export the templated types, and thus their statically constructed identity objects,
and instead allow the compiler to instantiate a local copy of these instances while compiling a plugin,
but this would definitely result in a violation of the current requirement that there be at most one instance of the
type identity object for a given C++ type across the entire program (including plugins loaded as a shared library).

For primitive and opaque types the static constructors of the identity types
are generally found in :source:`DataIdentity.h <library/include/DataIdentity.h>`
or :source:`DataIdentity.cpp <library/DataIdentity.cpp>`
Types defined by Dwarf Fortress are constructed in the header files and the related ``static*.inc`` files created by codegen,
which are included into DFHack via :source:`DataStatics.cpp <library/DataStatics.cpp>`.

Some plugins (e.g. :source:`blueprint <plugins/blueprint.cpp>`) also define their own type identities. Type identities in plugins should be used with caution,
because the DFHack plugin model allows plugins to be unloaded on request.
Since the ``type_identity`` object is constructed within the the plugin's address space, and Lua objects that reference
this ``type_identity`` object will hold a (borrowed) pointer to that object,
unloading the plugin will result in a dangling pointer reference within the Lua environment.
It is, at present, incumbent on plugin authors to ensure that they do not use plugin defined type identities on objects
that will persist in the Lua environment beyond the lifetime of the plugin.
Declaring a ``struct_identity`` in a plugin that is the child of another ``struct_identity`` will also result in
a potentially dangling reference to that identity in the ``child`` vector of the parent identity, which means this
must also be approached with caution.

A final note: because most instances of ``type_identity`` are statically constructed
and their construction is scattered across multiple translation units, it is _not_ safe to cross-reference
one ``type_identity`` instance during the instantiation of another, because the order in which statically constructed
objects are instantiated in C++ is unspecified for objects defined in different translation units.
Specifically, this means that the constructor for a ``type_identity`` instance must use care in using
``identity_traits<T>::get`` to get the identity object
of some other type because that type's identity object may not have been constructed yet.

Namespace issues
================

The type identity system formally lives in the ``DFHack`` namespace.
However, because the types created by the codegen process live in the ``df`` namespace,
the identities needed to describe types coming from Dwarf Fortress are imported into the ``df`` namespace
and at least some types are _only_ defined in that namespace.
This remains a point of confusion today and it can be difficult to determine whether a proposed new type identity
should be added to the ``DFHack`` namespace or created only in the ``df`` namespace.
This should probably be addressed at some point in the future.
