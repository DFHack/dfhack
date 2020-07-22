.. _remote:

=======================
DFHack Remote Interface
=======================

DFHack provides a remote access interface that external tools can connect to and
use to interact with DF. This is implemented with `Google protobuf`_ messages
exchanged over a TCP socket. Both the core and plugins can define
remotely-accessible methods, or **RPC methods**. The RPC methods currently
available are not comprehensive, but can be extended with plugins.

.. _Google protobuf: https://developers.google.com/protocol-buffers

.. contents::
    :local:


Server configuration
====================

DFHack attempts to start a TCP server to listen for remote connections on
startup. If this fails (due to the port being in use, for example), an error
message will be logged to stderr.log.

The server can be configured by setting options in ``dfhack-config/remote-server.json``:

- ``allow_remote`` (default: ``false``): if true, allows connections from hosts
  other than the local machine. This is insecure and may allow arbitrary code
  execution on your machine, so it is disabled by default.
- ``port`` (default: ``5000``): the port that the remote server listens on.
  Overriding this will allow the server to work if you have multiple instances
  of DF running, or if you have something else running on port 5000. Note that
  the ``DFHACK_PORT`` `environment variable <env-vars>` takes precedence over
  this setting and may be more useful for overriding the port temporarily.


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
