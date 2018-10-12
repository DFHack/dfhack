################
Compiling DFHack
################

You don't need to compile DFHack unless you're developing plugins or working on the core.

For users, modders, and authors of scripts it's better to download
and `install the latest release instead <installing>`.

.. contents::
   :depth: 2

.. _compile-how-to-get-the-code:

How to get the code
===================
DFHack doesn't have any kind of system of code snapshots in place, so you will have to
get code from the GitHub repository using Git.  How to get Git is described under
the instructions for each platform.

To get the latest release code (master branch)::

    git clone --recursive https://github.com/DFHack/dfhack
    cd dfhack

If your version of Git does not support the ``--recursive`` flag, you will need to omit it and run
``git submodule update --init`` after entering the dfhack directory.

To get the latest development code (develop branch), clone as above and then::

  git checkout develop
  git submodule update

Generally, you should only need to clone DFHack once.

**Important note regarding submodule update after pulling or changing branches**:

You must run ``git submodule update`` every time you change branches, such as
when switching between the master and develop branches or vice versa. You also
must run it after pulling any changes to submodules from the DFHack repo. If a
submodule only exists on the newer branch, or if a commit you just pulled
contains a new submodule, you need to run ``git submodule update --init``.
Failure to do this may result in a variety of errors, including ``fatal: <path>
does not exist`` when using Git, errors when building DFHack, and ``not a known
DF version`` when starting DF.

**More notes**:

* `note-offline-builds` - read this if your build machine may not have an internet connection!
* `note-old-git-and-dfhack`

Contributing to DFHack
======================
If you want to get involved with the development, create an account on
GitHub, make a clone there and then use that as your remote repository instead.

We'd love that; join us on IRC_ (#dfhack channel on freenode) for discussion,
and whenever you need help.

.. _IRC: https://webchat.freenode.net/?channels=dfhack

(Note: for submodule issues, please see the above instructions first!)

For lots more details on contributing to DFHack, including pull requests, code format,
and more, please see `contributing-code`.


Build settings
==============

Generator
---------

The ``Ninja`` CMake build generator is the prefered build method on Linux and
macOS, instead of ``Unix Makefiles``, which is the default. You can select Ninja
by passing ``-G Ninja`` to CMake. Incremental builds using Unix Makefiles can be
much slower than Ninja builds.

::

    cmake .. -G Ninja

.. warning::

  Most other CMake settings can be changed by running ``cmake`` again, but the
  generator cannot be changed after ``cmake`` has been run without creating a
  new build folder. Do not forget to specify this option.

Build type
----------

``cmake`` allows you to pick a build type by changing the ``CMAKE_BUILD_TYPE`` variable::

    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Valid and useful build types include 'Release' and 'RelWithDebInfo'. The default
build type is 'Release'.

Target architecture (32-bit vs. 64-bit)
---------------------------------------

Set DFHACK_BUILD_ARCH to either ``32`` or ``64`` to build a 32-bit or 64-bit
version of DFHack (respectively). The default is currently ``64``, so you will
need to specify this explicitly for 32-bit builds. Specifying it is a good idea
in any case.

::

    cmake .. -DDFHACK_BUILD_ARCH=32

*or*
::

    cmake .. -DDFHACK_BUILD_ARCH=64

Note that the scripts in the "build" folder on Windows will set the architecture
automatically.

Other settings
--------------
There are a variety of other settings which you can find in CMakeCache.txt in
your build folder or by running ``ccmake`` (or another CMake GUI). Most
DFHack-specific settings begin with ``BUILD_`` and control which parts of DFHack
are built.

Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
------------
DFHack is meant to be installed into an existing DF folder, so get one ready.

We assume that any Linux platform will have ``git`` available (though it may
need to be installed with your package manager.)

To build DFHack you need GCC version 4.8 or later. GCC 4.8 is easiest to work
with due to avoiding libstdc++ issues (see below), but any version from 4.8
onwards (including 5.x) will work.

Before you can build anything, you'll also need ``cmake``. It is advisable to
also get ``ccmake`` on distributions that split the cmake package into multiple
parts.

You also need zlib, libsdl (1.2, not sdl2, like DF), perl, and the XML::LibXML
and XML::LibXSLT perl packages (for the code generation parts). You should be
able to find them in your distro repositories.

To build `stonesense`, you'll also need OpenGL headers.

Here are some package install commands for various platforms:

