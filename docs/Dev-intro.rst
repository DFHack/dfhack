===========================
DFHack development overview
===========================

Currently, the most direct way to use the library is to write a script or plugin that can be loaded by it.
All the plugins can be found in the 'plugins' folder. There's no in-depth documentation
on how to write one yet, but it should be easy enough to copy one and just follow the pattern.
``plugins/skeleton/skeleton.cpp`` is provided for this purpose.

Other than through plugins, it is possible to use DFHack via remote access interface,
or by writing scripts in Lua or Ruby.  There are plenty of examples in the scripts folder.
The `lua-api` is quite well documented.

The most important parts of DFHack are the Core, Console, Modules and Plugins.

* Core acts as the centerpiece of DFHack - it acts as a filter between DF and
  SDL and synchronizes the various plugins with DF.
* Console is a thread-safe console that can be used to invoke commands exported by Plugins.
* Modules actually describe the way to access information in DF's memory. You
  can get them from the Core. Most modules are split into two parts: high-level
  and low-level. High-level is mostly method calls, low-level publicly visible
  pointers to DF's data structures.
* Plugins are the tools that use all the other stuff to make things happen.
  A plugin can have a list of commands that it exports and an onupdate function
  that will be called each DF game tick.

Rudimentary API documentation can be built using doxygen (see build options
in ``CMakeCache.txt`` or with ``ccmake`` or ``cmake-gui``).  The full DFHack
documentation is built with Sphinx_, which runs automatically at compile time.

.. _Sphinx: http://www.sphinx-doc.org

DFHack consists of variously licensed code, but invariably weak copyleft.
The main license is zlib/libpng, some bits are MIT licensed, and some are
BSD licensed.  See the `license` for more information.

Feel free to add your own extensions and plugins. Contributing back to
the DFHack repository is welcome and the right thing to do :)


Remote access interface
-----------------------
DFHack supports remote access by exchanging Google protobuf messages via a TCP
socket. Both the core and plugins can define remotely accessible methods. The
``dfhack-run`` command uses this interface to invoke ordinary console commands.

Currently the supported set of requests is limited, because the developers don't
know what exactly is most useful.  `remotefortressreader` provides a fairly
comprehensive interface for visualisers such as :forums:`Armok Vision <146473>`.

