===========================
DFHack development overview
===========================

DFHack has various components; this page provides an overview of some. If you
are looking to develop a tool for DFHack, developing a script or plugin is
likely the most straightforward choice.

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

These two diagrams give a very high level overview of where DFHack injects
itself in the DF call structure and how the pieces of DFHack itself fit
together:

.. image:: https://drive.google.com/uc?export=download&id=1-2yeNMC7WHgMfZ9iQsDQ0dEbLukd_xyU
  :alt: DFHack logic injection diagram
  :target: https://drive.google.com/file/d/1-2yeNMC7WHgMfZ9iQsDQ0dEbLukd_xyU
  :align: center

As seen in the diagram Dwarf Fortress utilizes the SDL library, this provides us with an easy to isolate
injection point for DFHack.

.. image:: https://drive.google.com/uc?export=download&id=1--JoEQbzKpVUOkRKDD9HxvuCqtom780F
  :alt: DFHack tool call graph
  :target: https://drive.google.com/file/d/1--JoEQbzKpVUOkRKDD9HxvuCqtom780F
  :align: center

Plugins
-------

DFHack plugins are written in C++ and located in the ``plugins`` folder.
Currently, documentation on how to write plugins is somewhat sparse. There are
templates that you can use to get started in the ``plugins/examples``
folder, and the source code of existing plugins can also be helpful.

If you want to compile a plugin that you have just added, you will need to add a
call to ``DFHACK_PLUGIN`` in ``plugins/CMakeLists.txt``.

Plugins have the ability to make one or more commands available to users of the
DFHack console. Examples include `3dveins` (which implements the ``3dveins``
command) and `reveal` (which implements ``reveal``, ``unreveal``, and several
other commands).

Plugins can also register handlers to run on every tick, and can interface with
the built-in `enable` and `disable` commands. For the full plugin API, see the
example ``skeleton`` plugin or ``PluginManager.cpp``.

Installed plugins live in the ``hack/plugins`` folder of a DFHack installation,
and the `load` family of commands can be used to load a recompiled plugin
without restarting DF.

Run `plug` at the DFHack prompt for a list of all plugins included in DFHack.

Scripts
-------

DFHack scripts are written in Lua, with a `well-documented library <lua-api>`.
Referring to existing scripts as well as the API documentation can be helpful
when developing new scripts.

Scripts included in DFHack live in a separate
:source-scripts:`scripts repository <>`. This can be found in the ``scripts``
submodule if you have `cloned DFHack <compile-how-to-get-the-code>`, or the
``hack/scripts`` folder of an installed copy of DFHack.

Core
----

The `DFHack core <dfhack-core>` has a variety of low-level functions. It is
responsible for hooking into DF (via SDL), providing a console, and providing an
interface for plugins and scripts to interact with DF.

Modules
-------

A lot of shared code to interact with DF in more complicated ways is contained
in **modules**. For example, the Units module contains functions for checking
various traits of units, changing nicknames properly, and more. Generally, code
that is useful to multiple plugins and scripts should go in the appropriate
module, if there is one.

Several modules are also `exposed to Lua <lua-cpp-func-wrappers>`, although
some functions (and some entire modules) are currently only available in C++.

Remote access interface
-----------------------

DFHack provides a remote access interface that external tools can connect to and
use to interact with DF. See `remote` for more information.
