luadebugger
===========

.. dfhack-tool::
    :summary: Debug Adapter Protocol server for interactively debugging DFHack Lua code.
    :tags: dev

The ``luadebugger`` plugin hosts a `Debug Adapter Protocol
<https://microsoft.github.io/debug-adapter-protocol/>`__ (DAP) server inside
DFHack so that Lua code running in the game can be debugged interactively from
an external client -- breakpoints, stepping, stack traces, variable
inspection, expression evaluation, and variable assignment are all supported.

The plugin is needed because DFHack's normal UI interaction paths cannot be
used for debugging: they all rely on yielding control back to Dwarf Fortress
so it can render and process input, and that is impossible while Lua is
stopped at a breakpoint. Instead, the plugin listens on a TCP socket
(``127.0.0.1:43030`` by default) and speaks DAP directly, so any DAP-capable
client -- most notably Visual Studio Code -- can attach to a running game.

While stopped at a breakpoint, the simulation thread is blocked in the
debugger, so the game is frozen. This is normal; the game resumes when the
debugger continues execution.

Usage
-----

``luadebugger``
    Show whether the debug server is running, which port it is bound to, and
    whether a client is connected.
``luadebugger status``
    Same as above.
``luadebugger start [port]``
    Start the debug server, listening on ``127.0.0.1:<port>``
    (default ``43030``). The plugin starts the server automatically when it is
    enabled; use this to restart it or change the port.
``luadebugger stop``
    Stop the debug server and disconnect any attached client.

The server accepts a single client connection at a time. When a client
attaches, the Lua-side debuggee installs line hooks on the main Lua thread and
on subsequently created coroutines (including coroutines created by
``dfhack.run_script``, ``dfhack.enable_script``, and the Lua interpreter), so
breakpoints apply across the whole Lua environment.

Attaching from VSCode
---------------------

An extension scaffold is provided in ``devtools/vscode-dfhack-lua``. Install
or symlink it into your VSCode extensions directory, then add a launch
configuration::

    {
        "type": "dfhack-lua",
        "request": "attach",
        "name": "Attach to DFHack",
        "host": "127.0.0.1",
        "port": 43030,
        "stopOnEntry": false
    }

With ``stopOnEntry`` set to ``true``, execution pauses on the next Lua line
executed after ``configurationDone``, which is useful for catching script
startup.

Breakpoints set in VSCode are resolved against the chunk name the code was
loaded with (e.g. ``hack/scripts/foo.lua`` style paths used by
``dfhack.script_environment``); paths are normalized and matched by suffix, so
absolute client paths generally match DFHack-internal relative paths.

Attaching with the CLI client
-----------------------------

``devtools/luadebug-cli.py`` is a minimal standalone DAP client::

    python3 devtools/luadebug-cli.py [host] [port]

It accepts commands for breakpoints (``b path:line``), stepping (``n``, ``s``,
``o``), continuing (``c``), pausing, thread listing (``t``), stack traces
(``bt``), variable expansion (``v``), and expression evaluation (``p expr``).

Supported protocol features
---------------------------

- ``initialize``, ``attach``, ``configurationDone``, ``disconnect``,
  ``terminate``
- ``setBreakpoints``, ``setFunctionBreakpoints``,
  ``setExceptionBreakpoints`` (reported but not implemented; exceptions are
  not trapped)
- ``threads``, ``stackTrace``, ``scopes``, ``variables``, ``evaluate``,
  ``setVariable``, ``loadedSources``
- ``continue``, ``next`` (step over), ``stepIn``, ``stepOut``, ``pause``

Notes and limitations
---------------------

- The listener binds to loopback only. Do not expose the port to the network;
  the protocol has no authentication and grants full code execution in the
  Lua environment.
- Only one debug client may be connected at a time. Connecting a second
  client replaces the first.
- Lua output produced while a session is attached is mirrored to the client's
  debug console via DAP ``output`` events in addition to the DFHack console.
- ``stepOut`` may not produce a stop event if the stepped-out frame
  immediately returns and its thread finishes.