* On Arch linux:

  * For the required Perl modules: ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On Ubuntu::

    apt-get install gcc cmake ninja-build git zlib1g-dev libsdl1.2-dev libxml-libxml-perl libxml-libxslt-perl

* Debian and derived distros should have similar requirements to Ubuntu.


Multilib dependencies
---------------------
If you want to compile 32-bit DFHack on 64-bit distributions, you'll need the
multilib development tools and libraries:

* ``gcc-multilib`` and ``g++-multilib``
* If you have installed a non-default version of GCC - for example, GCC 4.8 on a
  distribution that defaults to 5.x - you may need to add the version number to
  the multilib packages.

  * For example, ``gcc-4.8-multilib`` and ``g++-4.8-multilib`` if installing for GCC 4.8
    on a system that uses a later GCC version.
  * This is definitely required on Ubuntu/Debian, check if using a different distribution.

* ``zlib1g-dev:i386`` (or a similar i386 zlib-dev package)

Note that installing a 32-bit GCC on 64-bit systems (e.g. ``gcc:i386`` on
Debian) will typically *not* work, as it depends on several other 32-bit
libraries that conflict with system libraries. Alternatively, you might be able
to use ``lxc`` to
:forums:`create a virtual 32-bit environment <139553.msg5435310#msg5435310>`.

Build
-----
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

Incompatible libstdc++
~~~~~~~~~~~~~~~~~~~~~~
When compiling dfhack yourself, it builds against your system libstdc++. When
Dwarf Fortress runs, it uses a libstdc++ shipped with the binary, which comes
from GCC 4.8 and is incompatible with code compiled with newer GCC versions. If
you compile DFHack with a GCC version newer than 4.8, you will see an error
message such as::

   ./libs/Dwarf_Fortress: /pathToDF/libs/libstdc++.so.6: version
       `GLIBCXX_3.4.18' not found (required by ./hack/libdfhack.so)

To fix this you can compile with GCC 4.8 or remove the libstdc++ shipped with
DF, which causes DF to use your system libstdc++ instead::

    cd /path/to/DF/
    rm libs/libstdc++.so.6

Note that distributing binaries compiled with newer GCC versions requires end-
users to delete libstdc++ themselves and have a libstdc++ on their system from
the same GCC version or newer. For this reason, distributing anything compiled
with GCC versions newer than 4.8 is discouraged. In the future we may start
bundling a later libstdc++ as part of the DFHack package, so as to enable
compilation-for-distribution with a GCC newer than 4.8.

Mac OS X
========
DFHack functions similarly on OS X and Linux, and the majority of the
information above regarding the build process (cmake and ninja) applies here
as well.

DFHack can officially be built on OS X with GCC 4.8 or 7. Anything newer than 7
will require you to perform extra steps to get DFHack to run (see `osx-new-gcc-notes`),
and your build will likely not be redistributable.

.. _osx-new-gcc-notes:

Notes for GCC 8+ or OS X 10.10+ users
-------------------------------------

If none of these situations apply to you, skip to `osx-setup`.

If you have issues building on OS X 10.10 (Yosemite) or above, try definining
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

.. _osx-setup:

Dependencies and system set-up
------------------------------

#. Download and unpack a copy of the latest DF
#. Install Xcode from the Mac App Store

#. Install the XCode Command Line Tools by running the following command::

    xcode-select --install

#. Install dependencies

    It is recommended to use Homebrew instead of MacPorts, as it is generally
    cleaner, quicker, and smarter. For example, installing MacPort's GCC will
    install more than twice as many dependencies as Homebrew's will, and all in
    both 32-bit and 64-bit variants. Homebrew also doesn't require constant use
    of sudo.

    Using `Homebrew <http://brew.sh/>`_ (recommended)::

        brew tap homebrew/versions
        brew install git
        brew install cmake
        brew install ninja
        brew install gcc@7

    Using `MacPorts <https://www.macports.org>`_::

        sudo port install gcc7 +universal cmake +universal git-core +universal ninja +universal

    Macports will take some time - maybe hours.  At some point it may ask
    you to install a Java environment; let it do so.

