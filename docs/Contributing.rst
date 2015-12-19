###########################
How to contribute to DFHack
###########################

.. contents::


.. _contributing-code:

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
* Submit pull requests to the ``develop`` branch, not the ``master`` branch.
  (The ``master`` branch always points at the most recent release)
* Use a new branch for each feature or bugfix so that your changes can be merged independently
  (i.e. not the master or develop branch of your fork).
* If possible, compile on multiple platforms when changing anything that compiles
* It must pass CI - run ``python travis/all.py`` to check this.
* Update ``NEWS.rst`` and ``docs/Authors.rst`` when applicable.
* Create a GitHub pull request once finished
* Submit ideas and bug reports as :issue:`issues on GitHub <>`.
  Posts in the forum thread can easily get missed or forgotten.
* Work on :issue:`reported problems <?q=is:open+-label:idea>`
  will take priority over ideas or suggestions.

.. _contributing-memory-research:

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
``plugins/skeleton/skeleton.cpp`` is provided for this purpose.

Other than through plugins, it is possible to use DFHack via remote access interface,
or by writing scripts in Lua or Ruby.  There are plenty of examples in the scripts folder.

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
documentation is built with Sphinx, which runs automatically at compile time.

DFHack consists of variously licensed code, but invariably weak copyleft.
The main license is zlib/libpng, some bits are MIT licensed, and some are
BSD licensed.  See the `license` for more information.

Feel free to add your own extensions and plugins. Contributing back to
the DFHack repository is welcome and the right thing to do :)

DF data structure definitions
-----------------------------
DFHack uses information about the game data structures, represented via xml files
in the ``library/xml/`` submodule.

See https://github.com/DFHack/df-structures, and the documentation linked in the index.

Data structure layouts are described in files following the ``df.\*.xml`` name pattern.
This information is transformed by a perl script into C++ headers describing the
structures, and associated metadata for the Lua wrapper. These headers and data
are then compiled into the DFHack libraries, thus necessitating a compatibility
break every time layouts change; in return it significantly boosts the efficiency
and capabilities of DFHack code.

Global object addresses are stored in ``symbols.xml``, which is copied to the dfhack
release package and loaded as data at runtime.

Remote access interface
-----------------------
DFHack supports remote access by exchanging Google protobuf messages via a TCP
socket. Both the core and plugins can define remotely accessible methods. The
``dfhack-run`` command uses this interface to invoke ordinary console commands.

Currently the supported set of requests is limited, because the developers don't
know what exactly is most useful.  ``remotefortressreader`` provides a fairly
comprehensive interface for visualisers such as Armok Vision.


Documentation Standards
=======================
DFHack documentation is built with Sphinx, and configured automatically
through CMake.  If you want to build the docs *only*, use this command::

    sphinx-build . docs/html

Whether you're adding new code or just fixing old documentation (and there's plenty),
there are a few important standards for completeness and consistent style.  Treat
this section as a guide rather than iron law, match the surrounding text, and you'll
be fine.

Everything should be documented!  For plugins, it's a work in progress - use
``docs/Plugins.rst`` for now.  Core functions and general explanations should
go in the documents for that component; if it's not clear add a new section
as some may be missing.

Scripts can use a custom autodoc function, based on the Sphinx ``include``
directive and Ruby docstring conventions - any lines between ``=begin`` and
``=end`` are copied into the appropriate scripts documentation page.
They **must** have a heading which exactly matches the command, underlined
with ``=====`` to the same length.  For example, a lua file would have::

    --[[=begin

    add-thought
    ===========
    Adds a thought or emotion to the selected unit.  Can be used by other scripts,
    or the gui invoked by running ``add-thought gui`` with a unit selected.

    =end]]

Ruby scripts use the same syntax, but obviously omit the leading ``--[[`` and
trailing ``]]`` which denote a multiline comment in lua.
``=begin`` and ``=end`` are native syntax (and matched in lua for convenience).

Where the heading for a section is also the name of a command, the spelling
and case should exactly match the command to enter in the DFHack command line.

Try to keep lines within 80-100 characters, so it's readable in plain text -
Sphinx (our documentation system) will make sure paragraphs flow.

If there aren't many options or examples to show, they can go in a paragraph of
text.  Use double-backticks to put commands in monospaced font, like this::

    You can use ``cleanowned scattered x`` to dump tattered or abandoned items.

If the command takes more than three arguments, format the list as a table
called Usage.  The table *only* lists arguments, not full commands.
Input values are specified in angle brackets.  Example::

    Usage:

    :arg1:          A simple argument.
    :arg2 <input>:  Does something based on the input value.
    :Very long argument:
                    Is very specific.

To demonstrate usage - useful mainly when the syntax is complicated, list the
full command with arguments in monospaced font, then indent the next line and
describe the effect::

    ``resume all``
            Resumes all suspended constructions.

If it would be helpful to mention another DFHack command, don't just type the
name - add a hyperlink!  Specify the link target in backticks, and it will be
replaced with the corresponding title and linked:  eg ```autolabor```
=> `autolabor`.  Link targets should be equivalent to the command
described (without file extension), and placed above the heading of that
section like this::

    .. _autolabor:

    autolabor
    =========

Add link targets if you need them, but otherwise plain headings are preferred.
Scripts using the in-source docs option, which should be all of them, have
link targets created automatically.

Other ways to help
==================
DFHack is a software project, but there's a lot more to it than programming.
If you're not comfortable programming, you can help by:

* reporting bugs and incomplete documentation
* improving the documentation
* finding third-party scripts to add
* writing tutorials for newbies

All those things are crucial, and often under-represented.  So if that's
your thing, go get started!

