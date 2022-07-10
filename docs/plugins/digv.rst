dig
===
This plugin makes many automated or complicated dig patterns easy.

Basic commands:

:digv:      Designate all of the selected vein for digging.
:digvx:     Also cross z-levels, digging stairs as needed.  Alias for ``digv x``.
:digl:      Like ``digv``, for layer stone.  Also supports an ``undo`` option
            to remove designations, for if you accidentally set 50 levels at once.
:diglx:     Also cross z-levels, digging stairs as needed.  Alias for ``digl x``.

:dfhack-keybind:`digv`

.. note::

    All commands implemented by the `dig` plugin (listed by ``ls dig``) support
    specifying the designation priority with ``-p#``, ``-p #``, or ``p=#``,
    where ``#`` is a number from 1 to 7. If a priority is not specified, the
    priority selected in-game is used as the default.
