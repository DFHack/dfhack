.. highlight:: shell

.. _compile:

###########
Compilation
###########

DFHack builds are available for all supported platforms; see `installing` for
installation instructions. If you are a DFHack end-user, modder, or plan on
writing scripts [lua] (not plugins), it is generally recommended (and easier) to use
these `builds <https://github.com/DFHack/dfhack/releases>`_ instead of compiling DFHack from source.

However, if you are looking to develop plugins, work on the DFHack core, make
complex changes to DF-structures, or anything else that requires compiling
DFHack from source, this document will walk you through the build process. Note
that some steps may be unconventional compared to other projects, so be sure to
pay close attention if this is your first time compiling DFHack.

.. contents:: Contents
  :local:
  :depth: 2

.. _compile-how-to-get-the-code:

How to get the code
===================
DFHack uses Git for source control; instructions for installing Git can be found
in the platform-specific sections below. The code is hosted on
`GitHub <https://github.com/DFHack/dfhack>`_, and can be downloaded with::

    git clone --recursive https://github.com/DFHack/dfhack
    cd dfhack

If your version of Git does not support the ``--recursive`` flag, you will need
to omit it and run ``git submodule update --init`` after entering the dfhack
directory.

This will check out the code on the default branch of the GitHub repo, currently
``develop``, which may be unstable. If you want code for the latest stable
release, you can check out the ``master`` branch instead::

    git checkout master
    git submodule update

In general, a single DFHack clone is suitable for development - most Git
operations such as switching branches can be done on an existing clone. If you
find yourself cloning DFHack frequently as part of your development process, or
getting stuck on anything else Git-related, feel free to reach out to us for
assistance.

.. admonition:: Offline builds

  If you plan to build DFHack on a machine without an internet connection (or
  with an unreliable connection), see `note-offline-builds` for additional
  instructions.

.. admonition:: Working with submodules

  DFHack uses submodules extensively to manage its subprojects (including the
  ``scripts`` folder and DF-structures in ``library/xml``). Failing to keep
  submodules in sync when switching between branches can result in build errors
  or scripts that don't work. In general, you should always update submodules
  whenever you switch between branches in the main DFHack repo with
  ``git submodule update``. (If you are working on bleeding-edge DFHack and
  have checked out the master branch of some submodules, running ``git pull``
  in those submodules is also an option.)

  Rarely, we add or remove submodules. If there are any changes to the existence
  of submodules when you switch between branches, you should run
  ``git submodule update --init`` instead (adding ``--init`` to the above
  command).

  Some common errors that can arise when failing to update submodules include:

  * ``fatal: <some path> does not exist`` when performing Git operations
  * Build errors, particularly referring to structures in the ``df::`` namespace
    or the ``library/include/df`` folder
  * ``Not a known DF version`` when starting DF
  * ``Run 'git submodule update --init'`` when running CMake

  Submodules are a particularly confusing feature of Git. The
  `Git Book <https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ has a
  thorough explanation of them (as well as of many other aspects of Git) and
  is a recommended resource if you run into any issues. Other DFHack developers
  are also able to help with any submodule-related (or Git-related) issues
  you may encounter.

All Platforms
=============
Before you can compile the code you'll need to configure your build with cmake. Some IDEs can do this
for you, but it's more common to do it from the command line. Windows developers can refer to the
Windows section below for batch files that can be used to avoid opening a terminal/command-prompt.

You should seek cmake's documentation online or via ``cmake --help`` to see how the command works. See
the `build-options` page for help finding the DFHack build options relevant to you.

Before compiling code, you'll of course need code to compile. This **will include** the submodules, so
be sure you've read the section about getting the code.

.. _compile-linux:

Linux
=====

Building is fairly straightforward. Enter the ``build`` folder (or create an
empty folder in the DFHack directory to use instead) and start the build like this::

    cd build
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    ninja install  # or ninja -jX install to specify the number of cores (X) to use

<path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
version for the DFHack you are building. This will build the library along
with the normal set of plugins and install them into your DF folder.

Alternatively, you can use ccmake instead of cmake::

    cd build
    ccmake .. -G Ninja
    ninja install

This will show a curses-based interface that lets you set all of the
extra options. You can also use a cmake-friendly IDE like KDevelop 4
or the cmake-gui program.

.. _compile-windows:

Windows
=======
There are several different batch files in the ``win32`` and ``win64``
subfolders in the ``build`` folder, along with a script that's used for picking
the DF path. Use the subfolder corresponding to the architecture that you want
to build for.

First, run ``set_df_path.vbs`` and point the dialog that pops up at
a suitable DF installation which is of the appropriate version for the DFHack
you are compiling. The result is the creation of the file ``DF_PATH.txt`` in
the build directory. It contains the full path to the destination directory.
You could therefore also create this file manually - or copy in a pre-prepared
version - if you prefer.

Next, run one of the scripts with ``generate`` prefix. These create the MSVC
solution file(s):

* ``all`` will create a solution with everything enabled (and the kitchen sink).
* ``gui`` will pop up the CMake GUI and let you choose what to build.
  This is probably what you want most of the time. Set the options you are interested
  in, then hit configure, then generate. More options can appear after the configure step.
* ``minimal`` will create a minimal solution with just the bare necessities -
  the main library and standard plugins.
* ``release`` will create a solution with everything that should be included in
  release builds of DFHack. Note that this includes documentation, which requires
  Python.

Then you can either open the solution with MSVC or use one of the msbuild scripts.

Visual Studio IDE
-----------------
After running the CMake generate script you will have a new folder called VC2022
or VC2022_32, depending on the architecture you specified. Open the file
``dfhack.sln`` inside that folder. If you have multiple versions of Visual
Studio installed, make sure you open with Visual Studio 2022.

The first thing you must then do is ensure the build type is not Debug, which 
cannot be used on Windows. Debug is not binary-compatible with DF.
If you try to use a debug build with DF, you'll only get crashes and for this
reason the Windows "debug" scripts actually do RelWithDebInfo builds.
After loading the Solution, change the Build Type to either ``Release``
or ``RelWithDebInfo``.

Then build the ``INSTALL`` target listed under ``CMakePredefinedTargets``.

Command Line
------------
In the build directory you will find several ``.bat`` files:

* Scripts with ``build`` prefix will only build DFHack.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

Compiling from the command line is generally the quickest and easiest option.
Modern Windows terminal emulators such as `Cmder <https://cmder.app/>`_ or
`Windows Terminal <https://github.com/microsoft/terminal>`_ provide a better
experience by providing more scrollback and larger window sizes.

