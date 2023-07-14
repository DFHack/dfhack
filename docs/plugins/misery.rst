misery
======

.. dfhack-tool::
    :summary: Make citizens more miserable.
    :tags: fort gameplay units

When enabled, all of your citizens receive a negative thought about a
particularly nasty soapy bath. You can vary the strength of this negative
thought to increase or decrease the difficulty of keeping your citizens happy.

Usage
-----

::

    enable misery
    misery [status]
    misery <factor>
    misery clear

The default misery factor is ``2``, which will result in a moderate hit to your
dwarves' happiness. Larger numbers increase the challenge.

Examples
--------

``enable misery``
    Start adding bad thoughts about nasty soapy baths to your citizens!

``misery 5``
    Change the strength of the soapy bath negative thought to something quite
    large -- this is very challenging to handle!

``misery clear``
    Clear away negative thoughts added by ``misery``. Note that this will not
    clear negative thoughts that your dwarves accumulated "naturally".
