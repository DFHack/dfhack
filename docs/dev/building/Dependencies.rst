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
* git (required for `contributions<pr-link>`)
* ccache (**optional**, but recommended to improve build times)
* OpenGL headers (**optional**: to build `stonesense`)
* zlib (compression library used for `xlsxioreader` -> `quickfort`)
* build system (e.g. gcc & ninja, or Visual Studio)

..
    maybe the below should be talked about next to the bullets

**SDL** is used as an injection point which you can see more about in DFHack's `architecture` documentation & diagrams.

Perl packages:

* XML::LibXML
* XML::LibXSLT

These perl packages are used in code generation. DFHack has many structures that are reverse engineered, we use xml
files to define and update these structures. Then during the configuration process [running cmake] these xml files are
used to generate C++ source code to define these structures for use in plugins and scripts.

.. _pr-link: https://github.com/DFHack/dfhack/pulls

Installing
----------

.. contents:: Install Instructions
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

On Windows, DFHack replaces the SDL library distributed with DF.
For ABI compatibility with recent releases of Dwarf Fortress, DFHack requires the ``v140`` or ``v140_xp``
toolchain to build for windows.

What you'll need is as follows:

* Microsoft Visual C++ 2022, 2019, 2017, or 2015 (optional)
* ``v140`` or ``v140_xp`` toolchain (Microsoft Visual C++ 2015 Build Tools)
* Git (optional)
* CMake
* StrawberryPerl, OR CPAN (optional, see nested)

  * Perl (required)
  * XML:LibXML (required)
  * XML:LibXLST (required)
* `Python <python-install>` (required for documentation, optional otherwise)

  * `Sphinx <sphinx-install>`

Releases of Dwarf Fortress since roughly 2016 have been compiled for Windows using
Microsoft's Visual Studio 2015 C++ compiler. In order to guarantee ABI and STL compatibility
with Dwarf Fortress, DFHack has to be compiled with the same compiler.

Visual Studio 2015 is no longer supported by Microsoft and it can be difficult to obtain
working installers for this product today. The recommended approach is to use a modern community
version of Visual Studio such as 2022_ or 2019_, installing additional optional Visual Studio
components which provide the required support for using Visual Studio 2015's toolchain.
All of the required tools are available from Microsoft as part of Visual Studio's Community
Edition at no charge.

You can also download just the Visual C++ 2015 `build tools`_ if you aren't going to use
Visual Studio to edit code.

.. _build tools: https://my.visualstudio.com/Downloads?q=visual%20studio%202015&wt.mc_id=o~msft~vscom~older-downloads

With Choco
~~~~~~~~~~
Many of the dependencies are simple enough to download and install via the
`chocolatey<chocolatey-link>` package manager on the command line.

Here are some package install commands::

    choco install cmake
    choco install ccache
    choco install strawberryperl
    choco install python
    choco install sphinx
    choco install visualstudio2022community

.. _chocolatey-link: https://chocolatey.org/install

Visual Studio
~~~~~~~~~~~~~
You could install visual studio `manually<install-visual-studio>`, perhaps even the build tools as well.
You could also just run a ``choco`` command. For example::

    choco install visualstudio2022community

If Visual Studio is installed follow these next steps for the build tools:

1. Open **Visual Studio Installer**.
2. Select modify, for whichever version you've chosen to utilize.
3. Check the boxes for the following components:

* "Desktop Development with C++"
* "C++ Windows XP Support for VS 2017 (v141) tools [Deprecated]"
* "MSVC v140 - VS 2015 C++ build tools (v14.00)"

Yes, this is unintuitive. Installing XP Support for VS 2017 installs XP Support for VS 2015
if the 2015 toolchain is installed.

Manually
~~~~~~~~
If you prefer to install manually rather than using Chocolatey, details and
requirements are as below. If you do install manually, please ensure you
have all **executables searchable in your PATH variable**.

.. contents:: Windows
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

Python
^^^^^^
See `python-install`.

.. _python-install: https://www.python.org/downloads/

Sphinx
^^^^^^
See `sphinx-install` at https://www.sphinx-doc.org/

.. _sphinx-install: https://www.sphinx-doc.org/en/master/usage/installation.html

.. _install-visual-studio:

Visual Studio
^^^^^^^^^^^^^
Click Visual Studio 2022_ or 2019_ to download an installer wizard that will prompt you
to select the optional tools you want to download alongside the IDE. You may need to log into
(or create) a Microsoft account in order to download Visual Studio.

.. _2022: https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false
.. _2019: https://my.visualstudio.com/Downloads?q=visual%20studio%202019&wt.mc_id=o~msft~vscom~older-downloads

Build Tools [Without Visual Studio]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Click `build tools`_ and you will be prompted to login to your Microsoft account.
Then you should be redirected to a page with various download options with 2015
in their name. If this redirect doesn't occur, just copy, paste, and enter the
download link again and you should see the options. You need to get:

Visual C++ Build Tools for Visual Studio 2015 with Update 3.
Click the download button next to it and a dropdown of download formats will appear.
Select the DVD format to download an ISO file. When the download is complete,
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

.. _mac-dependency-instructions:

macOS
=====

DFHack can officially be built on macOS only with GCC 4.8 or 7. Anything newer than 7
will require you to perform extra steps to get DFHack to run (see `osx-new-gcc-notes`),
and your build will likely not be redistributable.

.. _osx-setup:

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
