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
get code from the github repository using git.  How to get git is described under
the instructions for each platform.

To get the latest release code (master branch)::

    git clone --recursive https://github.com/DFHack/dfhack
    cd dfhack

If your version of git does not support the ``--recursive`` flag, you will need to omit it and run
``git submodule update --init`` after entering the dfhack directory.

To get the latest development code (develop branch)::

  git clone --recursive https://github.com/DFHack/dfhack
  cd dfhack
  git checkout develop
  git submodule update

**Important note on submodule update**:

It is necessary to run ``git submodule update`` every time you change Git branch,
for example when switching between master and develop branches and back.


Contributing to DFHack
======================
If you want to get involved with the development, create an account on
Github, make a clone there and then use that as your remote repository instead.

We'd love that; join us on IRC (#dfhack channel on freenode) for discussion,
and whenever you need help.


Build types
===========
``cmake`` allows you to pick a build type by changing the ``CMAKE_BUILD_TYPE`` variable::

    cmake .. -DCMAKE_BUILD_TYPE:string=BUILD_TYPE

Without specifying a build type or 'None', cmake uses the
``CMAKE_CXX_FLAGS`` variable for building.

Valid and useful build types include 'Release', 'Debug' and
'RelWithDebInfo'.
'Debug' is not available on Windows.


Linux
=====
On Linux, DFHack acts as a library that shadows parts of the SDL API using LD_PRELOAD.

Dependencies
------------
DFHack is meant to be installed into an existing DF folder, so get one ready.

We assume that any Linux platform will have ``git`` available.

To build DFHack you need a version of GCC 4.x capable of compiling for 32-bit
(i386) targets. GCC 4.5 is easiest to work with due to avoiding libstdc++ issues
(see below), but any version from 4.5 onwards (including 5.x) will work.

On 64-bit distributions, you'll need the multilib development tools and libraries:

* ``gcc-multilib`` and ``g++-multilib``
* On Debian: ``gcc-4.x-multilib`` / ``g++-4.x-multilib``

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

Here are some package install commands that should give you everything you need:

* On Arch linux:

  * ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On 64-bit Ubuntu:

  * ``apt-get install zlib1g-dev:i386 libxml-libxml-perl libxml-libxslt-perl gcc-multilib g++-multilib``.

* On 32-bit Ubuntu:

  * ``apt-get install gcc-multilib g++-multilib zlib1g-dev libxml-libxml-perl libxml-libxslt-perl``.

* Debian-derived distros should have similar requirements.


Build
-----
Building is fairly straightforward. Enter the ``build`` folder (or create an
empty folder in the DFHack directory to use instead) and start the build like this::

    cd build
    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=<path to DF>
    make install

<path to DF> should be a path to a copy of Dwarf Fortress, of the appropriate
version for the DFHack you are building. This will build the library along
with the normal set of plugins and install them into your DF folder.

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

    Note that it is recommended to use Homebrew instead of MacPorts,
    as it is generally cleaner, quicker, and smarter. It also doesn't
    require constant use of sudo.

#. Additional notes for El Capitan (OSX 10.11) users:

  #. You will probably find that gcc45 will fail to install on OSX 10.11,
     due to the presence of XCode 7.
  #. There are two workarounds:

    #. Install GCC 5.x instead (``brew install gcc5``), and then after compile
       replace ``hack/libstdc++.6.dylib`` with a symlink to GCC 5's i386
       version of this file::

        cd <path to df>/hack && mv libstdc++.6.dylib libstdc++.6.dylib.orig &&
        ln -s /usr/local/Cellar/gcc5/5.2.0/lib/gcc/5/i386/libstdc++.6.dylib .

    #. Install XCode 6, which is available as a free download from the Apple
       Developer Center.

      #. Either install this as your only XCode, or install it additionally
         to XCode 7 and then switch between them using ``xcode-select``
      #. Ensure XCode 6 is active before attempting to install GCC 4.5 and
         whenever you are compiling DFHack with GCC 4.5.

