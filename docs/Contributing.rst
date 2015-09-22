###########################
How to contribute to DFHack
###########################

.. contents::

You can help without coding
===========================
DFHack is a software project, but there's a lot more to it than programming.
If you're not comfortable pregramming, you can help by:

* reporting bugs and incomplete documentation,
* improving the documentation,
* finding third-party scripts to add,
* writing tutorials for newbies

All those things are crucial, and all under-represented.  So if that's
your thing, the rest of this document is about contributing code - go get started!

Contributing Code
=================
Several things should be kept in mind when contributing code to DFHack.

Code Format
-----------

* Four space indents for C++. Never use tabs for indentation in any language.
* LF (Unix style) line terminators
* Avoid trailing whitespace
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines or opening brace at the end of the previous line
  * Braces placed at original indent level if on their own lines
  * #includes should be sorted. C++ libraries first, then dfhack modules, then df structures,
    then local includes. Within each category they should be sorted alphabetically.

How to get new code into DFHack
-------------------------------

* Submit pull requests to the ``develop`` branch, not the master branch. The master branch always points at the most recent release.
* Use new branches for each feature/fix so that your changes can be merged independently (i.e. not the master or develop branch of your fork).
* If possible, compile on multiple platforms when changing anything that compiles
* Update NEWS and doc/Authors.rst when applicable
* Create a Github Pull Request once finished
* Work done against Github issues tagged "bug report" get priority
* Submit ideas and bug reports as issues on Github. Posts in the forum thread can easily get missed or forgotten.

Memory research
---------------
If you want to do memory research, you'll need some tools and some knowledge.
In general, you'll need a good memory viewer and optionally something
to look at machine code without getting crazy :)
Using publicly known information and analyzing the game's data is preferred.

Good windows tools include:

* Cheat Engine
* IDA Pro 5.0 (freely available for non-commercial use)

Good linux tools:

* angavrilov's df-structures gui (visit us on IRC for details).
* edb (Evan's Debugger)
* IDA Pro 5.0 running under Wine
* Some of the tools residing in the ``legacy`` dfhack branch.

Using the library as a developer
================================
Currently, the most direct way to use the library is to write a script or plugin that can be loaded by it.
All the plugins can be found in the 'plugins' folder. There's no in-depth documentation
on how to write one yet, but it should be easy enough to copy one and just follow the pattern.

Other than through plugins, it is possible to use DFHack via remote access interface, or by writing Lua scripts.

The most important parts of DFHack are the Core, Console, Modules and Plugins.

* Core acts as the centerpiece of DFHack - it acts as a filter between DF and SDL and synchronizes the various plugins with DF.
* Console is a thread-safe console that can be used to invoke commands exported by Plugins.
* Modules actually describe the way to access information in DF's memory. You can get them from the Core. Most modules are split into two parts: high-level and low-level. High-level is mostly method calls, low-level publicly visible pointers to DF's data structures.
* Plugins are the tools that use all the other stuff to make things happen. A plugin can have a list of commands that it exports and an onupdate function that will be called each DF game tick.

Rudimentary API documentation can be built using doxygen (see build options with ``ccmake`` or ``cmake-gui``).

DFHack consists of variously licensed code, but invariably weak copyleft.
The main license is zlib/libpng, some bits are MIT licensed, and some are BSD licensed.

Feel free to add your own extensions and plugins. Contributing back to
the dfhack repository is welcome and the right thing to do :)

DF data structure definitions
-----------------------------

DFHack uses information about the game data structures, represented via xml files in the library/xml/ submodule.

See https://github.com/DFHack/df-structures

Data structure layouts are described in files following the df.\*.xml name pattern. This information is transformed by a perl script into C++ headers describing the structures, and associated metadata for the Lua wrapper. These headers and data are then compiled into the DFHack libraries, thus necessitating a compatibility break every time layouts change; in return it significantly boosts the efficiency and capabilities of DFHack code.

Global object addresses are stored in symbols.xml, which is copied to the dfhack release package and loaded as data at runtime.

Remote access interface
-----------------------
DFHack supports remote access by exchanging Google protobuf messages via a TCP socket. Both the core and plugins can define remotely accessible methods. The ``dfhack-run`` command uses this interface to invoke ordinary console commands.

Currently the supported set of requests is limited, because the developers don't know what exactly is most useful.
``remotefortressreader`` provides a fairly comprehensive interface for visualisers such as Armok Vision.