#. Install Perl dependencies

  * Using system Perl

    * ``sudo cpan``

      If this is the first time you've run cpan, you will need to go through the setup
      process. Just stick with the defaults for everything and you'll be fine.

      If you are running OS X 10.6 (Snow Leopard) or earlier, good luck!
      You'll need to open a separate Terminal window and run::

        sudo ln -s /usr/include/libxml2/libxml /usr/include/libxml

    * ``install XML::LibXML``
    * ``install XML::LibXSLT``

  * In a separate, local Perl install

    Rather than using system Perl, you might also want to consider
    the Perl manager, `Perlbrew <http://perlbrew.pl>`_.

    This manages Perl 5 locally under ``~/perl5/``, providing an easy
    way to install Perl and run CPAN against it without ``sudo``.
    It can maintain multiple Perl installs and being local has the
    benefit of easy migration and insulation from OS issues and upgrades.

    See http://perlbrew.pl/ for more details.

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

  etc.

* Build dfhack::

    mkdir build-osx
    cd build-osx
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    ninja install  # or ninja -jX install to specify the number of cores (X) to use

  <path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
  version for the DFHack you are building.

.. _compile-windows:

Windows
=======
On Windows, DFHack replaces the SDL library distributed with DF.

Dependencies
------------
You will need the following:

* Microsoft Visual C++ 2015 or 2017
* Git
* CMake
* Perl with XML::LibXML and XML::LibXSLT

  * It is recommended to install StrawberryPerl, which includes both.

* Python (for documentation; optional, except for release builds)

Microsoft Visual Studio 2015
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DFHack has to be compiled with the Microsoft Visual C++ 2015 or 2017 toolchain on Windows;
other versions won't work against Dwarf Fortress due to ABI and STL incompatibilities.

You can install Visual Studio 2015_ or 2017_ Community edition for free, which
include all the features needed by DFHack. You can also download just the
`build tools`_ if you aren't going to use Visual Studio to edit code.

.. _2015: https://visualstudio.microsoft.com/vs/older-downloads/#visual-studio-2015-and-other-products
.. _2017: https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
.. _build tools: https://visualstudio.microsoft.com/vs/older-downloads/#microsoft-build-tools-2015-update-3

Additional dependencies: installing with the Chocolatey Package Manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The remainder of dependencies - Git, CMake, StrawberryPerl, and Python - can be
most easily installed using the Chocolatey Package Manger. Chocolatey is a
\*nix-style package manager for Windows. It's fast, small (8-20MB on disk)
and very capable. Think "``apt-get`` for Windows."

Chocolatey is a recommended way of installing the required dependencies
as it's quicker, requires less effort, and will install known-good utilities
guaranteed to have the correct setup (especially PATH).

To install Chocolatey and the required dependencies:

* Go to https://chocolatey.org in a web browser
* At the top of the page it will give you the install command to copy

  * Copy the first one, which starts ``@powershell ...``
  * It won't be repeated here in case it changes in future Chocolatey releases.

* Open an elevated (Admin) ``cmd.exe`` window

  * On Windows 8 and later this can be easily achieved by:

    * right-clicking on the Start Menu, or pressing Win+X.
    * choosing "Command Prompt (Admin)"

  * On earlier Windows: find ``cmd.exe`` in Start Menu, right click
    and choose Open As Administrator.

* Paste in the Chocolatey install command and hit enter
* Close this ``cmd.exe`` window and open another Admin ``cmd.exe`` in the same way
* Run the following command::

    choco install git cmake.portable strawberryperl -y

* Close the Admin ``cmd.exe`` window; you're done!

You can now use all of these utilities from any normal ``cmd.exe`` window.
You only need Admin/elevated ``cmd.exe`` for running ``choco install`` commands;
for all other purposes, including compiling DFHack, you should use
a normal ``cmd.exe`` (or, better, an improved terminal like `Cmder <http://cmder.net/>`_;
details below, under Build.)

**NOTE**: you can run the above ``choco install`` command even if you already have
Git, CMake or StrawberryPerl installed. Chocolatey will inform you if any software
is already installed and won't re-install it. In that case, please check the PATHs
are correct for that utility as listed in the manual instructions below. Or, better,
manually uninstall the version you have already and re-install via Chocolatey,
which will ensure the PATH are set up right and will allow Chocolatey to manage
that program for you in future.

Additional dependencies: installing manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
If you prefer to install manually rather than using Chocolatey, details and
requirements are as below. If you do install manually, please ensure you
have all PATHs set up correctly.

Git
^^^
Some examples:

* `Git for Windows <https://git-for-windows.github.io>`_ (command-line and GUI)
* `tortoisegit <https://tortoisegit.org>`_ (GUI and File Explorer integration)

CMake
^^^^^
You can get the win32 installer version from
`the official site <http://www.cmake.org/cmake/resources/software.html>`_.
It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

