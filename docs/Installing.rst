.. _installing:

=================
Installing DFHack
=================

.. contents::
    :local:


Requirements
============

DFHack supports Windows, Linux, and macOS, and both 64-bit and 32-bit builds
of Dwarf Fortress.

.. _installing-df-version:

DFHack releases generally only support the version of Dwarf Fortress that they
are named after. For example, DFHack 0.40.24-r5 only supported DF 0.40.24.
DFHack releases *never* support newer versions of DF, because DFHack requires
data about DF that is only possible to obtain after DF has been released.
Occasionally, DFHack releases will be able to maintain support for older
versions of DF - for example, DFHack 0.34.11-r5 supported both DF 0.34.11 and
0.34.10. For maximum stability, you should usually use the latest versions of
both DF and DFHack.

Windows
-------

* DFHack only supports the SDL version of Dwarf Fortress. The "legacy" version
  will *not* work with DFHack (the "small" SDL version is acceptable, however).
* Windows XP and older are *not* supported, due in part to a
  `Visual C++ 2015 bug <https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp>`_

The Windows build of DFHack should work under Wine on other operating systems,
although this is not tested very often. It is recommended to use the native
build for your operating system instead.

.. _installing-reqs-linux:

Linux
-----

Generally, DFHack should work on any modern Linux distribution. There are
multiple release binaries provided - as of DFHack 0.47.04-r1, there are built
with GCC 7 and GCC 4.8 (as indicated by the ``gcc`` component of their
filenames). Using the newest build that works on your system is recommended.
The GCC 4.8 build is built on Ubuntu 14.04 and targets an older glibc, so it
should work on older distributions.

In the event that none of the provided binaries work on your distribution,
you may need to `compile DFHack from source <compile>`.

macOS
-----

OS X 10.6.8 or later is required.


.. _downloading:

Downloading DFHack
==================

Stable builds of DFHack are available on `GitHub <https://github.com/dfhack/dfhack/releases>`_.
GitHub has been known to change their layout periodically, but as of July 2020,
downloads are available at the bottom of the release notes for each release, under a section
named "Assets" (which you may have to expand). The name of the file indicates
which DF version, platform, and architecture the build supports - the platform
and architecture (64-bit or 32-bit) **must** match your build of DF. The DF
version should also match your DF version - see `above <installing-df-version>`
for details. For example:

* ``dfhack-0.47.04-r1-Windows-64bit.zip`` supports 64-bit DF on Windows
* ``dfhack-0.47.04-r1-Linux-32bit-gcc-7.tar.bz2`` supports 32-bit DF on Linux
  (see `installing-reqs-linux` for details on the GCC version indicator)

The `DFHack website <https://dfhack.org/builds>`_ also provides links to
unstable builds. These files have a different naming scheme, but the same
restrictions apply (e.g. a file named ``Windows64`` is for 64-bit Windows DF).

.. warning::

    Do *not* download the source code from GitHub, either from the releases page
    or by clicking "Download ZIP" on the repo homepage. This will give you an
    incomplete copy of the DFHack source code, which will not work as-is. (If
    you want to compile DFHack instead of using a pre-built release, see
    `compile` for instructions.)

Installing DFHack
=================

When you `download DFHack <downloading>`, you will end up with a release archive
(a ``.zip`` file on Windows, or a ``.tar.bz2`` file on other platforms). Your
operating system should have built-in utilities capable of extracting files from
these archives.

The release archives contain several files and folders, including a ``hack``
folder, a ``dfhack-config`` folder, and a ``dfhack.init-example`` file. To
install DFHack, copy all of the files from the DFHack archive into the root DF
folder, which should already include a ``data`` folder and a ``raw`` folder,
among other things. Some packs and other redistributions of Dwarf Fortress may
place DF in another folder, so ensure that the ``hack`` folder ends up next to
the ``data`` folder.

.. note::

    On Windows, installing DFHack will overwrite ``SDL.dll``. This is
    intentional and necessary for DFHack to work, so be sure to choose to
    overwrite ``SDL.dll`` if prompted. (If you are not prompted, you may be
    installing DFHack in the wrong place.)


Uninstalling DFHack
===================

Uninstalling DFHack essentially involves reversing what you did to install
DFHack. On Windows, replace ``SDL.dll`` with ``SDLreal.dll`` first. Then, you
can remove any files that were part of the DFHack archive. DFHack does not
currently maintain a list of these files, so if you want to completely remove
them, you should consult the DFHack archive that you installed for a full list.
Generally, any files left behind should not negatively affect DF.


Upgrading DFHack
================

The recommended approach to upgrade DFHack is to uninstall DFHack first, then
install the new version. This will ensure that any files that are only part
of the older DFHack installation do not affect the new DFHack installation
(although this is unlikely to occur).

It is also possible to overwrite an existing DFHack installation in-place.
To do this, follow the installation instructions above, but overwrite all files
that exist in the new DFHack archive (on Windows, this includes ``SDL.dll`` again).

.. note::

    You may wish to make a backup of your ``dfhack-config`` folder first if you
    have made changes to it. Some archive managers (e.g. Archive Utility on macOS)
    will overwrite the entire folder, removing any files that you have added.


Pre-packaged DFHack installations
=================================

There are :wiki:`several packs available <Utility:Lazy_Newb_Pack>` that include
DF, DFHack, and other utilities. If you are new to Dwarf Fortress and DFHack,
these may be easier to set up. Note that these packs are not maintained by the
DFHack team and vary in their release schedules and contents. Some may make
significant configuration changes, and some may not include DFHack at all.

Linux packages
==============

Third-party DFHack packages are available for some Linux distributions,
including in:

* `AUR <https://aur.archlinux.org/packages/dfhack/>`__, for Arch and related
  distributions
* `RPM Fusion <https://admin.rpmfusion.org/pkgdb/package/nonfree/dfhack/>`__,
  for Fedora and related distributions

Note that these may lag behind DFHack releases. If you want to use a newer
version of DFHack, we generally recommended installing it in a clean copy of DF
in your home folder. Attempting to upgrade an installation of DFHack from a
package manager may break it.
