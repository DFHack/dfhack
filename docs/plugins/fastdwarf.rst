fastdwarf
=========
Controls speedydwarf and teledwarf. Speedydwarf makes dwarves move quickly
and perform tasks quickly. Teledwarf makes dwarves move instantaneously,
but do jobs at the same speed.

:fastdwarf 0:   disables both (also ``0 0``)
:fastdwarf 1:   enables speedydwarf and disables teledwarf (also ``1 0``)
:fastdwarf 2:   sets a native debug flag in the game memory that implements an
                even more aggressive version of speedydwarf.
:fastdwarf 0 1: disables speedydwarf and enables teledwarf
:fastdwarf 1 1: enables both

See `superdwarf` for a per-creature version.
