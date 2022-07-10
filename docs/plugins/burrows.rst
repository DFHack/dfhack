burrows
=======
Miscellaneous burrow control. Allows manipulating burrows and automated burrow
expansion while digging.

Options:

:enable feature ...:
    Enable features of the plugin.
:disable feature ...:
    Disable features of the plugin.
:clear-unit burrow burrow ...:
    Remove all units from the burrows.
:clear-tiles burrow burrow ...:
    Remove all tiles from the burrows.
:set-units target-burrow src-burrow ...:
    Clear target, and adds units from source burrows.
:add-units target-burrow src-burrow ...:
    Add units from the source burrows to the target.
:remove-units target-burrow src-burrow ...:
    Remove units in source burrows from the target.
:set-tiles target-burrow src-burrow ...:
    Clear target and adds tiles from the source burrows.
:add-tiles target-burrow src-burrow ...:
    Add tiles from the source burrows to the target.
:remove-tiles target-burrow src-burrow ...:
    Remove tiles in source burrows from the target.

    For these three options, in place of a source burrow it is
    possible to use one of the following keywords: ABOVE_GROUND,
    SUBTERRANEAN, INSIDE, OUTSIDE, LIGHT, DARK, HIDDEN, REVEALED

Features:

:auto-grow: When a wall inside a burrow with a name ending in '+' is dug
            out, the burrow is extended to newly-revealed adjacent walls.
            This final '+' may be omitted in burrow name args of commands above.
            Digging 1-wide corridors with the miner inside the burrow is SLOW.
