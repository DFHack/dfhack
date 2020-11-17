.. _memory-research:

###############
Memory research
###############

There are a variety of tools that can be used to analyze DF memory - some are
listed here. Note that some of these may be old and unmaintained. If you aren't
sure what tool would be best for your purposes, feel free to ask for advice (on
IRC, Bay12, etc.).

.. contents:: Contents
  :local:


Cross-platform tools
====================

Ghidra
------

Ghidra is a cross-platform reverse-engineering framework (written in Java)
available at https://ghidra-sre.org. It supports analyzing both 32-bit and
64-bit executables for all supported DF platforms. There are some custom DFHack
Ghidra scripts available in the `df_misc`_ repo (look for ``.java`` files).


IDA Freeware 7.0
----------------

Available from `Hex-Rays <https://www.hex-rays.com/products/ida/support/download_freeware/>`_.
Supports analyzing both 32-bit and 64-bit executables for all supported DF platforms.
Some ``.idc`` scripts for IDA are available in the `df_misc`_ repo.

.. _df_misc: https://github.com/DFHack/df_misc


Hopper
------

Runs on macOS and some Linux distributions; available from https://www.hopperapp.com/.
`TWBT <https://github.com/mifki/df-twbt/blob/master/PATCHES.md>`_ uses this to produce some patches.


DFHack tools
------------

Plugins
~~~~~~~

There are a few development plugins useful for low-level memory research. They
are not built by default, but can be built by setting the ``BUILD_DEVEL``
`CMake option <compile-build-options>`. These include:

- ``check-structures-sanity``, which performs sanity checks on the given DF
  object. Note that this will crash in several cases, some intentional, so using
  this with `GDB <linux-gdb>` is recommended.
- ``memview``, which produces a hex dump of a given memory range. It also
  highlights valid pointers, and can be configured to work with `sizecheck`
  to auto-detect object sizes.
- ``vectors``, which can identify instances of ``std::vector`` in a given memory range.

Scripts
~~~~~~~

Several `development scripts <scripts-devel>` can be useful for memory research.
These include (but are not limited to):

- `devel/dump-offsets`
- `devel/find-offsets`
- `devel/lsmem`
- `devel/sc` (requires `sizecheck`)
- `devel/visualize-structure`
- Generally, any script starting with ``devel/find``

.. _sizecheck:

Sizecheck
~~~~~~~~~

Sizecheck is a custom tool that hooks into the memory allocator and inserts a
header indicating the size of every object. The corresponding logic to check for
this header when freeing memory usually works, but is inherently not foolproof.
You should not count on DF being stable when using this.

DFHack's implementation of sizecheck is currently only tested on Linux, although
it probably also works on macOS. It can be built with the ``BUILD_SIZECHECK``
`CMake option <compile-build-options>`, which produces a ``libsizecheck``
library installed in the ``hack`` folder. On Linux, passing ``--sc`` as the
first argument to the ``dfhack`` launcher script will load this library on
startup. On other platforms, or when passing a different argument to the
launcher (such as for `linux-gdb`), you will need to preload this library
manually, by setting ``PRELOAD_LIB`` on Linux (or ``LD_PRELOAD`` if editing the
``dfhack`` launcher script directly), or by editing the ``dfhack`` launcher
script and adding the library to ``DYLD_INSERT_LIBRARIES`` on macOS.

There is also an older sizecheck implementation by Mifki available on
`GitHub <https://github.com/mifki/df-sizecheck>`__ (``b.cpp`` is the main
sizecheck library, and ``win_patch.cpp`` is used for Windows support). To use
this with other DFHack tools, you will likely need to edit the header's
magic number to match what is used in `devel/sc` (search for a hexadecimal
constant starting with ``0x``).

Legacy tools
~~~~~~~~~~~~

Some very old DFHack tools are available in the `legacy branch on GitHub <https://github.com/dfhack/dfhack/tree/legacy/tools>`_.
No attempt is made to support these.


Linux-specific tools
====================

.. _linux-gdb:

GDB
---

`GDB <https://www.gnu.org/software/gdb/>`_ is technically cross-platform, but
tends to work best on Linux, and DFHack currently only offers support for using
GDB on 64-bit Linux. To start with GDB, pass ``-g`` to the DFHack launcher
script:

.. code-block:: shell

    ./dfhack -g

Some basic GDB commands:

- ``run``: starts DF from the GDB prompt. Any arguments will be passed as
  command-line arguments to DF (e.g. `load-save` may be useful).
- ``bt`` will produce a backtrace if DF crashes.

See the `official GDB documentation <https://www.gnu.org/software/gdb/documentation/>`_
for more details.

Other analysis tools
--------------------

The ``dfhack`` launcher script on Linux has support for launching several other
tools alongside DFHack, including Valgrind (as well as Callgrind and Helgrind)
and strace. See the script for the exact command-line option to specify. Note
that currently only one tool at a time is supported, and must be specified
with the first argument to the script.

df-structures GUI
-----------------

This is a tool written by Angavrilov and available on `GitHub <https://github.com/angavrilov/cl-linux-debug>`__.
It only supports 32-bit DF. Some assistance may be available on IRC.


EDB (Evan's debugger)
---------------------

Available on `GitHub <https://github.com/eteran/edb-debugger>`__.


Windows-specific tools
======================

Some people have used `Cheat Engine <https://www.cheatengine.org/>`__ for research in the past.
