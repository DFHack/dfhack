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
* Update ``changelog.txt`` and ``docs/Authors.rst`` when applicable. See
  `build-changelog` for more information on the changelog format.
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

Good Windows tools include:

* IDA Freeware 7.0 (for non-commercial use, supports 32-bit and 64-bit)
* Cheat Engine

Good Linux tools:

* angavrilov's df-structures gui (32-bit only, visit us on IRC for details)
* IDA Freeware 7.0 (see above)
* edb (Evan's Debugger)
* Some of the tools residing in the ``legacy`` dfhack branch.

Using the library as a developer
================================
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

DF data structure definitions
-----------------------------
DFHack uses information about the game data structures, represented via xml files
in the ``library/xml/`` submodule.

See https://github.com/DFHack/df-structures, and the documentation linked in the index.

Data structure layouts are described in files following the ``df.*.xml`` name pattern.
This information is transformed by a perl script into C++ headers describing the
structures, and associated metadata for the Lua wrapper. These headers and data
are then compiled into the DFHack libraries, thus necessitating a compatibility
break every time layouts change; in return it significantly boosts the efficiency
and capabilities of DFHack code.

Global object addresses are stored in :file:`symbols.xml`, which is copied to the dfhack
release package and loaded as data at runtime.

Remote access interface
-----------------------
DFHack supports remote access by exchanging Google protobuf messages via a TCP
socket. Both the core and plugins can define remotely accessible methods. The
``dfhack-run`` command uses this interface to invoke ordinary console commands.

Currently the supported set of requests is limited, because the developers don't
know what exactly is most useful.  `remotefortressreader` provides a fairly
comprehensive interface for visualisers such as :forums:`Armok Vision <146473>`.


Documentation Standards
=======================
DFHack documentation is built with Sphinx_, and configured automatically
through CMake.  If you want to build the docs *only*, use this command::

    sphinx-build . docs/html

Whether you're adding new code or just fixing old documentation (and there's plenty),
there are a few important standards for completeness and consistent style.  Treat
this section as a guide rather than iron law, match the surrounding text, and you'll
be fine.

Each command should have a short (~54 character) help string, which is shown
by the `ls` command.  For scripts, this is a comment on the first line
(the comment marker and whitespace is stripped).  For plugins it's the second
argument to ``PluginCommand``.  Please make this brief but descriptive!

Everything should be documented!  If it's not clear *where* a particular
thing should be documented, ask on IRC or in the DFHack thread on Bay12 -
as well as getting help, you'll be providing valuable feedback that
makes it easier for future readers!

Scripts can use a custom autodoc function, based on the Sphinx ``include``
directive - anything between the tokens is copied into the appropriate scripts
documentation page.  For Ruby, we follow the built-in docstring convention
(``=begin`` and ``=end``).  For Lua, the tokens are ``[====[`` and ``]====]``
- ordinary multi-line strings.  It is highly encouraged to reuse this string
as the in-console documentation by (eg.) printing it when a ``-help`` argument
is given.

The docs **must** have a heading which exactly matches the command, underlined
with ``=====`` to the same length.  For example, a lua file would have::

    local helpstr = [====[

    add-thought
    ===========
    Adds a thought or emotion to the selected unit.  Can be used by other scripts,
    or the gui invoked by running ``add-thought gui`` with a unit selected.

    ]====]


Where the heading for a section is also the name of a command, the spelling
and case should exactly match the command to enter in the DFHack command line.

Try to keep lines within 80-100 characters, so it's readable in plain text
in the terminal - Sphinx (our documentation system) will make sure
paragraphs flow.

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
Scripts have link targets created automatically.

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

