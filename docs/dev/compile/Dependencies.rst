.. _build-dependencies:

############
Dependencies
############

DFHack is meant to be installed into an **existing DF folder**, so ensure that
one is ready.

.. contents:: Contents
  :local:
  :depth: 2

Overview of Dependencies
========================

This section provides an overview of system dependencies that DFHack relies on.
See the platform-specific sections later in this document for specifics on how
to install these dependencies.

DFHack also has several dependencies on libraries that are included in the
repository as Git submodules, which require no further action to install.

System dependencies
-------------------


* CMake (v3.21 or newer is recommended)
* build system (e.g. gcc & ninja, or Visual Studio)
* Perl 5 (for code generation)
  * XML::LibXML
  * XML::LibXSLT
* Python 3 (for `documentation <documentation>`)
  * Sphinx
* Git (required for `contributions <https://github.com/DFHack/dfhack/pulls>`_)
* ccache (**optional**, but strongly recommended to improve build times)
* OpenGL headers (**optional**: to build `stonesense`)
* zlib (compression library used for `xlsxreader-api` -> `quickfort`)

Perl packages
-------------

* XML::LibXML
* XML::LibXSLT

The Perl packages are used in code generation. DF memory structures are
represented as XML in DFHack's source tree. During the configuration process
(cmake) the xml files are converted into C++ headers and Lua wrappers for use
by plugins and scripts.

Python packages
---------------

* Sphinx (required to build the `documentation <documentation>`)

Installing Dependencies
=======================

.. contents::
  :local:
  :depth: 2

.. _linux-dependency-instructions:

Linux
-----

Here are some package install commands for various distributions:

* On Arch Linux::

    pacman -Sy gcc cmake ccmake ninja git dwarffortress zlib perl-xml-libxml perl-xml-libxslt

  * The ``dwarffortress`` package provides the necessary SDL packages.
  * For the required Perl modules: ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On Ubuntu::

    apt-get install gcc cmake ninja-build git zlib1g-dev libsdl2-dev libxml-libxml-perl libxml-libxslt-perl

  * Other Debian-based distributions should have similar requirements.

* On Fedora::

    yum install gcc-c++ cmake ninja-build git zlib-devel SDL2-devel perl-core perl-XML-LibXML perl-XML-LibXSLT ruby

To build DFHack, you need GCC 10 or newer. Note that extremely new GCC versions
may not have been used to build DFHack yet, so if you run into issues with
these, please let us know (e.g. by opening a GitHub issue).

Note that distributing binaries compiled with newer GCC versions may result in
compatibility issues for players with older GCC versions. This is why DFHack
builds distributables with GCC 10, which is the same GCC version that DF itself
is compiled with.

Before you can build anything, you'll also need ``cmake``. It is advisable to
also get ``ccmake`` (the interactive configuration interface) on distributions
that split the cmake package into multiple parts. As mentioned above, ``ninja``
is recommended (many distributions call this package ``ninja-build``).

You will need pthread; most systems should have this already. Note that older
CMake versions may have trouble detecting pthread, so if you run into
pthread-related errors and pthread is installed, you may need to upgrade CMake,
either by downloading it from `cmake.org <https://cmake.org/download/>`_ or
through your package manager, if possible.

.. _windows-dependency-instructions:

Windows
-------

DFHack must be built with the Microsoft Visual C++ 2022 toolchain (aka MSVC v143)
for ABI compatibility with Dwarf Fortress v50.

.. contents::
    :local:
    :depth: 1

With Chocolatey
~~~~~~~~~~~~~~~
Many of the dependencies are simple enough to download and install via the
`chocolatey`_ package manager on the command line.

Here are some package install commands::

    choco install cmake
    choco install ccache
    choco install strawberryperl
    choco install python
    choco install sphinx

    # Visual Studio
    choco install visualstudio2022community --params "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
    # OR
    # Build Tools for Visual Studio
    choco install visualstudio2022buildtools --params "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"

