.. _installing:

==========
Installing
==========

.. contents::
    :local:

Requirements
============

DFHack supports all operating systems and platforms that Dwarf Fortress itself
supports, which at the moment is the 64-bit versions of Windows and Linux.
The Windows build of DFHack also works well under ``wine`` for platforms that
can't run a native version. When running via ``wine``, use the following commandline::

    wine64 explorer Dwarf\ Fortress.exe

.. _installing-df-version:

DFHack releases generally only support the version of Dwarf Fortress that they
are named after. For example, DFHack 50.05 only supported DF 50.05. DFHack
releases *never* support newer versions of DF -- DFHack requires data about DF
that is only possible to obtain after DF has been released. Occasionally,
DFHack releases will be able to maintain support for older versions of DF - for
example, DFHack 0.34.11-r5 supported both DF 0.34.11 and 0.34.10. For maximum
stability, you should use the latest versions of both DF and DFHack.

.. _downloading:

Downloading DFHack
==================

Stable builds of DFHack are available on
`Steam <https://store.steampowered.com/app/2346660/DFHack>`__
or from our `GitHub <https://github.com/dfhack/dfhack/releases>`__. Either
location will give you exactly the same package.

On Steam, note that DFHack is a separate app, not a DF Steam Workshop mod. You
can run DF with DFHack by launching either the DFHack app or the original Dwarf
Fortress app.

If you download from GitHub, downloads are available at the bottom of the
release notes for each release, under a section named "Assets" (which you may
have to expand). The name of the file indicates which DF version, platform, and
architecture the build supports - the platform and architecture (64-bit or
32-bit) **must** match your build of DF. The DF version should also match your
DF version - see `above <installing-df-version>` for details. For example:

* ``dfhack-50.07-r1-Windows-64bit.zip`` supports 64-bit DF on Windows

.. warning::

    Do *not* download the source code from GitHub, either from the releases page
    or by clicking "Download ZIP" on the repo homepage. This will give you an
    incomplete copy of the DFHack source code, which will not work as-is. (If
    you want to compile DFHack instead of using a pre-built release, please see
    `building-dfhack-index` for instructions.)

Beta releases
-------------

In between stable releases, we may create beta releases to test new features.
These are available via the ``beta`` release channel on Steam or from our
regular Github page as a pre-release tagged with a "beta" or "rc" ("release
candidate") suffix.

Development builds
------------------

If you are actively working with the DFHack team on testing a feature, you may
want to download and install a development build. They are available via the
``testing`` release channel on Steam or can be downloaded from the build
artifact list on GitHub for specific repository commits.

To download a development build from GitHub:

- Ensure you are logged into your GitHub account
- Go to https://github.com/DFHack/dfhack/actions/workflows/build.yml?query=branch%3Adevelop+event%3Apush
- Click on the first entry that has a green checkmark
- Click the number under "Artifacts" (or scroll down)
- Click on the "dfhack-*-build-*" artifact for your platform to download

You can extract this package the same as if you are doing a manual install (see
the next section).

Older releases
--------------

If you are downloading DFHack for very old versions of DF, the binaries for
0.40.15-r1 to 0.34.11-r4 are on DFFD_. Even older versions are available here_.

.. _DFFD: https://dffd.bay12games.com/search.php?string=DFHack&id=15&limit=1000
.. _here: https://dethware.org/dfhack/download

Installing DFHack
=================

If you are installing from Steam, this is handled for you automatically. The
instructions here are for manual installs.

When you `download DFHack <downloading>`, you will end up with a release archive
(a ``.zip`` file on Windows, or a ``.tar.bz2`` file on other platforms). Your
operating system should have built-in utilities capable of extracting files from
these archives.

If you are on Windows, please remember to right click on the file after
downloading, open the file properties, and select the "Unblock" checkbox. This
will prevent issues with Windows antivirus programs.

The release archives contain a ``hack`` folder where DFHack binary and system
data is stored, a ``stonesense`` folder that contains data specific to the
`stonesense` 3d renderer, and various libraries and executable files. To
install DFHack, copy all of the files from the DFHack archive into the root DF
folder, which should already include a ``data`` folder and a ``save`` folder,
among other things. Some redistributions of Dwarf Fortress may place DF in
another folder, so ensure that the ``hack`` folder ends up next to the ``data``
folder, and you'll be fine.

Installing into a wineskin on Mac
---------------------------------

Until DF (and DFHack) is natively available for Mac, you'll have to run the
Windows version under emulation. Here are the instructions for adding DFHack to
a wineskin that has DF installed in it:

#. Find the location of your existing Dwarf Fortress app (default is
   ``/user/applications/Wineskin/``). Control + click and select "Show package
   contents" from the menu.
#. Find the location of the ``Dwarf Fortress`` folder inside the package
   contents (default is ``/drive_c/Program Files/``)
#. Copy the contents of the unzipped DFHack folder (Windows versino) into the
   ``Dwarf Fortress`` folder inside the package.

These instructions were last tested on Mac Sonoma 14.1.2.

Uninstalling DFHack
===================

Just renaming or removing the ``dfhooks`` library file is enough to disable
DFHack. If you would like to remove all DFHack files, consult the DFHack install
archive to see the list of files and remove the corresponding files in the Dwarf
Fortress folder. Any DFHack files left behind will not negatively affect DF.

On Steam, uninstalling DFHack will cleanly remove everything that was installed
with DFHack, so there is nothing else for you to do.

Note that Steam will leave behind the ``dfhack-config`` folder, which contains
all your personal DFHack-related settings and data. If you keep this folder,
all your settings will be restored when you reinstall DFHack later.

Upgrading DFHack
================

Again, if you have installed from Steam, your copy of DFHack will automatically
be kept up to date. This section is for manual installers.

First, remove the ``hack`` and ``stonesense`` folders in their entirety. This
ensures that files that don't exist in the latest version are properly removed
and don't affect your new installation.

Then, follow the instructions in the `Installing DFHack`_ section above, making
sure to choose to overwrite any remaining top-level files when extracting.
