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

.. contents:: Contents
    :local:


.. _remote-server-config:

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


Developing with the remote API
==============================

At a high level, the core and plugins define RPC methods, and external clients
can call these methods. Each method is assigned an ID internally, which clients
use to call it. These method IDs can be obtained using the special ``BindMethod``
method, which has an ID of 0.

Examples
--------

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

.. _remote-client-libs:

Client libraries
----------------

Some external libraries are available for interacting with the remote interface
from other (non-C++) languages, including:

- `RemoteClientDF-Net <https://github.com/RosaryMala/RemoteClientDF-Net>`_ for C#
- `dfhackrpc <https://github.com/BenLubar/dfhackrpc>`_ for Go
- `dfhack-remote <https://github.com/alexchandel/dfhack-remote>`__ for JavaScript
- `dfhack-client-qt <https://github.com/cvuchener/dfhack-client-qt>`_ for C++ with Qt
- `dfhack-client-python <https://github.com/McArcady/dfhack-client-python>`_ for Python (adapted from :forums:`"Blendwarf" <178089>`)
- `dfhack-client-java <https://github.com/McArcady/dfhack-client-java>`_ for Java
- `dfhack-remote <https://docs.rs/dfhack-remote/latest/dfhack_remote/index.html>`__ for Rust


Protocol description
====================

This is a low-level description of the RPC protocol, which may be useful when
developing custom clients.

A WireShark dissector for this protocol is available in the
`df_misc repo <https://github.com/DFHack/df_misc/blob/master/wireshark_dfhack_rpc.lua>`_.


Built-in messages
-----------------
These messages have hardcoded IDs; all others must be obtained through ``BindMethod``.

===  ============ =============================== =======================
ID   Method       Input                           Output
===  ============ =============================== =======================
 0   BindMethod   dfproto.CoreBindRequest         dfproto.CoreBindReply
 1   RunCommand   dfproto.CoreRunCommandRequest   dfproto.EmptyMessage
===  ============ =============================== =======================



Conversation flow
-----------------

* Client → Server: `handshake request`_
* Server → Client: `handshake reply`_
* Repeated 0 or more times:
    * Client → Server: `request`_
    * Server → Client: `text`_ (0 or more times)
    * Server → Client: `result`_ or `failure`_
* Client → Server: `quit`_

Raw message types
-----------------

* All numbers are little-endian
* All strings are ASCII
* A payload size of greater than 64MiB is an error
* See ``RemoteClient.h`` for definitions of constants starting with ``RPC``

handshake request
~~~~~~~~~~~~~~~~~

.. csv-table::
    :align: left
    :header-rows: 1

    Type,    Name,    Value
    char[8], magic,   ``DFHack?\n``
    int32_t, version, 1

handshake reply
~~~~~~~~~~~~~~~

.. csv-table::
    :align: left
    :header-rows: 1

    Type,    Name,    Value
    char[8], magic,   ``DFHack!\n``
    int32_t, version, 1

header
~~~~~~

**Note:** the two fields of this message are sometimes repurposed. Uses of this
message are represented as ``header(x, y)``, where ``x`` corresponds to the ``id``
field and ``y`` corresponds to ``size``.

.. csv-table::
    :align: left
    :header-rows: 1

    Type,    Name
    int16_t, id
    int16_t, (padding - unused)
    int32_t, size

request
~~~~~~~

.. list-table::
    :align: left
    :header-rows: 1
    :widths: 25 75

    * - Type
      - Description
    * - `header`_
      - ``header(id, size)``
    * - buffer
      - Protobuf-encoded payload of the input message type of the method specified by ``id``; length of ``size`` bytes

text
~~~~

.. list-table::
    :align: left
    :header-rows: 1
    :widths: 25 75

    * - Type
      - Description
    * - `header`_
      - ``header(RPC_REPLY_TEXT, size)``
    * - buffer
      - Protobuf-encoded payload of type ``dfproto.CoreTextNotification``; length of ``size`` bytes

result
~~~~~~

.. list-table::
    :align: left
    :header-rows: 1
    :widths: 25 75

    * - Type
      - Description
    * - `header`_
      - ``header(RPC_REPLY_RESULT, size)``
    * - buffer
      - Protobuf-encoded payload of the output message type of the oldest incomplete method call; when received,
        that method call is considered completed. Length of ``size`` bytes.

failure
~~~~~~~

.. list-table::
    :align: left
    :header-rows: 1
    :widths: 25 75

    * - Type
      - Description
    * - `header`_
      - ``header(RPC_REPLY_FAIL, command_result)``
    * - command_result
      - return code of the command (a constant starting with ``CR_``; see ``RemoteClient.h``)

quit
~~~~

**Note:** the server closes the connection after receiving this message.

.. list-table::
    :align: left
    :header-rows: 1
    :widths: 25 75

    * - Type
      - Description
    * - `header`_
      - ``header(RPC_REQUEST_QUIT, 0)``