Perl / Strawberry Perl
^^^^^^^^^^^^^^^^^^^^^^
For the code generation parts you'll need Perl 5 with XML::LibXML and XML::LibXSLT.
`Strawberry Perl <http://strawberryperl.com>`_ is recommended as it includes
all of the required packages in a single, easy install.

After install, ensure Perl is in your user's PATH. This can be edited from
``Control Panel -> System -> Advanced System Settings -> Environment Variables``.

The following three directories must be in PATH, in this order:

* ``<path to perl>\c\bin``
* ``<path to perl>\perl\site\bin``
* ``<path to perl>\perl\bin``

Be sure to close and re-open any existing ``cmd.exe`` windows after updating
your PATH.

If you already have a different version of Perl (for example the one from Cygwin),
you can run into some trouble. Either remove the other Perl install from PATH, or
install XML::LibXML and XML::LibXSLT for it using CPAN.

Build
-----
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

Then you can either open the solution with MSVC or use one of the msbuild scripts:

Building/installing from the command line:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In the build directory you will find several ``.bat`` files:

* Scripts with ``build`` prefix will only build DFHack.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

Compiling from the command line is generally the quickest and easiest option.
However be aware that due to the limitations of ``cmd.exe`` - especially in
versions of Windows prior to Windows 10 - it can be very hard to see what happens
during a build.  If you get a failure, you may miss important errors or warnings
due to the tiny window size and extremely limited scrollback. For that reason you
may prefer to compile in the IDE which will always show all build output.

Alternatively (or additionally), consider installing an improved Windows terminal
such as `Cmder <http://cmder.net/>`_. Easily installed through Chocolatey with:
``choco install cmder -y``.

**Note for Cygwin/msysgit users**: It is also possible to compile DFHack from a
Bash command line. This has three potential benefits:

* When you've installed Git and are using its Bash, but haven't added Git to your path:

  * You can load Git's Bash and as long as it can access Perl and CMake, you can
    use it for compile without adding Git to your system path.

* When you've installed Cygwin and its SSH server:

  * You can now SSH in to your Windows install and compile from a remote terminal;
    very useful if your Windows installation is a local VM on a \*nix host OS.

* In general: you can use Bash as your compilation terminal, meaning you have a decent
  sized window, scrollback, etc.

  * Whether you're accessing it locally as with Git's Bash, or remotely through
    Cygwin's SSH server, this is far superior to using ``cmd.exe``.

You don't need to do anything special to compile from Bash. As long as your PATHs
are set up correctly, you can run the same generate- and build/install/package- bat
files as detailed above.

Building/installing from the Visual Studio IDE:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After running the CMake generate script you will have a new folder called VC2015
or VC2015_32, depending on the architecture you specified. Open the file
``dfhack.sln`` inside that folder. If you have multiple versions of Visual
Studio installed, make sure you open with Visual Studio 2015.

The first thing you must then do is change the build type. It defaults to Debug,
but this cannot be used on Windows. Debug is not binary-compatible with DF.
If you try to use a debug build with DF, you'll only get crashes and for this
reason the Windows "debug" scripts actually do RelWithDebInfo builds.
After loading the Solution, change the Build Type to either ``Release``
or ``RelWithDebInfo``.

Then build the ``INSTALL`` target listed under ``CMakePredefinedTargets``.


Building the documentation
==========================

DFHack documentation, like the file you are reading now, is created as .rst files,
which are in `reStructuredText (reST) <http://sphinx-doc.org/rest.html>`_ format.
This is a documenation format that has come from the Python community. It is very
similar in concept - and in syntax - to Markdown, as found on GitHub and many other
places. However it is more advanced than Markdown, with more features available when
compiled to HTML, such as automatic tables of contents, cross-linking, special
external links (forum, wiki, etc) and more. The documentation is compiled by a
Python tool, `Sphinx <http://sphinx-doc.org>`_.

The DFHack build process will compile the documentation but this has been disabled
by default. You only need to build the docs if you're changing them, or perhaps
if you want a local HTML copy; otherwise, read them easily online at
`ReadTheDoc's DFHack hosted documentation <https://dfhack.readthedocs.org>`_.

(Note that even if you do want a local copy, it is certainly not necesesary to
compile the documentation in order to read it. Like Markdown, reST documents are
designed to be just as readable in a plain-text editor as they are in HTML format.
The main thing you lose in plain text format is hyperlinking.)