If you already have Visual Studio 2022 or the Build Tools installed, you may
need to modify the installed version to include the workload components
listed in the manual installation section, as chocolatey will not amend
the existing install.

.. _chocolatey: https://chocolatey.org/install

Manually
~~~~~~~~
If you prefer to install manually rather than using Chocolatey, details and
requirements are as below. If you do install manually, **ensure that your PATH
variable is updated** to include the install locations for all tools. This can
be edited from ``Control Panel -> System -> Advanced System Settings ->
Environment Variables``.

.. contents::
  :local:
  :depth: 1

CMake
^^^^^
You can get the Windows installer from `the official site <https://cmake.org/download/>`_.
It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

Perl / Strawberry Perl
^^^^^^^^^^^^^^^^^^^^^^
For the code generation stage of the build process, you'll need Perl 5 with the
``XML::LibXML`` and ``XML::LibXSLT`` packages installed.
`Strawberry Perl <http://strawberryperl.com>`_ is recommended as it includes all
of the required packages in a single easy install.

After install, ensure Perl is in your user's PATH. The following directories must be in your PATH, in this order:

* ``<path to perl>\c\bin``
* ``<path to perl>\perl\site\bin``
* ``<path to perl>\perl\bin``
* ``<path to perl>\perl\vendor\lib\auto\XML\LibXML`` (path may only be required on some systems)

Be sure to close and re-open any existing ``cmd.exe`` windows after updating
your PATH.

If you already have a different version of Perl installed (for example, from Cygwin),
you can run into some trouble. Either remove the other Perl install from PATH, or
install XML::LibXML and XML::LibXSLT for it using CPAN.

Python
^^^^^^
See the `Python`_ website. Any supported version of Python 3 will work.

.. _Python: https://www.python.org/downloads/

Sphinx
^^^^^^
See the `Sphinx`_ website.

.. _Sphinx: https://www.sphinx-doc.org/en/master/usage/installation.html

.. _install-visual-studio:

Visual Studio
^^^^^^^^^^^^^
The required toolchain can be installed as a part of either the `Visual Studio 2022 IDE`_
or the `Build Tools for Visual Studio 2022`_. If you already have a preferred code
editor, the Build Tools will be a smaller install. You may need to log into (or create)
a Microsoft account in order to download Visual Studio.

.. _Visual Studio 2022 IDE: https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false
.. _Build Tools for Visual Studio 2022: https://my.visualstudio.com/Downloads?q=Build%20Tools%20for%20Visual%20Studio%202022


Build Tools [Without Visual Studio]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Click `Build Tools for Visual Studio 2022`_ and you will be prompted to login to your Microsoft account.
Then you should be redirected to a page with various download options with 2022
in their name. If this redirect doesn't occur, just copy, paste, and enter the
download link again and you should see the options.

You want to select the most up-to-date version -- as of writing this is
"Build Tools for Visual Studio 2022 (version 17.4)". "LTSC" is an extended
support variant and is not required for our purposes.

When installing, select the "Desktop Development with C++" workload and ensure that the following are checked:

- MSVC v143 - VS 2022 C++ x64/x86 build tools
- C++ CMake tools for Windows
- At least one Windows SDK (for example, Windows 11 SDK 10.0.22621).

.. _mac-dependency-instructions:

macOS
-----

NOTE: this section is currently outdated. Once DF itself can build on macOS
again, we will match DF's build environment and update the instructions here.

DFHack is easiest to build on macOS with exactly GCC 4.8 or 7. Anything newer than 7
will require you to perform extra steps to get DFHack to run (see `osx-new-gcc-notes`),
and your build will likely not be redistributable.

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

#. Install Python dependencies

    * You can choose to use a system Python 3 installation or any supported
      version of Python 3 from `python.org <https://www.python.org/downloads/>`__.

    * Install `Sphinx`_
