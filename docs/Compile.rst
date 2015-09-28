################
Compiling DFHack
################

.. important::
    You don't need to compile DFHack unless you're developing plugins or working on the core.
    For users, modders, and authors of scripts it's better to download the latest release instead.

.. contents::
    :depth: 2

===================
How to get the code
===================
DFHack doesn't have any kind of system of code snapshots in place, so you will have to
get code from the github repository using git.
The code resides at https://github.com/DFHack/dfhack

On Linux and OS X, having a ``git`` package installed is the minimal requirement (see below for OS X instructions),
but some sort of git gui or git integration for your favorite text editor/IDE will certainly help.

On Windows, you will need some sort of Windows port of git, or a GUI. Some examples:

 * http://msysgit.github.io - this is a command line version of git for windows.
   Most tutorials on git usage will apply.
 * http://code.google.com/p/tortoisegit - this puts a pretty, graphical face on top of msysgit

To get the code::

    git clone --recursive https://github.com/DFHack/dfhack
    cd dfhack

If your version of git does not support the ``--recursive`` flag, you will need to omit it and run
``git submodule update --init`` after entering the dfhack directory.

If you just want to compile DFHack or work on it by contributing patches, it's quite
enough to clone from the read-only address instead::

    git clone --recursive git://github.com/DFHack/dfhack.git
    cd dfhack

