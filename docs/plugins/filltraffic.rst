.. _alltraffic:
.. _restrictice:
.. _restrictliquids:

filltraffic
===========
Set traffic designations using flood-fill starting at the cursor.
See also `alltraffic`, `restrictice`, and `restrictliquids`.  Options:

:H:     High Traffic
:N:     Normal Traffic
:L:     Low Traffic
:R:     Restricted Traffic
:X:     Fill across z-levels.
:B:     Include buildings and stockpiles.
:P:     Include empty space.

Example:

``filltraffic H``
  When used in a room with doors, it will set traffic to HIGH in just that room.

alltraffic
==========
Set traffic designations for every single tile of the map - useful for resetting
traffic designations.  See also `filltraffic`, `restrictice`, and `restrictliquids`.

Options:

:H:     High Traffic
:N:     Normal Traffic
:L:     Low Traffic
:R:     Restricted Traffic

restrictice
===========
Restrict traffic on all tiles on top of visible ice.
See also `alltraffic`, `filltraffic`, and `restrictliquids`.

restrictliquids
===============
Restrict traffic on all visible tiles with liquid.
See also `alltraffic`, `filltraffic`, and `restrictice`.