.. _compile-macos:

macOS
=====

NOTE: this section is currently outdated. Once DF itself can build on macOS
again, we will match DF's build environment and update the instructions here.

DFHack functions similarly on macOS and Linux, and the majority of the
information above regarding the build process (CMake and Ninja) applies here
as well.

DFHack can officially be built on macOS only with GCC 4.8 or 7. Anything newer than 7
will require you to perform extra steps to get DFHack to run (see `osx-new-gcc-notes`),
and your build will likely not be redistributable.

Building
--------

* Get the DFHack source as per section `compile-how-to-get-the-code`, above.
* Set environment variables

  Homebrew (if installed elsewhere, replace /usr/local with ``$(brew --prefix)``)::

    export CC=/usr/local/bin/gcc-7
    export CXX=/usr/local/bin/g++-7

  Macports::

    export CC=/opt/local/bin/gcc-mp-7
    export CXX=/opt/local/bin/g++-mp-7

  Change the version numbers appropriately if you installed a different version of GCC.

  If you are confident that you have GCC in your path, you can omit the absolute paths::

    export CC=gcc-7
    export CXX=g++-7

  (adjust as needed for different GCC installations)

* Build DFHack::

    mkdir build-osx
    cd build-osx
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    ninja install  # or ninja -jX install to specify the number of cores (X) to use

  <path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
  version for the DFHack you are building.

.. _osx-new-gcc-notes:

Notes for GCC 8+ or OS X 10.10+ users
-------------------------------------

If you have issues building on OS X 10.10 (Yosemite) or above, try defining
the following environment variable::

    export MACOSX_DEPLOYMENT_TARGET=10.9

If you build with a GCC version newer than 7, DFHack will probably crash
immediately on startup, or soon after. To fix this, you will need to replace
``hack/libstdc++.6.dylib`` with a symlink to the ``libstdc++.6.dylib`` included
in your version of GCC::

  cd <path to df>/hack && mv libstdc++.6.dylib libstdc++.6.dylib.orig &&
  ln -s [PATH_TO_LIBSTDC++] .

For example, with GCC 6.3.0, ``PATH_TO_LIBSTDC++`` would be::

  /usr/local/Cellar/gcc@6/6.3.0/lib/gcc/6/libstdc++.6.dylib  # for 64-bit DFHack
  /usr/local/Cellar/gcc@6/6.3.0/lib/gcc/6/i386/libstdc++.6.dylib  # for 32-bit DFHack

**Note:** If you build with a version of GCC that requires this, your DFHack
build will *not* be redistributable. (Even if you copy the ``libstdc++.6.dylib``
from your GCC version and distribute that too, it will fail on older OS X
versions.) For this reason, if you plan on distributing DFHack, it is highly
recommended to use GCC 4.8 or 7.

.. _osx-m1-notes:

Notes for M1 users
------------------

Alongside the above, you will need to follow these additional steps to get it
running on Apple silicon.

Install an x86 copy of ``homebrew`` alongside your existing one. `This
stackoverflow answer <https://stackoverflow.com/a/64951025>`__ describes the
process.

Follow the normal macOS steps to install ``cmake`` and ``gcc`` via your x86 copy of
``homebrew``. Note that this will install a GCC version newer than 7, so see
`osx-new-gcc-notes`.

In your terminal, ensure you have your path set to the correct homebrew in
addition to the normal ``CC`` and ``CXX`` flags above::

  export PATH=/usr/local/bin:$PATH

Windows cross compiling from Linux (running DF inside docker)
=============================================================

.. highlight:: bash

You can use docker to build DFHack for Windows. These instructions were developed
on a Linux host system.

.. contents::
  :local:
  :depth: 1

