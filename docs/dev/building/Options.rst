.. _build-options:

#############
Build Options
#############

There are a variety of other settings which you can find in CMakeCache.txt in
your build folder or by running ``ccmake`` (or another CMake GUI). Most
DFHack-specific settings begin with ``BUILD_`` and control which parts of DFHack
are built.

Typical usage may look like::

    # Plugin development with updated documentation
    cmake . -G Ninja -B builds/debug-info/ -DCMAKE_INSTALL_PREFIX=<path to DF> -DCMAKE_BUILD_TYPE:string=RelWithDebInfo -DBUILD_DOCS:bool=ON -DBUILD_PLUGINS=1
    # Core DFHack only
    cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=<path to DF> -DCMAKE_BUILD_TYPE:string=RelWithDebInfo -DBUILD_TESTS -DBUILD_DOCS:0 -DBUILD_PLUGINS=0

Generator
---------
For the uninitiated, the generator is what allows cmake to, of course, generate
visual studio solution & project files, a makefile, or anything else.
Your selection of generator comes down to preference and availability.

Visual Studio
=============
To generate visual studio project files, you'll need to select a particular version of
visual studio, and match that to your system's generator list viewed with ``cmake --help``

example::

    cmake . -G "Visual Studio 17 2022"

Ninja
=====
The generally preferred build system where available.

Core
----
todo:

Plugins
-------
todo:

.. _building-documentation:

Documentation
-------------

Enabling the ``BUILD_DOCS`` CMake option will cause the documentation to be built
whenever it changes as part of the normal DFHack build process. There are several
ways to do this:

* When initially running CMake, add ``-DBUILD_DOCS:bool=ON`` to your ``cmake``
  command. For example::

    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DBUILD_DOCS:bool=ON -DCMAKE_INSTALL_PREFIX=<path to DF>

* If you have already run CMake, you can simply run it again from your build
  folder to update your configuration::

    cmake .. -DBUILD_DOCS:bool=ON

* You can edit the ``BUILD_DOCS`` setting in CMakeCache.txt directly

* You can use the CMake GUI or ``ccmake`` to change the ``BUILD_DOCS`` setting

* On Windows, if you prefer to use the batch scripts, you can run
  ``generate-msvc-gui.bat`` and set ``BUILD_DOCS`` through the GUI. If you are
  running another file, such as ``generate-msvc-all.bat``, you will need to edit
  the batch script to add the flag. You can also run ``cmake`` on the command line,
  similar to other platforms.

By default, both HTML and text docs are built by CMake. The generated
documentation is stored in ``docs/html`` and ``docs/text`` (respectively) in the
root DFHack folder, and they will both be installed to ``hack/docs`` when you
install DFHack. The html and txt files will intermingle, but will not interfere
with one another.