Enabling documentation building
-------------------------------
First, make sure you have followed all the necessary steps for your platform as
outlined in the rest of this document.

To compile documentation with DFHack, add the following flag to your ``cmake`` command::

  -DBUILD_DOCS:bool=ON

For example::

  cmake .. -DCMAKE_BUILD_TYPE:string=Release -DBUILD_DOCS:bool=ON -DCMAKE_INSTALL_PREFIX=<path to DF>

Alternatively you can use the CMake GUI which allows options to be changed easily.

On Windows you should either use ``generate-msvc-gui.bat`` and set the option
through the GUI, or else if you want to use an alternate file, such as
``generate-msvc-all.bat``, you will need to edit it to add the flag.
Or you could just run ``cmake`` on the command line like in other platforms.

Required dependencies
---------------------
In order to build the documentation, you must have Python with Sphinx
version 1.3.1 or later. Both Python 2.x and 3.x are supported.

When installing Sphinx from OS package managers, be aware that there is
another program called Sphinx, completely unrelated to documentation management.
Be sure you are installing the right Sphinx; it may be called ``python-sphinx``,
for example. To avoid doubt, ``pip`` can be used instead as detailed below.


Linux
-----
Most Linux distributions will include Python as standard.

Check your package manager to see if Sphinx 1.3.1 or later is available,
but at the time of writing Ubuntu for example only has 1.2.x.

You can instead install Sphinx with the pip package manager. This may need
to be installed from your OS package manager; this is the case on Ubuntu.
On Ubuntu/Debian, use the following to first install pip::

  sudo apt-get install python-pip

Once pip is available, you can then install the Python Sphinx module with::

  pip install sphinx

If you run this as a normal user it will install a local copy for your user only.
Run it with sudo if you want a system-wide install. Either is fine for DFHack,
however if installing locally do check that ``sphinx-build`` is in your path.
It may be installed in a directory such as ``~/.local/bin/``, so after pip
install, find ``sphinx-build`` and ensure its directory is in your local ``$PATH``.


Mac OS X
--------
OS X has Python 2.7 installed by default, but it does not have the pip package manager.

You can install Homebrew's Python 3, which includes pip, and then install the
latest Sphinx using pip::

  brew install python3
  pip3 install sphinx

Alternatively, you can simply install Sphinx 1.3.x directly from Homebrew::

  brew install sphinx-doc

This will install Sphinx for OS X's system Python 2.7, without needing pip.

Either method works; if you plan to use Python for other purposes, it might best
to install Homebrew's Python 3 so that you have the latest Python as well as pip.
If not, just installing sphinx-doc for OS X's system Python 2.7 is fine.


Windows
-------
Use the Chocolatey package manager to install Python and pip,
then use pip to install Sphinx.

Run the following commands from an elevated (Admin) ``cmd.exe``, after installing
Chocolatey as outlined in the `Windows section <compile-windows>`::

  choco install python pip -y

Then close that Admin ``cmd.exe``, re-open another Admin ``cmd.exe``, and run::

  pip install sphinx

.. _build-changelog:

Building the changelogs
-----------------------
If you have Python installed, but do not want to build all of the documentation,
you can build the changelogs with the ``docs/gen_changelog.py`` script.

All changes should be listed in ``changelog.txt``. A description of this file's
format follows:

.. include:: /docs/changelog.txt
   :start-after: ===help
   :end-before: ===end

Misc. Notes
===========

.. _note-offline-builds:

Note on building DFHack offline
-------------------------------

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
a command starting with ``cmake .. -G Ninja`` on Linux and OS X, following the
instructions in the sections above. CMake should automatically locate files that
you placed in ``CMake/downloads``, and use them instead of attempting to
download them.

.. _note-old-git-and-dfhack:

Note on using very old git versions with pre-0.43.03 DFHack versions
--------------------------------------------------------------------

If you are using git 1.8.0 or older, and cloned DFHack before commit 85a920d
(around DFHack v0.43.03-alpha1), you may run into fatal git errors when updating
submodules after switching branches. This is due to those versions of git being
unable to handle our change from "scripts/3rdparty/name" submodules to a single
"scripts" submodule. This may be fixable by renaming .git/modules/scripts to
something else and re-running ``git submodule update --init`` on the branch with
the single scripts submodule (and running it again when switching back to the
one with multiple submodules, if necessary), but it is usually much simpler to
upgrade your git version.
