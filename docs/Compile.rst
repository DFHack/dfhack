.. highlight:: shell

.. _compile:

################
Compiling DFHack
################

DFHack builds are available for all supported platforms; see `installing` for
installation instructions. If you are a DFHack end-user, modder, or plan on
writing scripts (not plugins), it is generally recommended (and easier) to use
these builds instead of compiling DFHack from source.

However, if you are looking to develop plugins, work on the DFHack core, make
complex changes to DF-structures, or anything else that requires compiling
DFHack from source, this document will walk you through the build process. Note
that some steps may be unconventional compared to other projects, so be sure to
pay close attention if this is your first time compiling DFHack.

.. contents:: Contents
  :local:
  :depth: 1

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


Contributing to DFHack
----------------------

For details on contributing to DFHack, including pull requests, code
format, and more, please see `contributing-code`.


Build settings
==============

This section describes build configuration options that apply to all platforms.
If you don't have a working build environment set up yet, follow the instructions
in the platform-specific sections below first, then come back here.

Generator
---------

The ``Ninja`` CMake build generator is the preferred build method on Linux and
macOS, instead of ``Unix Makefiles``, which is the default. You can select Ninja
by passing ``-G Ninja`` to CMake. Incremental builds using Unix Makefiles can be
much slower than Ninja builds. Note that you will probably need to install
Ninja; see the platform-specific sections for details.

::

    cmake .. -G Ninja

.. warning::

  Most other CMake settings can be changed by running ``cmake`` again, but the
  generator cannot be changed after ``cmake`` has been run without creating a
  new build folder. Do not forget to specify this option.

  CMake versions 3.6 and older, and possibly as recent as 3.9, are known to
  produce project files with dependency cycles that fail to build
  (see :issue:`1369`). Obtaining a recent version of CMake is recommended, either from
  `cmake.org <https://cmake.org/download/>`_ or through a package manager. See
  the sections below for more platform-specific directions for installing CMake.

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

.. _compile-build-options:

Other settings
--------------
There are a variety of other settings which you can find in CMakeCache.txt in
your build folder or by running ``ccmake`` (or another CMake GUI). Most
DFHack-specific settings begin with ``BUILD_`` and control which parts of DFHack
are built.


.. _compile-linux:

Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
------------
DFHack is meant to be installed into an existing DF folder, so get one ready.

We assume that any Linux platform will have ``git`` available (though it may
need to be installed with your package manager.)

To build DFHack, you need GCC 4.8 or newer. GCC 4.8 has the benefit of avoiding
`libstdc++ compatibility issues <linux-incompatible-libstdcxx>`, but can be hard
to obtain on modern distributions, and working around these issues is done
automatically by the ``dfhack`` launcher script. As long as your system-provided
GCC is new enough, it should work. Note that extremely new GCC versions may not
have been used to build DFHack yet, so if you run into issues with these, please
let us know (e.g. by opening a GitHub issue).

Before you can build anything, you'll also need ``cmake``. It is advisable to
also get ``ccmake`` on distributions that split the cmake package into multiple
parts. As mentioned above, ``ninja`` is recommended (many distributions call
this package ``ninja-build``).

You will need pthread; most systems should have this already. Note that older
CMake versions may have trouble detecting pthread, so if you run into
pthread-related errors and pthread is installed, you may need to upgrade CMake,
either by downloading it from `cmake.org <https://cmake.org/download/>`_ or
through your package manager, if possible.

You also need zlib, libsdl (1.2, not sdl2, like DF), perl, and the XML::LibXML
and XML::LibXSLT perl packages (for the code generation parts). You should be
able to find them in your distribution's repositories.

To build `stonesense`, you'll also need OpenGL headers.

Here are some package install commands for various distributions:

* On Arch linux:

  * For the required Perl modules: ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On Ubuntu::

    apt-get install gcc cmake ninja-build git zlib1g-dev libsdl1.2-dev libxml-libxml-perl libxml-libxslt-perl

  * Other Debian-based distributions should have similar requirements.

* On Fedora::

    yum install gcc-c++ cmake ninja-build git zlib-devel SDL-devel perl-core perl-XML-LibXML perl-XML-LibXSLT ruby


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

.. _linux-incompatible-libstdcxx:

Incompatible libstdc++
~~~~~~~~~~~~~~~~~~~~~~
When compiling DFHack yourself, it builds against your system libstdc++. When
Dwarf Fortress runs, it uses a libstdc++ shipped in the ``libs`` folder, which
comes from GCC 4.8 and is incompatible with code compiled with newer GCC
versions. As of DFHack 0.42.05-alpha1, the ``dfhack`` launcher script attempts
to fix this by automatically removing the DF-provided libstdc++ on startup.
In rare cases, this may fail and cause errors such as:

