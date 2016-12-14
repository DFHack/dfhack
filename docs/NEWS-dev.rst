.. comment
    This is the development changelog file for DFHack.  If you add or change
    anything, note it here under the heading "DFHack Future", in the appropriate
    section. Items within each section are listed in alphabetical order to
    minimise merge conflicts.  Try to match the style and level of detail of the
    other entries.

    This file contains changes that are relevant to users of prereleases. These
    changes should include changes from just the previous release, whether that
    release was stable or not. For instance, a feature added in 0.43.05-alpha1
    should go under "0.43.05-alpha1" here *and* "0.43.05-r1" (or "future") in
    NEWS.rst. A fix in one prerelease for an issue in the previous prerelease
    should just go here in the appropriate section, not in NEWS.rst.

    Sections for each release are added as required, and consist solely of the
    following in order as subheadings::

        Fixes
        Structures
        API Changes
        Additions/Removals
        Other Changes

    When referring to a script, plugin, or command, use backticks (```) to
    create a link to the relevant documentation - and check that the docs are
    still up to date!

    When adding a new release, change "DFHack future" to the appropriate title
    before releasing, and then add a new "DFHack future" section after releasing.

.. _dev-changelog:

#####################
Development Changelog
#####################

.. contents::
   :depth: 2

DFHack 0.43.05-alpha3
=====================

Fixes
-----
- `add-thought`: fixed support for emotion names
- `autofarm`: Made surface farms detect local biome
- `devel/export-dt-ini`: fixed squad_schedule_entry size
- `labormanager`:

    - Now accounts for unit attributes
    - Made instrument-building jobs work (constructed instruments)
    - Fixed deconstructing constructed instruments
    - Fixed jobs in bowyer's shops
    - Fixed trap component jobs
    - Fixed multi-material construction jobs
    - Fixed deconstruction of buildings containing items
    - Fixed interference caused by "store item in vehicle" jobs

- `manipulator`: Fixed crash when selecting a profession from an empty list
- `ruby`:

    - Fixed crash on Win64 due to truncated global addresses
    - Fixed compilation on Win64
    - Use correct raw string length with encodings

Structures
----------
- Changed many ``comment`` XML attributes with version numbers to use new
  ``since`` attribute instead
- ``activity_event_conflictst.sides``: named many fields
- ``building_def.build_key``: fixed size on 64-bit Linux and OS X
- ``historical_kills``:

    - ``unk_30`` -> ``killed_underground_region``
    - ``unk_40`` -> ``killed_region``

- ``historical_kills.killed_undead``: removed ``skeletal`` flag
- ``ui_advmode``: aligned enough so that it doesn't crash (64-bit OS X/Linux)
- ``ui_advmode.show_menu``: changed from bool to enum
- ``unit_personality.emotions.flags``: now a bitfield

API Changes
-----------
- Added ``DFHack::Job::removeJob()`` function
- C++: Removed bitfield constructors that take an initial value. These kept
  bitfields from being used in unions. Set ``bitfield.whole`` directly instead.
- Lua: ``bitfield.whole`` now returns an integer, not a decimal

Additions/Removals
------------------
- Removed source for treefarm plugin (wasn't built)
- Added `modtools/change-build-menu`: Edit the build mode sidebar menus
- Added `modtools/if-entity`: Run a command if the current entity matches a
  given ID
- Added `season-palette`: Swap color palettes with the changes of the seasons

Other changes
-------------
- Changed minimum GCC version to 4.8 on OS X and Linux (earlier versions
  wouldn't have worked on Linux anyway)
- Updated TinyXML from 2.5.3 to 2.6.2
