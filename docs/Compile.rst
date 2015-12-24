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

**Important note regarding submodule update and changing branches**:

You must run ``git submodule update`` every time you change Git branch,
for example when switching between master and develop branches and back.


Contributing to DFHack
======================
If you want to get involved with the development, create an account on
GitHub, make a clone there and then use that as your remote repository instead.

We'd love that; join us on IRC (#dfhack channel on freenode) for discussion,
and whenever you need help.

For lots more details on contributing to DFHack, including pull requests, code format,
and more, please see `contributing-code`.


Build types
===========
``cmake`` allows you to pick a build type by changing the ``CMAKE_BUILD_TYPE`` variable::

    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid and useful build types include 'Release', 'Debug' and
'RelWithDebInfo'.
'Debug' is not available on Windows, use 'RelWithDebInfo' instead.


Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
------------
DFHack is meant to be installed into an existing DF folder, so get one ready.

We assume that any Linux platform will have ``git`` available (though it may
require installing from your package manager.)

To build DFHack you need GCC version 4.5 or later, capable of compiling for 32-bit
(i386) targets. GCC 4.5 is easiest to work with due to avoiding libstdc++ issues
(see below), but any version from 4.5 onwards (including 5.x) will work.

On 64-bit distributions, you'll need the multilib development tools and libraries:

* ``gcc-multilib`` and ``g++-multilib``
* If you have installed a non-default version of GCC - for example, GCC 4.5 on a
  distribution that defaults to 5.x - you may need to add the version number to
  the multilib packages.

  * For example, ``gcc-4.5-multilib`` and ``g++-4.5-multilib`` if installing for GCC 4.5
    on a system that uses a later GCC version.
  * This is definitely required on Ubuntu/Debian, check if using a different distribution.

Note that installing a 32-bit GCC on 64-bit systems (e.g. ``gcc:i386`` on Debian) will
typically *not* work, as it depends on several other 32-bit libraries that
conflict with system libraries. Alternatively, you might be able to use ``lxc``
to
:forums:`create a virtual 32-bit environment <139553.msg5435310#msg5435310>`.

Before you can build anything, you'll also need ``cmake``. It is advisable to also get
``ccmake`` on distributions that split the cmake package into multiple parts.

You also need perl and the XML::LibXML and XML::LibXSLT perl packages (for the code generation parts).
You should be able to find them in your distro repositories.

To build Stonesense, you'll also need OpenGL headers.

Here are some package install commands for various platforms:

* On Arch linux:

  * For the required Perl modules: ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On 64-bit Ubuntu:

  * ``apt-get install gcc cmake git gcc-multilib g++-multilib zlib1g-dev:i386 libxml-libxml-perl libxml-libxslt-perl``.

* On 32-bit Ubuntu:

  * ``apt-get install gcc cmake git gcc-multilib g++-multilib zlib1g-dev libxml-libxml-perl libxml-libxslt-perl``.

* Debian-derived distros should have similar requirements.


Build
-----
Building is fairly straightforward. Enter the ``build`` folder (or create an
empty folder in the DFHack directory to use instead) and start the build like this::

    cd build
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    make install # or make -jX install on multi-core systems to compile with X parallel processes

<path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
version for the DFHack you are building. This will build the library along
with the normal set of plugins and install them into your DF folder.

Alternatively, you can use ccmake instead of cmake::

    cd build
    ccmake ..
    make install

This will show a curses-based interface that lets you set all of the
extra options. You can also use a cmake-friendly IDE like KDevelop 4
or the cmake-gui program.

