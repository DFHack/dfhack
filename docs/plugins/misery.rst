misery
======

.. dfhack-tool::
    :summary: Increase the intensity of your citizens' negative thoughts.
    :tags: fort gameplay units

When enabled, negative thoughts that your citizens have will multiply by the
specified factor. This makes it more challenging to keep them happy.

Usage
-----

::

    enable misery
    misery [status]
    misery <factor>
    misery clear

The default misery factor is ``2``, meaning that your dwarves will become
miserable twice as fast.

Examples
--------

``enable misery``
    Start multiplying bad thoughts for your citizens!

``misery 5``
    Make dwarves become unhappy 5 times faster than normal -- this is quite
    challenging to handle!

``misery clear``
    Clear away negative thoughts added by ``misery``. Note that this will not
    clear negative thoughts that your dwarves accumulated "naturally".
