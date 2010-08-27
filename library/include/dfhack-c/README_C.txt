=======================================
Introduction And Reasons For Existence
=======================================

C++ is not an easy language to access from other languages.  There are several "features" that make interoperability considerably more painful when trying to write bindings for Python, Ruby, Lua, or whatever.  To that end, dfhack has a C translation layer to ease the process of making bindings for other languages.  A shadow API, if you will.

.. contents::


=================
Getting DFHack-C
=================
The C shim is a part of the standard dfhack package.  If you've installed dfhack, you've already got it.  The dfhack source and binaries are hosted on github_, at  http://github.com/peterix/dfhack

.. _github: http://www.github.com/

Packages
=========
The library and tools are packaged for Archlinux and are available both
in AUR and the arch-games repository.

The package name is dfhack-git.

========
Layout
========
The structure of the C shim mimics, as far as possible, the normal dfhack structure.  Of course, since C lacks things like classes, templates, and function overloading, there are a few places where deviations are unavoidable.

Return Values
=============
Unless otherwise specified, functions that return an int return one of the following values:

- 0:  The operation failed.
- 1:  The operation succeeded.
- -1:  An invalid module pointer was supplied.

Types
=======
Module objects are passed around as void pointers with the typedef name 'DFHackObject'.  Wherever possible, the structures and enumerations defined by dfhack are used without redefinition.

Allocator Callbacks
====================
Wherever possible, the C shim eschews the native allocation of memory, as this would require language bindings to remember to free the memory later, and would, in my opinion, make the shim less flexible.  So a number of function pointers are exposed to allow memory to be allocated in the language being used to wrap dfhack.  In general, the allocations relate to arrays of dfhack structures, but there are a couple of corner cases.

The buffer callback functions should take a pointer to an array of the particular type, and a 32-bit unsigned integer (uint32_t) defining the length of the array needed.  If the buffer was successfully created, the callback function should return 1.  In case of failure, set the buffer pointer to NULL (or 0) and return 0.

All of the allocators are defined in dfhack/library/include/dfhack-c/DFTypes_C.h.

Buffer Callback List
---------------------
- alloc_byte_buffer_callback(int8_t*, uint32_t)
- alloc_short_buffer_callback(int16_t*, uint32_t)
- alloc_int_buffer_callback(int32_t*, uint32_t)
- alloc_ubyte_buffer_callback(uint8_t*, uint32_t)
- alloc_ushort_buffer_callback(uint16_t*, uint32_t)
- alloc_uint_buffer_callback(uint32_t*, uint32_t)
- alloc_matgloss_buffer_callback(t_matgloss*, uint32_t)
- alloc_descriptor_buffer_callback(t_descriptor_color*, uint32_t)
- alloc_matgloss_other_buffer_callback(t_matglossOther*, uint32_t)
- alloc_vein_buffer_callback(t_vein*, uint32_t)
- alloc_frozenliquidvein_buffer_callback(t_frozenliquidvein*, uint32_t)
- alloc_spattervein_buffer_callback(t_spattervein*, uint32_t)

Templates Make My Life Harder
-------------------------------
Three dfhack structures (t_colormodifier, t_creaturecaste, and t_creaturetype) contain vectors, which (obviously) don't work in C.  Therefore, these structures have C versions that replace the vector with a buffer and length, but are otherwise identical to their C++ counterparts.  For each structure, there are three associated callbacks.  One initializes an empty instance of the structure, one initializes an instance with values passed in, and one allocates a buffer in the same manner as the other allocators.

A Small Callback Example In Python
-------------------------------------
The Python bindings for dfhack implement the unsigned integer allocator callback like this:

.. admonition:: util.py

    |    from ctypes import \*
    |
    |    def _allocate_array(t_type, count):
    |        arr_type = t_type * count
    |        arr = arr_type()
    |        
    |        ptr = c_void_p()
    |        ptr = addressof(arr)
    |        
    |        return (arr, ptr)
    |
    |    def _alloc_uint_buffer(ptr, count):
    |        a = _allocate_array(c_uint, count)
    |        
    |        ptr = addressof(a[0])
    |        
    |        return 1
    |
    |    _uint_functype = CFUNCTYPE(c_int, POINTER(c_uint), c_uint)
    |    alloc_uint_buffer = _uint_functype(_alloc_uint_buffer)

.. admonition:: dftypes.py

    |   from ctypes import \*
    |   from util import \*
    |
    |   libdfhack = cdll.libdfhack
    |
    |   libdfhack.alloc_uint_buffer_callback = alloc_uint_buffer

Modules
========
Every dfhack module has a corresponding set of C functions.  The functions are named <MODULE>_<FUNCTION>, as in 'Maps_Start', 'Materials_ReadOthers', etc.  The first argument to any module function is a void pointer that points to an instance of the module object in question.
