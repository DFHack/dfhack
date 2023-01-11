RemoteFortressReader
====================

.. dfhack-tool::
    :summary: Backend for Armok Vision.
    :tags: untested dev graphics
    :no-command:

.. dfhack-command:: RemoteFortressReader_version
    :summary: Print the loaded RemoteFortressReader version.

.. dfhack-command:: load-art-image-chunk
    :summary: Gets an art image chunk by index.

This plugin provides an API for realtime remote fortress visualization. See
:forums:`Armok Vision <146473>`.

Usage
-----

``RemoteFortressReader_version``
    Print the loaded RemoteFortressReader version.
``load-art-image-chunk <chunk id>``
    Gets an art image chunk by index, loading from disk if necessary.
