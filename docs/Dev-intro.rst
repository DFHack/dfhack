===========================
DFHack development overview
===========================

DFHack has various components; this page provides an overview of some. If you
are looking to develop a tool for DFHack, developing a script or plugin is
likely the most straightforward choice.

Other pages that may be relevant include:

- `contributing`
- `documentation`
- `license`


.. contents:: Contents
    :local:


Plugins
-------

DFHack plugins are written in C++ and located in the ``plugins`` folder.
Currently, documentation on how to write plugins is somewhat sparse. There are
templates that you can use to get started in the ``plugins/skeleton``
folder, and the source code of existing plugins can also be helpful.

If you want to compile a plugin that you have just added, you will need to add a
call to ``DFHACK_PLUGIN`` in ``plugins/CMakeLists.txt``.

Plugins have the ability to make one or more commands available to users of the
DFHack console. Examples include `3dveins` (which implements the ``3dveins``
command) and `reveal` (which implements ``reveal``, ``unreveal``, and several
other commands).

Plugins can also register handlers to run on every tick, and can interface with
the built-in `enable` and `disable` commands. For the full plugin API, see the
skeleton plugins or ``PluginManager.cpp``.

Installed plugins live in the ``hack/plugins`` folder of a DFHack installation,
and the `load` family of commands can be used to load a recompiled plugin
without restarting DF.

See `plugins-index` for a list of all plugins included in DFHack.

Scripts
-------

DFHack scripts can currently be written in Lua or Ruby. The `Lua API <lua-api>`
is more complete and currently better-documented, however. Referring to existing
scripts as well as the API documentation can be helpful when developing new
scripts.

`Scripts included in DFHack <scripts-index>` live in a separate `scripts repository <https://github.com/dfhack/scripts>`_.
This can be found in the ``scripts`` submodule if you have
`cloned DFHack <compile-how-to-get-the-code>`, or the ``hack/scripts`` folder
of an installed copy of DFHack.

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
