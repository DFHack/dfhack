===========================
DFHack development overview
===========================

This page provides an overview of DFHack components. If you are looking to
develop a tool for DFHack, developing a script or plugin is likely the most
straightforward choice.

Other pages that may be relevant include:

- `building-dfhack-index`
- `contributing`
- `documentation`
- `license`

.. contents:: Contents
    :local:

.. _architectural-diagrams:

Architecture diagrams
---------------------

These two diagrams give a very high level overview of where DFHack fits into
the DF call structure and how the pieces of DFHack itself fit together:

.. image:: https://lh3.googleusercontent.com/d/1-2yeNMC7WHgMfZ9iQsDQ0dEbLukd_xyU
  :alt: DFHack logic injection diagram
  :target: https://drive.google.com/file/d/1-2yeNMC7WHgMfZ9iQsDQ0dEbLukd_xyU
  :align: center

When DF loads, it looks for a "dfhooks" library file (named appropriately per
platform, e.g. ``libdfhooks.so`` on Linux). DFHack provides this library file,
and DF calls the API functions at specific points in its initialization code
and main event loop.

In addition, DFHack can "interpose" the virtual methods of DF classes. In
particular, it intercepts calls to the interface functions of each DF
viewscreen class to provide `overlay` functionality.

The dfhooks API is defined in DF's open source component ``g_src``:
https://github.com/Putnam3145/Dwarf-Fortress--libgraphics--/blob/master/g_src/dfhooks.h

.. image:: https://lh3.googleusercontent.com/d/1--JoEQbzKpVUOkRKDD9HxvuCqtom780F
  :alt: DFHack tool call graph
  :target: https://drive.google.com/file/d/1--JoEQbzKpVUOkRKDD9HxvuCqtom780F
  :align: center

DF memory layout is encoded in the xml files of the
`df-structures <https://github.com/DFHack/df-structures>`__ repository. These
XML files are converted into C++ header files during the build process.

The functionality of the DFHack core library is grouped by `Modules`_ that
access DF memory according to the defined structures.

The Lua API layer makes DFHack core facilities available to Lua scripts. Both
the C++ and Lua APIs have a library of convenience functions, though only the
Lua API is `well-documented <lua-api>`. Notably, the entire
`UI widget library<lua-ui-library>` is Lua-only, though C++ plugins can easily
access it via the plugin-Lua interop layer.

Plugins
-------

DFHack plugins are written in C++ and located in the ``plugins`` folder.
Currently, documentation on how to write plugins is somewhat sparse. There are
templates that you can use to get started in the :source:`plugins/examples`
folder, and the source code of existing plugins is also helpful.

If you want to compile a plugin that you have just added, you will need to add a
call to ``DFHACK_PLUGIN`` in :source:`plugins/CMakeLists.txt`.

Plugins have the ability to make one or more commands available to users of the
DFHack console. Examples include `3dveins` (which implements the ``3dveins``
command) and `reveal` (which implements ``reveal``, ``unreveal``, and several
other commands).

Plugins can also register handlers to run on every tick, and can interface with
the built-in `enable` and `disable` commands. For the full plugin API, see the
example :source:`skeleton <plugins/examples/skeleton.cpp>` plugin.

Installed plugins live in the ``hack/plugins`` folder of a DFHack installation,
and the `load` family of commands can be used to load a recompiled plugin
without restarting DF.

Run `plug` at the DFHack prompt for a list of all plugins included in DFHack.

Scripts
-------

DFHack scripts are written in Lua, with a `well-documented library <lua-api>`.
Referring to existing scripts as well as the API documentation is very helpful
when developing new scripts.

Scripts included in DFHack live in a separate
:source-scripts:`scripts repository <>`. This can be found in the ``scripts``
submodule if you have `cloned DFHack <compile-how-to-get-the-code>`, or the
``hack/scripts`` folder of an installed copy of DFHack.

Core
----

The `DFHack core <dfhack-core>` has a variety of low-level functions. It is
responsible for implementing the dfhooks API that DF calls, it provides a
console, and it provides an interface for plugins and scripts to interact with
DF.

Modules
-------

A lot of shared code to interact with DF in more complicated ways is contained
in **modules**. For example, the Units module contains functions for checking
various traits of units, changing nicknames properly, and more. Generally, code
that is useful to multiple plugins and scripts should go in the appropriate
module, if there is one.

Most modules are also `exposed to Lua <lua-cpp-func-wrappers>`, although some
functions (and some entire modules) are currently only available in C++.

Remote access interface
-----------------------

DFHack provides a remote access interface that external tools can connect to and
use to interact with DF. See `remote` for more information.
