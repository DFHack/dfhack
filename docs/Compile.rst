################
Compiling DFHack
################

You don't need to compile DFHack unless you're developing plugins or working on the core.

For users, modders, and authors of scripts it's better to download
and `install the latest release instead <installing>`.

.. contents::
   :depth: 2


How to get the code
===================
DFHack doesn't have any kind of system of code snapshots in place, so you will have to
get code from the github repository using git.  How to get git is described under
the instructions for each platform.

To get the code::

    git clone --recursive https://github.com/DFHack/dfhack
    cd dfhack

If your version of git does not support the ``--recursive`` flag, you will need to omit it and run
``git submodule update --init`` after entering the dfhack directory.

If you want to get involved with the development, create an account on
Github, make a clone there and then use that as your remote repository instead.
We'd love that; join us on IRC (#dfhack channel on freenode) if you need help.


Build types
===========
``cmake`` allows you to pick a build type by changing the ``CMAKE_BUILD_TYPE`` variable::

    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid and useful build types include 'Release', 'Debug' and
'RelWithDebInfo'. 'Debug' is not available on Windows.


Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
------------
DFHack is meant to be installed into an existing DF folder, so get one ready.

We assume that any Linux platform will have ``git`` available.

To build DFHack you need a version of GCC 4.x capable of compiling for 32-bit
(i386) targets. GCC 4.5 is easiest to work with due to avoiding libstdc++ issues
(see below), but any later 4.x version should work as well. GCC 5.x will not
work due to ABI changes (the entire plugin loading system won't work, for
example). On 64-bit distributions, you'll need the multilib development tools
and libraries (``gcc-multilib`` or ``gcc-4.x-multilib`` on Debian). Note that
installing a 32-bit GCC on 64-bit systems (e.g. ``gcc:i386`` on Debian) will
typically *not* work, as it depends on several other 32-bit libraries that
conflict with system libraries. Alternatively, you might be able to use ``lxc``
to
:forums:`create a virtual 32-bit environment <139553.msg5435310#msg5435310>`.

Before you can build anything, you'll also need ``cmake``. It is advisable to also get
``ccmake`` on distributions that split the cmake package into multiple parts.

You also need perl and the XML::LibXML and XML::LibXSLT perl packages (for the code generation parts).
You should be able to find them in your distro repositories.

* On Arch linux, ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)
* On 64-bit Ubuntu, ``apt-get install zlib1g-dev:i386 libxml-libxml-perl libxml-libxslt-perl``.
* On 32-bit Ubuntu, ``apt-get install gcc-multilib g++-multilib zlib1g-dev libxml-libxml-perl libxml-libxslt-perl``.
* Debian-derived distros should have similar requirements.

To build Stonesense, you'll also need OpenGL headers.


Build
-----
Building is fairly straightforward. Enter the ``build`` folder (or create an
empty folder in the DFHack directory to use instead) and start the build like this::

    cd build
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=/home/user/DF
    make install

Obviously, replace the install path with path to your DF. This will build the library
along with the normal set of plugins and install them into your DF folder.

Alternatively, you can use ccmake instead of cmake::

    cd build
    ccmake ..
    make install

This will show a curses-based interface that lets you set all of the
extra options.  You can also use a cmake-friendly IDE like KDevelop 4
or the cmake-gui program.


Incompatible libstdc++
~~~~~~~~~~~~~~~~~~~~~~
When compiling dfhack yourself, it builds against your system libstdc++.
When Dwarf Fortress runs, it uses a libstdc++ shipped with the binary, which
comes from GCC 4.5 and is incompatible with code compiled with newer GCC versions.
This manifests itself with the error message::

   ./libs/Dwarf_Fortress: /pathToDF/libs/libstdc++.so.6: version
       `GLIBCXX_3.4.15' not found (required by ./hack/libdfhack.so)

To fix this, you can compile with GCC 4.5 or remove the libstdc++ shipped with
DF, which causes DF to use your system libstdc++ instead::

    cd /path/to/DF/
    rm libs/libstdc++.so.6

Note that distributing binaries compiled with newer GCC versions requires end-
users to delete libstdc++ themselves and have a libstdc++ on their system from
the same GCC version or newer. For this reason, distributing anything compiled
with GCC versions newer than 4.8 is discouraged.

Mac OS X
========
DFHack functions similarly on OS X and Linux, and the majority of the
information above regarding the build process (cmake and make) applies here
as well.

If you have issues building on OS X 10.10 (Yosemite) or above, try definining the
following environment variable::

    export MACOSX_DEPLOYMENT_TARGET=10.9

#. Download and unpack a copy of the latest DF
#. Install Xcode from Mac App Store
#. Open Xcode, go to Preferences > Downloads, and install the Command Line Tools.
#. Install dependencies

    Using `Homebrew <http://brew.sh/>`_::

        brew tap homebrew/versions
        brew install git
        brew install cmake
        brew install gcc45

    Using `MacPorts <https://www.macports.org>`_::

        sudo port install gcc45 +universal cmake +universal git-core +universal

    Macports will take some time - maybe hours.  At some point it may ask
    you to install a Java environment; let it do so.

