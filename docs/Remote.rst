.. _remote:

=======================
DFHack Remote Interface
=======================

DFHack provides a remote access interface that external tools can connect to and
use to interact with DF. This is implemented with `Google protobuf`_ messages
exchanged over a TCP socket (which only accepts connections from the local
machine by default). Both the core and plugins can define remotely-accessible
methods (often referred to as **RPC methods**). The RPC methods currently
available are not comprehensive, but can be extended with plugins.

.. _Google protobuf: https://developers.google.com/protocol-buffers

.. contents::
    :local:

Examples
========

The `dfhack-run` command uses the RPC interface to invoke DFHack commands
(or Lua functions) externally.

Plugins that implement RPC methods include:

- `rename`
- `remotefortressreader`
- `isoworldremote`

Plugins that use the RPC API include:

- `stonesense`

Third-party tools that use the RPC API include:

- `Armok Vision <https://github.com/RosaryMala/armok-vision>`_ (:forums:`Bay12 forums thread <146473>`)

Client libraries
================

Some external libraries are available for interacting with the remote interface
from other (non-C++) languages, including:

- `RemoteClientDF-Net <https://github.com/RosaryMala/RemoteClientDF-Net>`_ for C#
- `dfhackrpc <https://github.com/BenLubar/dfhackrpc>`_ for Go
- `dfhack-remote <https://github.com/alexchandel/dfhack-remote>`_ for JavaScript