.. code-block:: text

   ./libs/Dwarf_Fortress: /pathToDF/libs/libstdc++.so.6: version
       `GLIBCXX_3.4.18' not found (required by ./hack/libdfhack.so)

The easiest way to fix this is generally removing the libstdc++ shipped with
DF, which causes DF to use your system libstdc++ instead::

    cd /path/to/DF/
    rm libs/libstdc++.so.6

Note that distributing binaries compiled with newer GCC versions may result in
the opposite compatibily issue: users with *older* GCC versions may encounter
similar errors. This is why DFHack distributes both GCC 4.8 and GCC 7 builds. If
you are planning on distributing binaries to other users, we recommend using an
older GCC (but still at least 4.8) version if possible.


.. _compile-macos:

macOS
=====
DFHack functions similarly on macOS and Linux, and the majority of the
information above regarding the build process (CMake and Ninja) applies here
as well.

DFHack can officially be built on macOS only with GCC 4.8 or 7. Anything newer than 7
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
    of ``sudo``.

    Using `Homebrew <https://brew.sh/>`_ (recommended)::

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
    the Perl manager, `Perlbrew <https://perlbrew.pl>`_.

    This manages Perl 5 locally under ``~/perl5/``, providing an easy
    way to install Perl and run CPAN against it without ``sudo``.
    It can maintain multiple Perl installs and being local has the
    benefit of easy migration and insulation from OS issues and upgrades.

    See https://perlbrew.pl/ for more details.

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


.. _compile-windows:

Windows
=======
.. contents:: Section Contents
  :local:
  :depth: 3

In order to build on windows you'll need to resolve a few dependencies. The dfhack build system uses perl to generate
source files using xml files detailing various DF structures. Cmake is used to generate build files, or project files
for visual studio. Read the sections below for more detail.

Note: On Windows, DFHack replaces the SDL library distributed with DF.

Dependencies
------------
These are the general dependencies you will need:

* Microsoft Visual C++ 2022, 2019, 2017, or 2015 (optional)
* Microsoft Visual C++ 2015 Build Tools
* Git
* CMake
* StrawberryPerl

  * Perl
  * XML:LibXML
  * XML:LibXLST
* Python (for documentation; optional, except for release builds)

Installing Dependencies
~~~~~~~~~~~~~~~~~~~~~~~
.. contents:: Section Contents
  :local:
  :depth: 1

Chocolatey (`fast install instructions`_) is recommended for installing Git, Cmake, Perl, and Python if they
are not already installed. Put simply, chocolatey is a package manager for windows so you can install/uninstall
software from the command line.

.. _fast install instructions: https://chocolatey.org/install

Choco install commands::

   choco install git
   choco install cmake
   choco install strawberryperl
   choco install visualstudio2015community

Note: strictly speaking, you do not need visual studio 2015, or any other visual studio release, but visual studio
is recommended by these instructions. More advanced options are available to those who seek them.

Alternative to chocolatey, you're also more than free to download and install manually.

Manually [Fun!]
^^^^^^^^^^^^^^^
If you prefer to install manually rather than using Chocolatey, details and
requirements are as below. If you do install manually, please ensure you
have all PATHs set up correctly.

Git
...
Some examples:

* `Git for Windows <https://git-for-windows.github.io>`_ (command-line and GUI)
* `tortoisegit <https://tortoisegit.org>`_ (GUI and File Explorer integration)

CMake
.....
You can get the win32 installer version from
`the official site <https://cmake.org/download/>`_.
It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

Perl / Strawberry Perl
......................
For the code generation stage of the build process, you'll need Perl 5 with
XML::LibXML and XML::LibXSLT. `Strawberry Perl <http://strawberryperl.com>`_ is
recommended as it includes all of the required packages in a single, easy
install.

After install, ensure Perl is in your user's PATH. This can be edited from
``Control Panel -> System -> Advanced System Settings -> Environment Variables``.

The following directories must be in your PATH, in this order:

* ``<path to perl>\c\bin``
* ``<path to perl>\perl\site\bin``
* ``<path to perl>\perl\bin``
* ``<path to perl>\perl\vendor\lib\auto\XML\LibXML`` (may only be required on some systems)

Be sure to close and re-open any existing ``cmd.exe`` windows after updating
your PATH.