#. Install perl dependencies

    1. ``sudo cpan``

       If this is the first time you've run cpan, you will need to go through the setup
       process. Just stick with the defaults for everything and you'll be fine.

       If you are running OS X 10.6 (Snow Leopard) or earlier, good luck!
       You'll need to open a separate Terminal window and run::

          sudo ln -s /usr/include/libxml2/libxml /usr/include/libxml

    2. ``install XML::LibXML``
    3. ``install XML::LibXSLT``

#. Get the DFHack source as per section `compile-how-to-get-the-code`, above.

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
The Express version is sufficient.

You can grab it from `Microsoft's site <http://download.microsoft.com/download/1/E/5/1E5F1C0A-0D5B-426A-A603-1798B951DDAE/VS2010Express1.iso>`_.

You'll also need the Visual Studio 2010 SP1 update, which is obtained from
Windows Update. After installing Visual Studio, be sure to go to Windows Update
and check for and install the SP1 update.  If no update is found, check that
your Windows Update settings include "Updates from all Microsoft products".

You can confirm whether you have SP1 by opening the Visual Studio 2010 IDE
and selecting About from the Help menu.  If you have SP1 it will have *SP1Rel*
at the end of the version number, for example: *Version 10.0.40219.1 SP1Rel*

It is vital that you do use SP1 as, while building with the original release
of Visual Studio 2010 (RTM) may succeed, it will result in non-working DFHack
binaries that crash when connecting to Dwarf Fortress.

Additional dependencies: installing with the Chocolatey Package Manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The remainder of dependencies - git, cmake and StrawberryPerl - can be most
easily installed using the Chocolatey Package Manger. This is a system that
attempts to bring a Linux-like package manager to Windows.

Think "apt-get for Windows."

Chocolatey is the recommended way of installing the required dependencies
on Windows, as it's less work and installs known-good utilities with the
correct setup (especially PATH).

To install Chocolatey and the required dependencies:

* Go to https://chocolatey.org in a web browser
* At the top of the page it will give you the install command to copy

  * Copy the first one, that starts ``@powershell ...``
  * It won't be repeated here in case it changes in future Chocolatey releases.

* Open an elevated (Admin) cmd.exe window

  * On Windows 8 and later this can be easily achieved by:

    * right-clicking on the Start Menu, or pressing Win+X.
    * choosing "Command Prompt (Admin)"

  * On earlier Windows: find cmd.exe in Start Menu, right click
    and choose Open As Administrator.

* Paste in the Chocolatey install command, hit enter, and follow all prompts
* Close your Admin cmd.exe window, and open another Admin cmd.exe window
* Run the following command::

    choco install git cmake strawberryperl -y

* Close the Admin cmd.exe window; you're done!

You can now use all of the above commands from any future cmd.exe window,
and it does not need to be elevated (Admin).

**NOTE**: the above assumes you have none of Git, cmake and StrawberryPerl
already installed. If you do have one, you may want to remove that entry
from the install command listed above.

Additional dependencies: installing manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This is no longer generally recommended, as Chocolatey makes life a lot easier.
Use only if you have special requirements - or to check that your
already-installed versions of the below programs are as required for DFHack.

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
For the code generation parts, you'll need perl with XML::LibXML and XML::LibXSLT.
`Strawberry Perl <http://strawberryperl.com>`_ works nicely for this and includes
all of the required packages.

After install, ensure Perl is in your user's PATH directory. This can be edited
from ``Control Panel -> System -> Advanced System Settings -> Environment Variables``.

Be sure that all of the following three directories are present, in this order:

* ``<path to perl>\c\bin``
* ``<path to perl>\perl\site\bin``
* ``<path to perl>\perl\bin``

If you already have a different version of perl (for example the one from cygwin),
you can run into some trouble. Either remove the other perl install from PATH, or
install XML::LibXML and XML::LibXSLT for it using CPAN.

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

Building/installing from the command line:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Scripts with ``build`` prefix will only build DFHack.
* Scripts with ``install`` prefix will build DFHack and install it to the previously selected DF path.
* Scripts with ``package`` prefix will build and create a .zip package of DFHack.

