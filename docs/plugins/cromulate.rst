cromulate
=========

Tags: `tag/productivity`, `tag/unit`, `tag/adventure`
:dfhack-keybind:`cromulate`

:index:`Collects all widgets into a frobozz electric cromufiler.
<cromulate; Collects all widgets into a frobozz electric cromufiler.>` You might
want to do this if you discover that your widgets have become decromulated. It
is safe to run this command periodically even if you are unsure if that's the
case.

Usage::

    cromulate [all|here] [<options>]

When run without parameters, it lists all your widgets. Add the ``all`` keyword
to collect all widgets into the cromufiler, or the ``here`` keyword to just
collect those under the cursor.

Options:

- ``-d``, ``--destroy``
    Destroy the widgets instead of collecting them into the cromufiler.
- ``-q``, ``--quiet``
    Don't display any informational output. Errors will still be printed to the
    console.

Examples:

- ``cromulate``
    Lists all widgets and their positions
- ``cromlate all``
    Gather all widgets into the cromufiler
- ``cromulate here --destroy``
    Destroys the widgets under the cursor