If you already have a different version of Perl installed (for example, from Cygwin),
you can run into some trouble. Either remove the other Perl install from PATH, or
install XML::LibXML and XML::LibXSLT for it using CPAN.

Environment 1: Visual Studio
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
It is generally recommended to install any relatively modern version of Visual Studio, so 2019_ or 2022_
Click Visual Studio 2022_ or 2019_ to download an installer wizard that will prompt you
to select the optional tools you want to download alongside the IDE. You may need to log into
(or create) a Microsoft account in order to download Visual Studio.

or, with choco::

    choco install visualstudio2022community

1. Open **Visual Studio Installer**.
2. Select modify, for whichever version you've chosen to utilize.
3. Check the boxes for the following components:

* "Desktop Development with C++"
* "C++ Windows XP Support for VS 2017 (v141) tools [Deprecated]"
* "MSVC v140 - VS 2015 C++ build tools (v14.00)"

Yes, this is unintuitive. Installing XP Support for VS 2017 installs XP Support for VS 2015
if the 2015 toolchain is installed.

.. _2022: https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false
.. _2019: https://my.visualstudio.com/Downloads?q=visual%20studio%202019&wt.mc_id=o~msft~vscom~older-downloads

Environment 2: Command Line
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. contents:: Section Contents
  :local:
  :depth: 2

Click `build tools`_ and you will be prompted to login to your Microsoft account.
Then you should be redirected to a page with various download options with 2015
in their name. If this redirect doesn't occur, just copy, paste, and enter the
download link again and you should see the options. You need to get:
Visual C++ Build Tools for Visual Studio 2015 with Update 3.
Click the download button next to it and a dropdown of download formats will appear.
Select the DVD format to download an ISO file. When the donwload is complete,
click on the ISO file and a folder will popup with the following contents:

* packages (folder)
* VCPlusPlusBuildTools2015Update3_x64_Files.cat
* VisualCppBuildTools_Full.exe

The packages folder contains the dependencies that are required by the build tools.
These include:

* Microsoft .NET Framework 4.6.1 Developer Pack
* Microsoft Visual C++ 2015 Redistributable (x64) - 14.0.24210
* Windows 10 Universal SDK - 10.0.10240
* Windows 8.1 SDK

Click VisualCppBuildTools_Full.exe and use the default options provided by the installer
wizard that appears. After the installation is completed, add the path where MSBuild.exe
was installed to your PATH environment variable. The path should be:

* ``C:\Program Files (x86)\MSBuild\14.0\Bin``

Note that this process may install only the ``v140`` toolchain, not the ``v140_xp`` toolchain that
is normally used to compile build releases of DFHack. Due to a bug in the Microsoft-provided libraries used with
the ``v140_xp`` toolchain that Microsoft has never fixed, DFHack (and probably also Dwarf Fortress itself)
doesn't run reliably on 64-bit XP. Investigations have so far suggested that ``v140`` and
``v140_xp`` are ABI-compatible. As such, there should be no harm in using ``v140`` instead of
``v140_xp`` as the build toolchain, at least on 64-bit platforms. However, it is our policy to use
``v140_xp`` for release builds for both 32-bit and 64-bit Windows,
since 32-bit releases of Dwarf Fortress work on XP and ``v140_xp`` is required for compatibility with
XP.

The ``v141`` toolchain, in Visual Studio 2017, has been empirically documented to be incompatible with
released versions of Dwarf Fortress and cannot be used to make usable builds of DFHack.

.. _build tools: https://my.visualstudio.com/Downloads?q=visual%20studio%202015&wt.mc_id=o~msft~vscom~older-downloads

Powershell
..........
If PATH in windows is configured correctly, then powershell [or cmd] should just work
[once the dependencies are installed].

.. _git bash:

Git Bash
........
Git bash can be packaged with its own version of perl which will supersede the user installed version
(StrawberryPerl) already in Windows' PATH variable. So steps should be taken to ensure the necessary
dependencies are found/used.

Option 1: configure PATH
::::::::::::::::::::::::
Git bash will modify PATH so that its perl is found under /usr/bin. Using ``.bash_profile`` or ``.bashrc``
file(s) to manually configure PATH will easily solve this issue.

Option 2: perl dependencies
:::::::::::::::::::::::::::
Perl is responsible for finding the LibXML and LibXLST dependencies, so installing them for Git bash's perl to
find will ensure everything is resolved correctly.

Cygwin
......
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
are set up correctly for Bash, you can run the same generate- and build/install/package- bat
files as detailed above.