Step 1: prepare a build container
---------------------------------

On your Linux host, install and run the docker daemon and then run these commands::

    xhost +local:root
    docker run -it --env="DISPLAY" --env="QT_X11_NO_MITSHM=1" --volume=/tmp/.X11-unix:/tmp/.X11-unix --user buildmaster --name dfhack-win ghcr.io/dfhack/build-env:master

The ``xhost`` command and ``--env`` parameters are there so you can eventually
run Dwarf Fortress from the container and have it display on your host.

Step 2: build DFHack
--------------------

The ``docker run`` command above will give you a shell prompt (as the ``buildmaster`` user) in the
container. Inside the container, run the following commands::

    git clone https://github.com/DFHack/dfhack.git
    cd dfhack
    git submodule update --init
    cd build
    dfhack-configure windows 64 Release
    dfhack-make

Inside the ``dfhack-*`` scripts there are several commands that set up the wine
server. Each invocation of a Windows tool will cause wine to run in the container.
Preloading the wineserver and telling it not to exit will speed configuration and
compilation up considerably (approx. 10x). You can configure and build DFHack
with regular ``cmake`` and ``ninja`` commands, but your build will go much slower.

Step 3: copy Dwarf Fortress to the container
--------------------------------------------

First, create a directory in the container to house the Dwarf Fortress binary and
assets::

    mkdir ~/df

If you can just download Dwarf Fortress directly into the container, then that's fine.
Otherwise, you can do something like this in your host Linux environment to copy an
installed version to the container::

    cd ~/.steam/steam/steamapps/common/Dwarf\ Fortress/
    docker cp . dfhack-win:df/

Step 4: install DFHack and run DF
---------------------------------

Back in the container, run the following commands::

    cd dfhack/build
    cmake .. -DCMAKE_INSTALL_PREFIX=/home/buildmaster/df
    ninja install
    cd ~/df
    wine64 "Dwarf Fortress.exe"

Other notes
-----------

Closing your shell will kick you out of the container. Run this command on your Linux
host when you want to reattach::

    docker start -ai dfhack-win

If you edit code and need to rebuild, run ``dfhack-make`` and then ``ninja install``.
That will handle all the wineserver management for you.

Cross-compiling windows files for running DF in Steam for Linux
===============================================================

.. highlight:: bash

If you wish, you can use Docker to build just the Windows files to copy to your
existing Steam installation on Linux.

.. contents::
  :local:
  :depth: 1

Step 1: Get dfhack, and run the build script
--------------------------------------------

Check out ``dfhack`` into another directory, and run the build script::

   git clone https://github.com/DFHack/dfhack.git
   cd dfhack
   git submodule update --init --recursive
   cd build
   ./build-win64-from-linux.sh

The script will mount your host's ``dfhack`` directory to docker, use it to
build the artifacts in ``build/win64-cross``, and put all the files needed to
install in ``build/win64-cross/output``.

If you need to run ``docker`` using ``sudo``, run the script using ``sudo``
rather than directly::

  sudo ./build-win64-from-linux.sh

Step 2: install dfhack to your Steam DF install
-----------------------------------------------
As the script will tell you, you can then copy the files into your DF folder::

   # Optional -- remove the old hack directory in case we leave files behind
   rm ~/.local/share/Steam/steamapps/common/"Dwarf Fortress"/hack
   cp -r win64-cross/output/* ~/.local/share/Steam/steamapps/common/"Dwarf Fortress"/

Afterward, just run DF as normal.

.. _note-offline-builds:

Building DFHack Offline
=======================
As of 0.43.05, DFHack downloads several files during the build process, depending
on your target OS and architecture. If your build machine's internet connection
is unreliable, or nonexistent, you can download these files in advance.

First, you must locate the files you will need. These can be found in the
`dfhack-bin repo <https://github.com/DFHack/dfhack-bin/releases>`_. Look for the
most recent version number *before or equal to* the DF version which you are
building for. For example, suppose "0.43.05" and "0.43.07" are listed. You should
choose "0.43.05" if you are building for 0.43.05 or 0.43.06, and "0.43.07" if
you are building for 0.43.07 or 0.43.08.

Then, download all of the files you need, and save them to ``<path to DFHack
clone>/CMake/downloads/<any filename>``. The destination filename you choose
does not matter, as long as the files end up in the ``CMake/downloads`` folder.
You need to download all of the files for the architecture(s) you are building
for. For example, if you are building for 32-bit Linux and 64-bit Windows,
download all files starting with ``linux32`` and ``win64``. GitHub should sort
files alphabetically, so all the files you need should be next to each other.

.. note::

  * Any files containing "allegro" in their filename are only necessary for
    building `stonesense`. If you are not building Stonesense, you don't have to
    download these, as they are larger than any other listed files.

It is recommended that you create a build folder and run CMake to verify that
you have downloaded everything at this point, assuming your download machine has
CMake installed. This involves running a "generate" batch script on Windows, or
a command starting with ``cmake .. -G Ninja`` on Linux and macOS, following the
instructions in the sections above. CMake should automatically locate files that
you placed in ``CMake/downloads``, and use them instead of attempting to
download them.