Building/installing from the Visual Studio IDE:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After running the generate script as above, you will have a new folder called VC2010.
Open the file ``dfhack.sln`` from that folder - and if you have multiple versions of VS
installed, make sure you choose Visual Studio 2010.

The first thing you must then do is change the build type. It defaults to Debug,
but this cannot be used on Windows. Debug is not binary-compatible with DF.
If you try to use a debug build with DF, you'll only get crashes.
For this reason the Windows "debug" scripts actually do RelWithDebInfo builds,
so after loading the Solution, change the Build Type to either either Release
or RelWithDebInfo build.

Then build the ``INSTALL`` target listed under ``CMakePredefinedTargets``.


##########################
Building the documentation
##########################

DFHack documentation, like the file you are reading now, is created as .rst files,
which are in `reStructuredText (reST) <http://sphinx-doc.org/rest.html>`_ format.
This is a documenation format that has come from the Python community. It is very
similar in concept - and in syntax - to Markdown, as found on Github and many other places.
However it is more advanced than Markdown, and can be compiled to sophisticated HTML files
with tables of contents, cross-linking, references and more.

The documentation can be built during the standard DFHack compilation procedure,
but this has been disabled by default.  You only need to build the docs if you're changing
them, or perhaps if you want a local HTML copy; otherwise, read them easily online at
`ReadTheDoc's DFHack hosted documentation <https://dfhack.readthedocs.org>`_.

(Note that even if you do want a local copy, it is certainly not necesesary to compile the
documentation in order to read it. Like Markdown, reST documents are designed to be just as
readable in a plain-text editor as they are in HTML format. The main thing you lose in plain
text format is links.)


Enabling documentation building
===============================
First, make sure you have followed all the necessary steps for your platform as outlined
in the rest of this document.

#. Edit ``CMakeLists.txt`` in the root folder of your dfhack directory.
#. Find the line::

    OPTION(BUILD_DOCS "Choose whether to build the documentation (requires python and Sphinx)." OFF)

#. Change ``OFF`` to ``ON`` and save.
#. Now compile as normal, and the .rst documents will be compiled to HTML in
   ``/docs/html/docs/`` under your dfhack directory.
#. If you are committing to DFHack, be sure not to add your changed ``CMakeLists.txt`` to any commit.


Required dependencies
=====================
In order to build the documentation, you must have Python with Sphinx
version 1.3.1 or later. Both Python 2.x and 3.x are supported.

When installing Sphinx from OS package managers, be aware that there is
another program called Sphinx, completely unrelated to documentation management.
Be sure you are installing the right Sphinx!


Linux
-----PTION(BUILD_DOCS "Choose whether to build the documentation (requires python and Sphinx)." ON)
Most Linux distributions will include Python as standard, including the pip
package manager.

Check your package manager to see if Sphinx 1.3.1 or later is available,
but at the time of writing Ubuntu for example only has 1.2.x.

You can instead install the Python module with::

  pip install sphinx

If you run this as a normal user it will install a local copy for your user only.
Run it with sudo if you want a system-wide install.


Mac OS X
--------
OS X has Python 2.7 installed by default, but it does not have the pip package manager.

You can install Homebrew's Python 3, which includes pip, and then install the
latest Sphinx using pip::

  brew install python3
  pip3 install sphinx

Alternatively, you can simply install Sphinx 1.3.x directly from Homebrew::

  brew install sphinx-doc

This will directly install Sphinx for OS X's system Python 2.7, without needing pip.

Either method works; if you plan to use Python for other purposes, it might best
to install Homebrew's Python 3 so that you have the latest Python as well as pip.
If not, just installing sphinx-doc into OS X's system Python 2.7 is fine.


Windows
-------
Use the Chocolatey package manager to install Python and pip,
then use pip to install Sphinx.

Run the following commands from an elevated (Admin) cmd.exe, after installing
Chocolatey as outlined in the `Windows section <compile-windows>`::

  choco install python pip -y

Then close that Admin cmd.exe, re-open another Admin cmd.exe, and run::

  pip install sphinx
