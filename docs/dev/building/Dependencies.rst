.. _build-dependencies:

############
Dependencies
############

The most immediate consideration is of course that DFHack is meant to be installed into an **existing DF folder**.
So it is prudent that one is ready.

.. contents:: Contents
  :local:
  :depth: 2

Build Dependencies
------------------

..
    DFHack is quite large, so I've attempted to
    leave some sort of bread crumbs for each
    mentionable aspect.

Many of DFHack's build dependencies are included in the repository as git submodules,
however there are some system dependencies as well. They are as follows:

System packages:

* SDL (libsdl 1.2, not sdl2).
* cmake
* Perl
* Python

  * Sphinx
* git (required for `contributions <https://github.com/DFHack/dfhack/pulls>`_)
* ccache (**optional**, but recommended to improve build times)
* OpenGL headers (**optional**: to build `stonesense`)
* zlib (compression library used for `xlsxreader-api` -> `quickfort`)
* build system (e.g. gcc & ninja, or Visual Studio)

..
    maybe the below should be talked about next to the bullet point??

**SDL** is used as an injection point which you can see more about in DFHack's `architectural <architectural-diagrams>` documentation & diagrams.

Perl packages:

* XML::LibXML
* XML::LibXSLT

These perl packages are used in code generation. DFHack has many structures that are reverse engineered, we use xml
files to define and update these structures. Then during the configuration process [running cmake] these xml files are
used to generate C++ source code to define these structures for use in plugins and scripts.


Installing
----------

.. contents::
  :local:
  :depth: 2

.. _linux-dependency-instructions:

Linux
=====

Here are some package install commands for various distributions:

* On Arch linux::

    pacman -Sy gcc cmake ccmake ninja git dwarffortress zlib perl-xml-libxml perl-xml-libxslt

  * The ``dwarffortress`` package provides the necessary SDL packages.
  * For the required Perl modules: ``perl-xml-libxml`` and ``perl-xml-libxslt`` (or through ``cpan``)

* On Ubuntu::

    apt-get install gcc cmake ninja-build git zlib1g-dev libsdl1.2-dev libxml-libxml-perl libxml-libxslt-perl

  * Other Debian-based distributions should have similar requirements.

* On Fedora::

    yum install gcc-c++ cmake ninja-build git zlib-devel SDL-devel perl-core perl-XML-LibXML perl-XML-LibXSLT ruby

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

Building 32-bit Binaries
~~~~~~~~~~~~~~~~~~~~~~~~
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
the opposite compatibility issue: users with *older* GCC versions may encounter
similar errors. This is why DFHack distributes both GCC 4.8 and GCC 7 builds. If
you are planning on distributing binaries to other users, we recommend using an
older GCC (but still at least 4.8) version if possible.

.. _windows-dependency-instructions:

Windows
=======

DFHack must be built with the Microsoft Visual C++ 2022 toolchain (aka MSVC v143)
for ABI compatibility with Dwarf Fortress v50.

With Choco
~~~~~~~~~~
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
requirements are as below. If you do install manually, please ensure you
have all **executables searchable in your PATH variable**.

.. contents::
  :local:
  :depth: 1

CMake
^^^^^
You can get the win32 installer version from
`the official site <https://cmake.org/download/>`_.
It has the usual installer wizard. Make sure you let it add its binary folder
to your binary search PATH so the tool can be later run from anywhere.

Perl / Strawberry Perl
^^^^^^^^^^^^^^^^^^^^^^
For the code generation stage of the build process, you'll need Perl 5 with
**XML::LibXML** and **XML::LibXSLT**. `Strawberry Perl <http://strawberryperl.com>`_ is
recommended as it includes all of the required packages in a single easy
install.

After install, ensure Perl is in your user's PATH. This can be edited from
``Control Panel -> System -> Advanced System Settings -> Environment Variables``.

The following directories must be in your PATH, in this order:

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
See the `Python`_ website.

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
=====

DFHack can officially be built on macOS only with GCC 4.8 or 7. Anything newer than 7
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