#. Install perl dependencies

    1. ``sudo cpan``

       If this is the first time you've run cpan, you will need to go through the setup
       process. Just stick with the defaults for everything and you'll be fine.

       If you are running OS X 10.6 (Snow Leopard) or earlier, good luck!
       You'll need to open a separate Terminal window and run::

          sudo ln -s /usr/include/libxml2/libxml /usr/include/libxml

    2. ``install XML::LibXML``
    3. ``install XML::LibXSLT``

#. Get the dfhack source::

    git clone --recursive https://github.com/DFHack/dfhack.git
    cd dfhack

#. Set environment variables:

   Homebrew (if installed elsewhere, replace /usr/local with ``$(brew --prefix)``)::

    export CC=/usr/local/bin/gcc-4.5
    export CXX=/usr/local/bin/g++-4.5

   Macports::

    export CC=/opt/local/bin/gcc-mp-4.5
    export CXX=/opt/local/bin/g++-mp-4.5

#. Build dfhack::

    mkdir build-osx
    cd build-osx
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=/path/to/DF/directory
    make
    make install


Windows
=======
On Windows, DFHack replaces the SDL library distributed with DF.

Dependencies
------------
You will need some sort of Windows port of git, or a GUI. Some examples:

* `Git for Windows <https://git-for-windows.github.io>`_ (command-line and GUI)
* `tortoisegit <https://tortoisegit.org>`_ (GUI and File Explorer integration)

You need ``cmake``. Get the win32 installer version from
`the official site <http://www.cmake.org/cmake/resources/software.html>`_.
It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

You'll need a copy of Microsoft Visual C++ 2010. The Express version is sufficient.
Grab it from `Microsoft's site <http://download.microsoft.com/download/1/E/5/1E5F1C0A-0D5B-426A-A603-1798B951DDAE/VS2010Express1.iso>`_.
You'll also need the Visual Studio 2010 SP1 update.

For the code generation parts, you'll need perl with XML::LibXML and XML::LibXSLT.
`Strawberry Perl <http://strawberryperl.com>`_ works nicely for this and includes
all of the required packages.

If you already have a different version of perl (for example the one from cygwin),
you can run into some trouble. Either remove the other perl install from PATH, or
install libxml and libxslt for it instead.

Build
-----
There are several different batch files in the ``build`` folder along
with a script that's used for picking the DF path.

First, run ``set_df_path.vbs`` and point the dialog that pops up at your
DF folder that you want to use for development.
Next, run one of the scripts with ``generate`` prefix. These create the MSVC solution file(s):

* ``all`` will create a solution with everything enabled (and the kitchen sink).
* ``gui`` will pop up the cmake gui and let you pick and choose what to build.
  This is probably what you want most of the time. Set the options you are interested
  in, then hit configure, then generate. More options can appear after the configure step.
* ``minimal`` will create a minimal solution with just the bare necessities -
  the main library and standard plugins.

Then you can either open the solution with MSVC or use one of the msbuild scripts:

* Scripts with ``build`` prefix will only build DFHack.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

When you open the solution in MSVC, make sure you never use the Debug builds. Those aren't
binary-compatible with DF. If you try to use a debug build with DF, you'll only get crashes.
For this reason the Windows "debug" scripts actually do RelWithDebInfo builds,
so pick either Release or RelWithDebInfo build and build the INSTALL target.

