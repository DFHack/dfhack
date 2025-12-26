.. _build-options:

#############
Build Options
#############

.. contents:: Typical Options
  :local:
  :depth: 1

There are a variety of other settings which you can find in CMakeCache.txt in
your build folder or by running ``ccmake`` (or another CMake GUI). Most
DFHack-specific settings begin with ``BUILD_`` and control which parts of DFHack
are built.

Typical usage may look like::

    # Plugin development with updated documentation
    cmake ./ -G Ninja -B builds/debug-info/ -DCMAKE_INSTALL_PREFIX=<path to DF> -DCMAKE_BUILD_TYPE:string=RelWithDebInfo -DBUILD_DOCS:bool=ON -DBUILD_PLUGINS=1
    # Core DFHack only
    cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=<path to DF> -DCMAKE_BUILD_TYPE:string=RelWithDebInfo -DBUILD_TESTS -DBUILD_DOCS:0 -DBUILD_PLUGINS=0

.. admonition:: Modifying Build Options

    You can typically run new cmake commands from your build directory to turn on/off options.
    Of course the generator selection is not something you can change, but the rest are.

    Additionally, you can edit the build settings in CMakeCache.txt. You also have cmake's
    configuration utility ``ccmake``.

Generator
=========
For the uninitiated, the generator is what allows cmake to, of course, generate
visual studio solution & project files, a makefile, or anything else.
Your selection of generator comes down to preference and availability.

Visual Studio
-------------
To generate visual studio project files, you'll need to select a particular version of
visual studio, and match that to your system's generator list viewed with ``cmake --help``

example::

    cmake .. -G "Visual Studio 17 2022"

Ninja
-----
The generally preferred build system where available.

example::

    cmake .. -G Ninja

Install Location
================
This is the location where DFHack will be installed.

Variable: ``CMAKE_INSTALL_PREFIX``

Usage::

    cmake .. -DCMAKE_INSTALL_PREFIX=<path to df>

The path to df will of course depend on your system. If the directory exists it is
recommended to use ``~/.dwarffortress`` to avoid permission troubles.

Build type
==========
This is the type of build you want. This controls what information about symbols and
line numbers the debugger will have available to it.

Variable: ``CMAKE_BUILD_TYPE``

Usage::

    cmake .. -DCMAKE_BUILD_TYPE:string=RelWithDebInfo

Options:

* Release
* RelWithDebInfo

Target architecture (32/64-bit)
===============================
You can set this if you need 32-bit binaries or are looking to be explicit about
building 64-bit.

Variable: ``DFHACK_BUILD_ARCH``

Usage::

    cmake .. -DDFHACK_BUILD_ARCH=32

Options:

* '32'
* '64' (default option)

Library
=======
This will only be useful if you're looking to avoid building the library core, as it builds by default.

Variable: ``BUILD_LIBRARY``

Usage::

    cmake .. -DBUILD_LIBRARY:bool=OFF
    cmake .. -DBUILD_LIBRARY=0

Testing
=======
Regression testing will be arriving in the future, but for now there are only tests written in lua.

Variables:

* ``BUILD_TESTING`` (will build unit tests, in the future)
* ``BUILD_TESTS`` (installs lua tests)

Usage::

    cmake .. -DBUILD_TESTS:bool=ON
    cmake .. -DBUILD_TESTS=1

Plugins
=======
If you're doing plugin development.

Variable: ``BUILD_PLUGINS``

Usage::

    cmake .. -DBUILD_PLUGINS:bool=ON
    cmake .. -DBUILD_PLUGINS=1

.. _building-documentation:

Documentation
=============
If you need to build `documentation <documentation>`.

.. note::

    These options are primarily useful for verifying that the end-to-end process
    for building and packaging the documentation is working as expected. For
    iterating on documentation changes, `faster alternatives <docs-build>` are
    available.

Variables:

* ``BUILD_DOCS``: enables the default documentation build
* ``BUILD_DOCS_NO_HTML``: disables the HTML documentation build (only builds the text documentation used in-game)

Usage::

    cmake .. -DBUILD_DOCS:bool=ON
    cmake .. -DBUILD_DOCS=1
    cmake .. -DBUILD_DOCS_NO_HTML:bool=ON
    cmake .. -DBUILD_DOCS_NO_HTML=1

The generated documentation is stored in ``docs/html`` and ``docs/text`` (respectively)
in the root DFHack folder, and they will both be installed to ``hack/docs`` when you
install DFHack. The html and txt files will intermingle, but will not interfere with
one another.