WSL
...
For the modern windows developer this is likely the preferred build environment.
Unfortunately no configuration instructions are currently available. Please submit
an issue or pull request if you have advice to help users get WSL up and running.


Build
-----
.. contents:: Section Contents
  :local:
  :depth: 1

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

From the command line:
~~~~~~~~~~~~~~~~~~~~~~
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



From the Visual Studio IDE:
~~~~~~~~~~~~~~~~~~~~~~~~~~~
After running the CMake generate script you will have a new folder called VC2015
or VC2015_32, depending on the architecture you specified.

Now, open the file ``dfhack.sln`` inside that folder.
Note: You will likely be prompted to upgrade, select ``No Upgrade`` because we require this specific version
of build tools as can be seen specified in the cmake command (ie. ``-T v140_xp``).

Next, with Visual Studio open, check the build configuration drop downs. Debug does not work, so you must
select ``RelWithDebInfo`` if you wish for debug information to be generated. Otherwise you can select
``Release`` if you wish.

Now you're ready to build. You'll find the Cmake defined targed under: ``CMakePredefinedTargets``
You'll likely want to select ``INSTALL`` or ``ALL_BUILD`` from this folder.

.. note::

    If you're having trouble building, remember Visual Studio sometimes needs you
    to rebuild or clean targets. So give that a try if you're seeing build errors.
    Otherwise check the `FAQ`_ or `discord`_

With the documentation
~~~~~~~~~~~~~~~~~~~~~~

To generate documentation, you'll need to enable it when you generate your build/project files.

* When initially running CMake, add ``-DBUILD_DOCS:bool=ON`` to your ``cmake``
  command. For example::

    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DBUILD_DOCS:bool=ON -DCMAKE_INSTALL_PREFIX=<path to DF>

* If you have already run CMake, you can simply run it again from your build
  folder to update your configuration::

    cmake .. -DBUILD_DOCS:bool=ON

* You can edit the ``BUILD_DOCS`` setting in CMakeCache.txt directly

* You can use the CMake GUI or ``ccmake`` to change the ``BUILD_DOCS`` setting

* On Windows, if you prefer to use the batch scripts, you can run
  ``generate-msvc-gui.bat`` and set ``BUILD_DOCS`` through the GUI. If you are
  running another file, such as ``generate-msvc-all.bat``, you will need to edit
  it to add the flag. You can also run ``cmake`` on the command line, similar to
  other platforms.

The generated documentation will be stored in ``docs/html`` in the root DFHack
folder, and will be installed to ``hack/docs`` when you next install DFHack in a
DF folder.

FAQ
---

**What do I do when I'm getting the error** ``fatal error C1083: Cannot open include file: 'CoreProtocol.pb.h': No such file or directory``
**or** ``..\zlib\lib\zlib.lib : warning LNK4272: library machine type 'X86' conflicts with target machine type 'x64'``?

* Check the architecture CMake has configured: ``cat CMakeCache.txt | grep ARCH`` if it is 32, and you're building 64 [or vice versa] you'll need to regenerate the project from scratch.

* Modify the cmake command to include ``-DDFHACK_BUILD_ARCH=64`` or 32 if that's the appropriate value for what you're building. If you didn't use cmake, check the batch file for the cmake command.


**What should I do if running one of the generate-MSVC- batch files I get an error** ``Failed to run MSBuild command:``?

Modify the batch file, it may be that you haven't installed Visual Studio or that you haven't installed the version used by default as the generator inside the batch files. Simply remove the -G generator option from the cmake command and try again. Remember to delete VC2015/ before running the modified batch file.


**What does it mean if building or running cmake I see the error** ``Can't locate XML/LibXML.pm in @INC (you may need to
install the XML::LibXML module)``?

* It means perl can't find that dependency.
* You'll first want to check if cmake finds perl at all

  * then that the path of perl matches what you installed, or if it is perhaps the git-bash packaged perl
  * cmake output ex: ``-- Found Perl: C:/cmder/vendor/git-for-windows/usr/bin/perl.exe (found version "5.34.0")``
* If it isn't the version you expect to see, read the section about `git bash`.
* If it is, ensure the LibXML and LibXLST dependencies are installed for the detected perl installation.
* If they are, seek help on `discord`_.

.. _discord: https://dfhack.org/discord


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
a command starting with ``cmake .. -G Ninja`` on Linux and macOS, following the
instructions in the sections above. CMake should automatically locate files that
you placed in ``CMake/downloads``, and use them instead of attempting to
download them.