Incompatible libstdc++
~~~~~~~~~~~~~~~~~~~~~~
When compiling dfhack yourself, it builds against your system libstdc++.
When Dwarf Fortress runs, it uses a libstdc++ shipped with the binary, which
comes from GCC 4.5 and is incompatible with code compiled with newer GCC versions.
This manifests itself with an error message such as::

   ./libs/Dwarf_Fortress: /pathToDF/libs/libstdc++.so.6: version
       `GLIBCXX_3.4.15' not found (required by ./hack/libdfhack.so)

To fix this you can compile with GCC 4.5 or remove the libstdc++ shipped with
DF, which causes DF to use your system libstdc++ instead::

    cd /path/to/DF/
    rm libs/libstdc++.so.6

Note that distributing binaries compiled with newer GCC versions requires end-
users to delete libstdc++ themselves and have a libstdc++ on their system from
the same GCC version or newer. For this reason, distributing anything compiled
with GCC versions newer than 4.5 is discouraged. In the future we may start
bundling a later libstdc++ as part of the DFHack package, so as to enable
compilation-for-distribution with a GCC newer than 4.5.

Mac OS X
========
DFHack functions similarly on OS X and Linux, and the majority of the
information above regarding the build process (cmake and make) applies here
as well.

If you have issues building on OS X 10.10 (Yosemite) or above, try definining the
following environment variable::

    export MACOSX_DEPLOYMENT_TARGET=10.9

Note for El Capitan (OSX 10.11) and XCode 7.x users
---------------------------------------------------

* You will probably find when following the instructions below that GCC 4.5 will
  fail to install on OSX 10.11, or any older OSX that is using XCode 7.
* There are two workarounds:

  * Install GCC 5.x instead (``brew install gcc5``), and then after compile
    replace ``hack/libstdc++.6.dylib`` with a symlink to GCC 5's i386
    version of this file::

      cd <path to df>/hack && mv libstdc++.6.dylib libstdc++.6.dylib.orig &&
      ln -s /usr/local/Cellar/gcc5/5.2.0/lib/gcc/5/i386/libstdc++.6.dylib .

  * Install XCode 6, which is available as a free download from the Apple
    Developer Center.

    * Either install this as your only XCode, or install it additionally
      to XCode 7 and then switch between them using ``xcode-select``
    * Ensure XCode 6 is active before attempting to install GCC 4.5 and
      whenever you are compiling DFHack with GCC 4.5.

Dependencies and system set-up
------------------------------

#. Download and unpack a copy of the latest DF
#. Install Xcode from Mac App Store

#. Install the XCode Command Line Tools by running the following command::

    xcode-select --install

#. Install dependencies

    Using `Homebrew <http://brew.sh/>`_ (recommended)::

        brew tap homebrew/versions
        brew install git
        brew install cmake
        brew install gcc45

    Using `MacPorts <https://www.macports.org>`_::

        sudo port install gcc45 +universal cmake +universal git-core +universal

    Macports will take some time - maybe hours.  At some point it may ask
    you to install a Java environment; let it do so.

    It is recommended to use Homebrew instead of MacPorts, as it is generally
    cleaner, quicker, and smarter. For example, installing
    MacPort's GCC 4.5 will install more than twice as many dependencies
    as Homebrew's will, and all in both 32bit and 64bit variants.
    Homebrew also doesn't require constant use of sudo.

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

    export CC=/usr/local/bin/gcc-4.5
    export CXX=/usr/local/bin/g++-4.5

  Macports::

    export CC=/opt/local/bin/gcc-mp-4.5
    export CXX=/opt/local/bin/g++-mp-4.5

  Change the version numbers appropriately if you installed a different version of GCC.

* Build dfhack::

    mkdir build-osx
    cd build-osx
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    make install # or make -j X install on multi-core systems to compile with X parallel processes

  <path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
  version for the DFHack you are building.

.. _compile-windows:

Windows
=======
On Windows, DFHack replaces the SDL library distributed with DF.

Dependencies
------------
You will need the following:

* Microsoft Visual Studio 2010 SP1, with the C++ language
* Git
* CMake
* Perl with XML::LibXML and XML::LibXSLT

  * It is recommended to install StrawberryPerl, which includes both.

Microsoft Visual Studio 2010 SP1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DFHack has to be compiled with the Microsoft Visual C++ 2010 SP1 toolchain; later
versions won't work against Dwarf Fortress due to ABI and STL incompatibilities.

At present, the only way to obtain the MSVC C++ 2010 toolchain is to install a
full copy of Microsoft Visual Studio 2010 SP1. The free Express version is sufficient.

You can grab it from `Microsoft's site <http://download.microsoft.com/download/1/E/5/1E5F1C0A-0D5B-426A-A603-1798B951DDAE/VS2010Express1.iso>`_.

You should also install the Visual Studio 2010 SP1 update.

You can confirm whether you have SP1 by opening the Visual Studio 2010 IDE
and selecting About from the Help menu.  If you have SP1 it will have *SP1Rel*
at the end of the version number, for example: *Version 10.0.40219.1 SP1Rel*

Use of pre-SP1 releases has been reported to cause issues and is therefore not
supported by DFHack. Please ensure you are using SP1 before raising any Issues.

If your Windows Update is configured to receive updates for all Microsoft
Products, not just Windows, you will receive the SP1 update automatically
through Windows Update (you will probably need to trigger a manual check.)

If not, you can download it directly `from this Microsoft Download link <https://www.microsoft.com/en-gb/download/details.aspx?id=23691>`_.

Additional dependencies: installing with the Chocolatey Package Manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The remainder of dependencies - Git, CMake and StrawberryPerl - can be most
easily installed using the Chocolatey Package Manger. Chocolatey is a
\*nix-style package manager for Windows. It's fast, small (8-20MB on disk)
and very capable. Think "``apt-get`` for Windows."

Chocolatey is a preferred way of installing the required dependencies
as it's quicker, less effort and will install known-good utilities
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

    choco install git cmake strawberryperl -y

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
There are several different batch files in the ``build`` folder along
with a script that's used for picking the DF path.

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
After running the CMake generate script you will have a new folder called VC2010.
Open the file ``dfhack.sln`` inside that folder. If you have multiple versions of
Visual Studio installed, make sure you open with Visual Studio 2010.

The first thing you must then do is change the build type. It defaults to Debug,
but this cannot be used on Windows. Debug is not binary-compatible with DF.
If you try to use a debug build with DF, you'll only get crashes and for this
reason the Windows "debug" scripts actually do RelWithDebInfo builds.
After loading the Solution, change the Build Type to either ``Release``
or ``RelWithDebInfo``.

Then build the ``INSTALL`` target listed under ``CMakePredefinedTargets``.


##########################
Building the documentation
##########################

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
===============================
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
=====================
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