If you want to get really involved with the development, create an account on
Github, make a clone there and then use that as your remote repository instead.
Detailed instructions are beyond the scope of this document. If you need help,
join us on IRC (#dfhack channel on freenode).

===========
Build types
===========
``cmake`` allows you to pick a build type by changing this
variable: ``CMAKE_BUILD_TYPE``  ::

    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid and useful build types include 'Release', 'Debug' and
'RelWithDebInfo'. 'Debug' is not available on Windows.

=====
Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
============
DFHack is meant to be installed into an existing DF folder, so get one ready.

For building, you need a 32-bit version of GCC. For example, to build DFHack on
a 64-bit distribution like Arch, you'll need the multilib development tools and libraries.
Alternatively, you might be able to use ``lxc`` to
`create a virtual 32-bit environment <http://www.bay12forums.com/smf/index.php?topic=139553.msg5435310#msg5435310>`_.

Before you can build anything, you'll also need ``cmake``. It is advisable to also get
``ccmake`` on distributions that split the cmake package into multiple parts.

You also need perl and the XML::LibXML and XML::LibXSLT perl packages (for the code generation parts).
You should be able to find them in your distro repositories (on Arch linux
``perl-xml-libxml`` and ``perl-xml-libxslt``) or through ``cpan``.

To build Stonesense, you'll also need OpenGL headers.

Some additional dependencies for other distros are listed on the
`wiki <https://github.com/DFHack/dfhack/wiki/Linux-dependencies>`_.

Build
=====
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
extra options.

You can also use a cmake-friendly IDE like KDevelop 4 or the cmake-gui
program.

Fixing the libstdc++ version bug
================================

When compiling dfhack yourself, it builds against your system libc.
When Dwarf Fortress runs, it uses a libstdc++ shipped with the binary, which
comes from GCC 4.5 and is incompatible with code compiled with newer GCC versions.
This manifests itself with the error message::

   ./libs/Dwarf_Fortress: /pathToDF/libs/libstdc++.so.6: version
       `GLIBCXX_3.4.15' not found (required by ./hack/libdfhack.so)

To fix this, simply remove the libstdc++ shipped with DF, it will fall back
to your system lib and everything will work fine::

    cd /path/to/DF/
    rm libs/libstdc++.so.6

Alternatively, this issue can be avoided by compiling DFHack with GCC 4.5.

========
Mac OS X
========

DFHack functions similarly on OS X and Linux, and the majority of the
information above regarding the build process (cmake and make) applies here
as well.

* If you are building on 10.6, please read the subsection below titled "Snow Leopard Changes" FIRST.
* If you are building on 10.10+, read the "Yosemite Changes" subsection before building.

1. Download and unpack a copy of the latest DF
2. Install Xcode from Mac App Store
3. Open Xcode, go to Preferences > Downloads, and install the Command Line Tools.
4. Install dependencies

    Option 1: Using Homebrew:

        * `Install Homebrew <http://brew.sh/>`_ and run:
        * ``brew tap homebrew/versions``
        * ``brew install git``
        * ``brew install cmake``
        * ``brew install gcc45``

    Option 2: Using MacPorts:

        * `Install MacPorts <http://www.macports.org/>`_
        * Run ``sudo port install gcc45 +universal cmake +universal git-core +universal``
          This will take some time—maybe hours, depending on your machine.

        At some point during this process, it may ask you to install a Java environment; let it do so.

5. Install perl dependencies

    1. ``sudo cpan``

       If this is the first time you've run cpan, you will need to go through the setup
       process. Just stick with the defaults for everything and you'll be fine.

    2. ``install XML::LibXML``
    3. ``install XML::LibXSLT``

6. Get the dfhack source::

    git clone --recursive https://github.com/DFHack/dfhack.git
    cd dfhack

7. Set environment variables:

  Homebrew (if installed elsewhere, replace /usr/local with ``$(brew --prefix)``)::

    export CC=/usr/local/bin/gcc-4.5
    export CXX=/usr/local/bin/g++-4.5

  Macports::

    export CC=/opt/local/bin/gcc-mp-4.5
    export CXX=/opt/local/bin/g++-mp-4.5

8. Build dfhack::

    mkdir build-osx
    cd build-osx
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=/path/to/DF/directory
    make
    make install


Snow Leopard Changes
====================

1. Add a step 6.2a (before Install XML::LibXSLT)::
    In a separate Terminal window or tab, run:
    ``sudo ln -s /usr/include/libxml2/libxml /usr/include/libxml``

2. Add a step 7a (before building)::
    In <dfhack directory>/library/LuaTypes.cpp, change line 467 to
        ``int len = strlen((char*)ptr);``

Yosemite Changes
================

If you have issues building on OS X Yosemite (or above), try definining the
following environment variable::

    export MACOSX_DEPLOYMENT_TARGET=10.9

=======
Windows
=======
On Windows, DFHack replaces the SDL library distributed with DF.

Dependencies
============
First, you need ``cmake``. Get the win32 installer version from the official
site: http://www.cmake.org/cmake/resources/software.html

It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

You'll need a copy of Microsoft Visual C++ 2010. The Express version is sufficient.
Grab it from Microsoft's site.

You'll also need the Visual Studio 2010 SP1 update.

For the code generation parts, you'll need perl with XML::LibXML and XML::LibXSLT.
`Strawberry Perl <http://strawberryperl.com>`_ works nicely for this.

If you already have a different version of perl (for example the one from cygwin),
you can run into some trouble. Either remove the other perl install from PATH, or
install libxml and libxslt for it instead. Strawberry perl works though and has
all the required packages.

Build
=====
There are several different batch files in the ``build`` folder along with a script that's used for picking the DF path.

First, run set_df_path.vbs and point the dialog that pops up at your DF folder that you want to use for development.
Next, run one of the scripts with ``generate`` prefix. These create the MSVC solution file(s):

* ``all`` will create a solution with everything enabled (and the kitchen sink).
* ``gui`` will pop up the cmake gui and let you pick and choose what to build. This is probably what you want most of the time. Set the options you are interested in, then hit configure, then generate. More options can appear after the configure step.
* ``minimal`` will create a minimal solution with just the bare necessities - the main library and standard plugins.

Then you can either open the solution with MSVC or use one of the msbuild scripts:

* Scripts with ``build`` prefix will only build.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

When you open the solution in MSVC, make sure you never use the Debug builds. Those aren't
binary-compatible with DF. If you try to use a debug build with DF, you'll only get crashes.
So pick either Release or RelWithDebInfo build and build the INSTALL target.

The ``debug`` scripts actually do RelWithDebInfo builds.
